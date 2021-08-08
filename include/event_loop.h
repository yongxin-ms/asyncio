#pragma once
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <condition_variable>
#include <mutex>
#include "protocol.h"
#include "transport.h"
#include "log.h"
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
	std::shared_ptr<BaseTimer> CallLater(int milliseconds, MSG_CALLBACK&& func);

	void CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, uint16_t port);
	void CreateServer(ProtocolFactory& protocol_factory, int port);

private:
	std::queue<MSG_CALLBACK> m_queue;
	std::condition_variable m_condition;
	mutable std::mutex m_mutex;
	std::atomic<bool> m_stop = false;
	TimerMgr m_timer_mgr;
	asio::io_context m_context;
};

EventLoop::EventLoop() {
}

void EventLoop::RunForever() {
	std::unique_lock<std::mutex> lock(m_mutex);
	std::queue<MSG_CALLBACK> msgs;

	while (true) {
		auto nearest_time =
			(std::min)(std::chrono::system_clock::now() + std::chrono::milliseconds(500), m_timer_mgr.GetNearestTime());
		m_condition.wait_until(lock, nearest_time,
							  [this] { return m_stop.load(std::memory_order_acquire) || !m_queue.empty(); });
		std::swap(msgs, m_queue);
		lock.unlock();

		while (!msgs.empty()) {
			const auto& p = msgs.front();
			p();
			msgs.pop();
		}

		m_timer_mgr.Update();
		lock.lock();
		if (m_stop.load(std::memory_order_acquire) && m_queue.empty()) {
			lock.unlock();
			break;
		}
	}
}

void EventLoop::RunUntilComplete() {
}

bool EventLoop::IsRunning() {
	return true;
}

void EventLoop::Stop() {
	m_stop.exchange(true, std::memory_order_release);
	m_condition.notify_one();
}

void EventLoop::RunInLoop(MSG_CALLBACK&& func) {
}

void EventLoop::QueueInLoop(MSG_CALLBACK&& func) {
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.emplace(std::move(func));
	}
	m_condition.notify_one();
}

std::shared_ptr<BaseTimer> EventLoop::CallLater(int milliseconds, MSG_CALLBACK&& func) {
	return m_timer_mgr.AddDelayTimer(milliseconds, std::move(func));
}

void EventLoop::CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, uint16_t port) {
	auto transport = std::make_shared<Transport>(m_context, *protocol_factory.CreateProtocol(), host, port);
}

void EventLoop::CreateServer(ProtocolFactory& protocol_factory, int port) {
}

} // namespace asyncio
