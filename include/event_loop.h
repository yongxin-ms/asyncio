#pragma once
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <asio.hpp>
#include "protocol.h"
#include "log.h"
#include "listener.h"
#include "timer.h"

namespace asyncio {

class ProtocolFactory;

class EventLoop {
	using MSG_CALLBACK = std::function<void()>;

public:
	EventLoop();
	EventLoop(const EventLoop&) = delete;
	EventLoop& operator=(const EventLoop&) = delete;

	void RunForever();
	void RunUntilComplete();

	bool IsRunning();
	void Stop();

	void RunInLoop(MSG_CALLBACK&& func);
	void QueueInLoop(MSG_CALLBACK&& func);
	std::shared_ptr<TimerWrap> CallLater(int milliseconds, MSG_CALLBACK&& func);

	std::shared_ptr<Transport> CreateConnection(
		ProtocolFactory& protocol_factory, const std::string& host, uint16_t port);
	Listener* CreateServer(ProtocolFactory& protocol_factory, int port);

	asio::io_context& IOContext() { return m_context; }

private:
	asio::io_context m_context;
	asio::executor_work_guard<asio::io_context::executor_type> m_work;
};

EventLoop::EventLoop()
	: m_work(asio::make_work_guard(m_context)) {}

void EventLoop::RunForever() {
	m_context.run();
}

void EventLoop::RunUntilComplete() {}

bool EventLoop::IsRunning() {
	return true;
}

void EventLoop::Stop() {
	m_context.stop();
}

void EventLoop::RunInLoop(MSG_CALLBACK&& func) {}

void EventLoop::QueueInLoop(MSG_CALLBACK&& func) {
	asio::post(func);
}

std::shared_ptr<TimerWrap> EventLoop::CallLater(int milliseconds, MSG_CALLBACK&& func) {
	auto timer = std::make_shared<TimerWrap>(m_context, milliseconds, std::move(func));
	return timer;
}

std::shared_ptr<Transport> EventLoop::CreateConnection(
	ProtocolFactory& protocol_factory, const std::string& host, uint16_t port) {
	auto transport = std::make_shared<Transport>(m_context, *protocol_factory.CreateProtocol(), host, port);
	transport->Connect();
	return transport;
}

// 监听器需要用户手工删除
Listener* EventLoop::CreateServer(ProtocolFactory& protocol_factory, int port) {
	auto listener = new Listener(m_context, protocol_factory);
	if (!listener->Listen(port)) {
		delete listener;
		return nullptr;
	}
	return listener;
}

} // namespace asyncio
