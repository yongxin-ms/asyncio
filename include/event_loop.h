#pragma once
#include <string>
#include <memory>
#include <functional>
#include "protocol.h"
#include "transport.h"
#include "log.h"

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
	void CallLater(int milliseconds, MSG_CALLBACK&& func);

	void CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, int port);
	void CreateServer(ProtocolFactory& protocol_factory, int port);
};

EventLoop::EventLoop() {
}

void EventLoop::RunForever() {
}

void EventLoop::RunUntilComplete() {
}

bool EventLoop::IsRunning() {
	return true;
}

void EventLoop::Stop() {
}

void EventLoop::RunInLoop(MSG_CALLBACK&& func) {
}

void EventLoop::QueueInLoop(MSG_CALLBACK&& func) {
}

void EventLoop::CallLater(int milliseconds, MSG_CALLBACK&& func) {
}

void EventLoop::CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, int port) {
}

void EventLoop::CreateServer(ProtocolFactory& protocol_factory, int port) {
}

} // namespace asyncio
