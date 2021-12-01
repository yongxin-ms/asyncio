#pragma once
#include <string>
#include <memory>
#include <functional>
#include <asyncio/type.h>
#include <asyncio/protocol.h>
#include <asyncio/context_pool.h>
#include <asyncio/log.h>
#include <asyncio/listener.h>
#include <asyncio/timer.h>
#include <asyncio/http_server.h>

namespace asyncio {

class ProtocolFactory;

class EventLoop {
	using MSG_CALLBACK = std::function<void()>;
public:
	explicit EventLoop(size_t work_io_num = 0);
	EventLoop(const EventLoop&) = delete;
	EventLoop& operator=(const EventLoop&) = delete;

	void RunForever();
	void Stop();
	void QueueInLoop(MSG_CALLBACK&& func);

	/*
	 * 请通过持有返回值来控制定时器的生命周期
	 */
	DelayTimerPtr CallLater(int milliseconds, DelayTimer::FUNC_CALLBACK&& func, int run_times = DelayTimer::RUN_ONCE);
	ProtocolPtr CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, uint16_t port);

	/*
	 * remote_addr形如 127.0.0.1:9000
	 */
	ProtocolPtr CreateConnection(ProtocolFactory& protocol_factory, const std::string& remote_addr);
	ListenerPtr CreateServer(ProtocolFactory& protocol_factory, uint16_t port);
	http::server_ptr CreateHttpServer(uint16_t port, http::request_handler handler);

private:
	IOContext& MainIOContext() {
		return m_main_context;
	}

	IOContext& WorkerIOContext();

	bool IsInLoopThread() const {
		return m_thread_id == std::this_thread::get_id();
	}

	std::thread::id GetThreadId() const {
		return m_thread_id;
	}

private:
	IOContext m_main_context;
	IOWorker m_main_work;
	std::shared_ptr<ContextPool> m_worker_io;
	const std::thread::id m_thread_id;
};

EventLoop::EventLoop(size_t work_io_num)
	: m_main_work(asio::make_work_guard(m_main_context))
	, m_thread_id(std::this_thread::get_id()) {
	if (work_io_num > 0) {
		m_worker_io = std::make_shared<ContextPool>(work_io_num);
		ASYNCIO_LOG_INFO("EventLoop init with %d io thread(s)", work_io_num);
	} else {
		ASYNCIO_LOG_INFO("EventLoop init with single thread");
	}
}

IOContext& EventLoop::WorkerIOContext() {
	if (m_worker_io == nullptr) {
		return m_main_context;
	} else {
		return m_worker_io->NextContext();
	}
}

void EventLoop::RunForever() {
	auto thread_id = std::this_thread::get_id();
	ASYNCIO_LOG_INFO("EventLoop thread_id:%d", thread_id);

	m_main_context.run();
}

void EventLoop::Stop() {
	m_main_context.stop();
	ASYNCIO_LOG_INFO("EventLoop stopped");
}

void EventLoop::QueueInLoop(MSG_CALLBACK&& func) {
	asio::post(m_main_context, std::move(func));
}

DelayTimerPtr EventLoop::CallLater(int milliseconds, DelayTimer::FUNC_CALLBACK&& func, int run_times) {
	auto cur_thread_id = std::this_thread::get_id();
	if (cur_thread_id != m_thread_id) {
		ASYNCIO_LOG_ERROR("Thread Error, cur_thread_id:%d, m_thread_id:%d", cur_thread_id, m_thread_id);
		throw std::runtime_error("this function can only be called in main loop thread");
	}

	auto timer = std::make_unique<DelayTimer>(m_thread_id, m_main_context, milliseconds, std::move(func));
	timer->Run(run_times);
	return timer;
}

ProtocolPtr EventLoop::CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, uint16_t port) {
	auto protocol = protocol_factory.CreateProtocol();
	auto transport = std::make_shared<Transport>(WorkerIOContext(), protocol, host, port);
	transport->Connect();
	return protocol;
}

ProtocolPtr EventLoop::CreateConnection(ProtocolFactory& protocol_factory, const std::string& remote_addr) {
	std::vector<std::string> vec;
	if (util::Text::SplitStr(vec, remote_addr, ':') != 2) {
		return nullptr;
	}

	const std::string& host = vec[0];
	uint16_t port = atoi(vec[1].data());
	return CreateConnection(protocol_factory, host, port);
}

// 使用者应该保持这个监听器
ListenerPtr EventLoop::CreateServer(ProtocolFactory& protocol_factory, uint16_t port) {
	auto listener = std::make_shared<Listener>(m_main_context, m_worker_io, protocol_factory);
	if (!listener->Listen(port)) {
		ASYNCIO_LOG_ERROR("Listen on %d failed", port);
		return nullptr;
	}
	return listener;
}

http::server_ptr EventLoop::CreateHttpServer(uint16_t port, http::request_handler handler) {
	auto http_server = std::make_shared<http::server>(WorkerIOContext(), port, handler);
	return http_server;
}

} // namespace asyncio
