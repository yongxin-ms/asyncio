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
		
	}

private:
	TransportPtr m_transport;
}

class MyMessionFactory : public asyncio::ProtocolFactory{
public:
	virtual Protocol* CreateProtocol() override {
		return new MySession();
	}
}

int main() {
	auto my_event_loop = new asyncio::EventLoop();
	my_event_loop->CreateServer(new MyMessionFactory(), 9000);
	my_event_loop->RunForever();
	return 0;
}