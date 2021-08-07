#pragma once
#include <string>
#include <memory>
#include "protoco.h"
#include "transport.h"

namespace asyncio {

class ProtocolFactory;

class EventLoop {
	using MSG_CALLBACK = std::function<void()>;

public:
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
} // namespace asyncio
