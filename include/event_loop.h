#pragma once

namespace asyncio {

class ProtocolFactory;

class EventLoop {
public:
	void run_forever();
	void run_until_complete();
	void stop();
	bool is_running();
	bool is_closed();
	void close();

	void call_soon();
	void call_later();
	void call_at();

	void create_connection(ProtocolFactory* protocol_factory, const std::string& host, int port);
	void create_server(ProtocolFactory* protocol_factory, int port);
};
} // namespace asyncio
