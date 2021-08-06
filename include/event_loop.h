#pragma once

namespace asyncio {

class ProtocolFactory;

class EventLoop {
	using MSG_CALLBACK = std::function<void()>;

public:
	void RunForever();
	void RunUntilComplete();
	void Stop();
	bool IsRunning();
	bool IsClosed();
	void Close();

	void Post(MSG_CALLBACK&& func);
	void CallLater(int milliseconds, MSG_CALLBACK&& func);

	void CreateConnection(ProtocolFactory& protocol_factory, const std::string& host, int port);
	void CreateServer(ProtocolFactory& protocol_factory, int port);
};
} // namespace asyncio
