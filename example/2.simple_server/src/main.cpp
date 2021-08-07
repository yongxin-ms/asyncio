#include "asyncio.h"

class MySession : public asyncio::Protocol {
public:
	MySession(asyncio::Log& log) : m_log(log) {}

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		LOG_INFO("ConnectionMade");
	}

	virtual void ConnectionLost(int err_code) override {
		m_transport = nullptr;
		LOG_INFO("ConnectionLost");
	}

	virtual void DataReceived(const char* data, size_t len) override {
		LOG_INFO("DataReceived: %s", data);
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
		LOG_INFO("EofReceived");
	}

	size_t Send(const char* data, size_t len) {
		if (m_transport == nullptr) {
			return 0;
		}

		LOG_INFO("Send: %s", data);
		m_transport->Write(data, len);
		return len;
	}

private:
	asyncio::Log& m_log;
	asyncio::TransportPtr m_transport;
};

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(asyncio::Log& log) : m_log(log) {}

	virtual asyncio::Protocol* CreateProtocol() override {
		return new MySession(m_log);
	}

private:
	asyncio::Log& m_log;
};

int main() {
	asyncio::Log my_log([](asyncio::Log::LogLevel lv, const char* msg) {
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
	MySessionFactory my_session_factory(my_log);
	my_event_loop.CreateServer(my_session_factory, 9000);
	my_event_loop.RunForever();
	return 0;
}
