#include "asyncio.h"

class MyConnection : public asyncio::Protocol {
public:
	MyConnection(asyncio::EventLoop& event_loop) 
		: m_event_loop(event_loop)
		, m_codec(std::bind(&MyConnection::OnMyMessageFunc, this, holder1, holder2)) {

	}

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		m_is_connected = true;
	}

	virtual void ConnectionLost(int err_code) override {
		m_is_connected = false;
		m_event_loop.CallLater(3000, [this](){
			m_transport->Reconnect();
		});
	}

	virtual void DataReceived(const char* data, size_t len) override {
		m_codec.Decode(m_transport, data, len);
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
	}

	size_t Send(uint32_t msg_id, const char* data, size_t len) {
		if (!m_is_connected) {
			return 0;
		}

		return m_transport->Write(m_codec.Encode(msg_id, data, len));
	}

	void OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {

	}

	bool IsConnected() {
		return m_is_connected;
	}

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecX m_codec;
	bool m_is_connected = false;
}

class MyConnectionFactory : public asyncio::ProtocolFactory{
public:
	MyConnectionFactory(asyncio::EventLoop& event_loop) : m_event_loop(event_loop) {

	}

	virtual asyncio::Protocol* CreateProtocol() override {
		return new MyConnection(m_event_loop);
	}

private:
	asyncio::EventLoop& m_event_loop;
}

int main() {
	auto my_event_loop = new asyncio::EventLoop();
	my_event_loop->CreateConnection(new MyConnectionFactory(*my_event_loop), "127.0.0.1", 9000);
	my_event_loop->RunForever();
	return 0;
}