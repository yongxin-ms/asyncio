/* asyncio implementation.
 *
 * Copyright (c) 2019-2022, Will <will at i007 dot cc>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of asyncio nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <string>
#include <memory>
#include <functional>
#include <asyncio/type.h>
#include <asyncio/log.h>
#include <asyncio/timer.h>
#include <asyncio/http_server.h>
#include <asyncio/protocol.h>
#include <asyncio/context_pool.h>
#include <asyncio/transport.h>
#include <asyncio/listener.h>

namespace asyncio {

class ProtocolFactory;

static const int DEFAULT_IO_THREAD_NUM = std::min(uint32_t(3), std::thread::hardware_concurrency() - 1);

class EventLoop {
	using MSG_CALLBACK = std::function<void()>;

public:
	explicit EventLoop(size_t work_io_num = DEFAULT_IO_THREAD_NUM);
	EventLoop(const EventLoop&) = delete;
	EventLoop& operator=(const EventLoop&) = delete;
	~EventLoop() {
		m_worker_io = nullptr;
		ASYNCIO_LOG_DEBUG("EventLoop destroyed");
	}

	void RunForever();
	void Stop();
	void QueueInLoop(MSG_CALLBACK&& func);

	template <class Rep, class Period>
	[[nodiscard]] DelayTimerPtr CallLater(
		const std::chrono::duration<Rep, Period>& timeout, DelayTimer::FUNC_CALLBACK&& func, int run_times = RUN_ONCE);
	[[nodiscard]] ProtocolPtr CreateConnection(
		ProtocolFactory& protocol_factory, const std::string& host, uint16_t port);

	[[nodiscard]] ProtocolPtr CreateConnection(ProtocolFactory& protocol_factory, const std::string& remote_addr);
	[[nodiscard]] ListenerPtr CreateServer(ProtocolFactory& protocol_factory, uint16_t port);
	[[nodiscard]] http::server_ptr CreateHttpServer(uint16_t port, http::request_handler handler);

private:
	auto& MainIOContext() {
		return m_main_context;
	}

	IOContext& WorkerIOContext();

	bool IsInLoopThread() const {
		return m_thread_id == std::this_thread::get_id();
	}

	auto GetThreadId() const {
		return m_thread_id;
	}

private:
	IOContext m_main_context;
	IOWorker m_main_work;
	ContextPoolPtr m_worker_io;
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
	ASYNCIO_LOG_INFO("EventLoop thread_id:%u", thread_id);

	m_main_context.run();
}

void EventLoop::Stop() {
	m_main_work.reset();
	m_main_context.stop();
	ASYNCIO_LOG_INFO("EventLoop stopped");
}

void EventLoop::QueueInLoop(MSG_CALLBACK&& func) {
	asio::post(m_main_context, std::move(func));
}

template <class Rep, class Period>
DelayTimerPtr EventLoop::CallLater(
	const std::chrono::duration<Rep, Period>& timeout, DelayTimer::FUNC_CALLBACK&& func, int run_times) {
	if (run_times < 0) {
		throw std::runtime_error("wrong run_times");
	}

	auto cur_thread_id = std::this_thread::get_id();
	if (cur_thread_id != m_thread_id) {
		ASYNCIO_LOG_ERROR("Thread Error, cur_thread_id:%u, m_thread_id:%u", cur_thread_id, m_thread_id);
		throw std::runtime_error("this function can only be called in main loop thread");
	}

	auto timer = std::make_unique<DelayTimer>(cur_thread_id, m_main_context, timeout, std::move(func), run_times);
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

ListenerPtr EventLoop::CreateServer(ProtocolFactory& protocol_factory, uint16_t port) {
	auto listener = std::make_shared<Listener>(m_main_context, m_worker_io, protocol_factory);
	if (!listener->Listen(port)) {
		ASYNCIO_LOG_ERROR("Listen on %d failed", port);
		return nullptr;
	}
	return listener;
}

http::server_ptr EventLoop::CreateHttpServer(uint16_t port, http::request_handler handler) {
	auto http_server = std::make_shared<http::server>(WorkerIOContext(), handler);
	if (!http_server->listen(port)) {
		ASYNCIO_LOG_ERROR("listen on %d failed", port);
		return nullptr;
	}
	return http_server;
}

} // namespace asyncio
