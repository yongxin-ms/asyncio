#include "asyncio.h"

class MyConnection : public asyncio::Protocol {
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
	CodecX m_codec;
}

class MyConnectionFactory : public asyncio::ProtocolFactory{
public:
	virtual Protocol* CreateProtocol() override {
		return new MyConnection();
	}
}

int main() {
	auto my_event_loop = new asyncio::EventLoop();
	my_event_loop->CreateConnection(new MyConnectionFactory(), "127.0.0.1", 9000);
	my_event_loop->RunForever();
	return 0;
}