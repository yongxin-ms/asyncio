#include "asyncio.h"

class MySession : public asyncio::Protocol {
public:
	virtual void ConnectionMade(TransportPtr transport) override {
		m_transport = transport;
	}

	virtual void ConnectionLost(int err_code) override {
		m_transport = null_ptr;
	}

	virtual void DataReceived(const char* data, size_t len) override {
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
	}

	size_t Send(const char* data, size_t len) {
		if (m_transport == null_ptr) {
			return 0;
		}

		return m_transport->Write(data, len);
	}

private:
	asyncio::TransportPtr m_transport;
}

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	virtual Protocol* CreateProtocol() override {
		return new MySession();
	}
}

int main() {
	auto my_event_loop = new asyncio::EventLoop();
	my_event_loop->CreateServer(new MySessionFactory(), 9000);
	my_event_loop->RunForever();
	return 0;
}
