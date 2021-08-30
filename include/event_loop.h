#pragma once
#include <string>
#include <memory>
#include <functional>
#include "protocol.h"
#include "context_pool.h"
#include "log.h"
#include "listener.h"
#include "timer.h"
#include "http_server.h"

namespace asyncio {

class ProtocolFactory;

class EventLoop {
public:
	using MSG_CALLBACK = std::function<void()>;

	EventLoop(size_t work_io_num = 0);
	EventLoop(const EventLoop&) = delete;
	EventLoop& operator=(const EventLoop&) = delete;

	IOContext& MainIOContext() { return m_main_context; }
	IOContext& WorkerIOContext();

	void RunForever();
	void Stop();
	void QueueInLoop(MSG_CALLBACK&& func);
	DelayTimerPtr CallLater(int milliseconds, DelayTimer::FUNC_CALLBACK&& func, int run_times = DelayTimer::RUN_ONCE);
	ProtocolPtr CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, uint16_t port);

	/*
	remote_addr形如 127.0.0.1:9000
	*/
	ProtocolPtr CreateConnection(ProtocolFactory& protocol_factory, const std::string& remote_addr);
	ListenerPtr CreateServer(ProtocolFactory& protocol_factory, uint16_t port);
	http::server_ptr CreateHttpServer(uint16_t port, http::request_handler handler);

private:
	IOContext m_main_context;
	IOWorker m_main_work;
	std::shared_ptr<ContextPool> m_worker_io;
};

EventLoop::EventLoop(size_t work_io_num)
	: m_main_work(asio::make_work_guard(m_main_context)) {
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
	auto timer = std::make_shared<DelayTimer>(m_main_context, milliseconds, std::move(func));
	timer->Run(run_times);
	return timer;
}

ProtocolPtr EventLoop::CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, uint16_t port) {
	auto transport = std::make_shared<Transport>(WorkerIOContext(), protocol_factory.CreateProtocol(), host, port);
	transport->Connect();
	return transport->GetProtocol();
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
