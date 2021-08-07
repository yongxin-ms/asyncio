#include "asyncio.h"

class MySession : public asyncio::Protocol {
public:
	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
	}

	virtual void ConnectionLost(int err_code) override {
		m_transport = nullptr;
	}

	virtual void DataReceived(const char* data, size_t len) override {
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
	}

	size_t Send(const char* data, size_t len) {
		if (m_transport == nullptr) {
			return 0;
		}

		m_transport->Write(data, len);
		return len;
	}

private:
	asyncio::TransportPtr m_transport;
};

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	virtual asyncio::Protocol* CreateProtocol() override {
		return new MySession();
	}
};

int main() {
	asyncio::EventLoop my_event_loop;
	MySessionFactory my_session_factory;
	my_event_loop.CreateServer(my_session_factory, 9000);
	my_event_loop.RunForever();
	return 0;
}
