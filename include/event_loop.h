#pragma once

namespace asyncio {

class ProtocolFactory;

class EventLoop {
public:
	void RunForever();
	void RunUntilComplete();
	void Stop();
	bool IsRunning();
	bool IsClosed();
	void Close();

	void CallSoon();
	void CallLater();
	void CallAt();

	void CreateConnection(ProtocolFactory* protocol_factory, const std::string& host, int port);
	void CreateServer(ProtocolFactory* protocol_factory, int port);
};
} // namespace asyncio
