#include "asyncio.h"

class MyConnection : public asyncio::Protocol {
public:
	MyConnection(asyncio::Log& log) : m_log(log) {}

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		m_log.DoLog(asyncio::Log::kInfo, "ConnectionMade");
	}

	virtual void ConnectionLost(int err_code) override {
		m_transport = nullptr;
		m_log.DoLog(asyncio::Log::kInfo, "ConnectionLost");
	}

	virtual void DataReceived(const char* data, size_t len) override {
		m_log.DoLog(asyncio::Log::kInfo, "DataReceived: %s", data);
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
		m_log.DoLog(asyncio::Log::kInfo, "EofReceived");
	}

	size_t Send(const char* data, size_t len) {
		if (m_transport == nullptr)
			return 0;
		m_log.DoLog(asyncio::Log::kInfo, "Send: %s", data);
		m_transport->Write(data, len);
		return len;
	}

private:
	asyncio::Log& m_log;
	asyncio::TransportPtr m_transport;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(asyncio::Log& log): m_log(log) {}

	virtual asyncio::Protocol* CreateProtocol() override {
		return new MyConnection(m_log);
	}

private:
	asyncio::Log& m_log;
};

int main() {
	asyncio::Log my_log([](asyncio::Log::LogLevel lv, const char* msg){
		if (lv > log_level)
			return;

		std::string time_now = asyncio::util::Time::FormatDateTime(std::chrono::system_clock::now());
		switch (lv) {
		case kError:
			printf("%s Error: %s\n", time_now.c_str(), msg);
			break;
		case kWarning:
			printf("%s Warning: %s\n", time_now.c_str(), msg);
			break;
		case kInfo:
			printf("%s Info: %s\n", time_now.c_str(), msg);
			break;
		case kDebug:
			printf("%s Debug: %s\n", time_now.c_str(), msg);
			break;
		default:
			break;
		}
	}, asyncio::Log::kDebug);

	asyncio::EventLoop my_event_loop(my_log);
	MyConnectionFactory my_conn_factory(my_log);
	my_event_loop.CreateConnection(my_conn_factory, "127.0.0.1", 9000);
	my_event_loop.RunForever();
	return 0;
}
