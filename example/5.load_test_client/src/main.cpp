#include <functional>
#include "asyncio.h"

class MyConnection : public asyncio::Protocol {
public:
	MyConnection(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop)
		, m_codec(std::bind(&MyConnection::OnMyMessageFunc, this, std::placeholders::_1)) {
	}

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		m_is_connected = true;
	}

	virtual void ConnectionLost(int err_code) override {
		m_is_connected = false;
		m_event_loop.CallLater(3000, [this]() { m_transport->Reconnect(); });
	}

	virtual void DataReceived(const char* data, size_t len) override {
		m_codec.Decode(m_transport, data, len);
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
	}

	size_t Send(const char* data, size_t len) {
		if (!m_is_connected) {
			return 0;
		}

		auto ret = m_codec.Encode(data, len);
		m_transport->Write(ret->data(), ret->size());
		return ret->size();
	}

	void OnMyMessageFunc(std::shared_ptr<std::string> data) {
	}

	bool IsConnected() {
		return m_is_connected;
	}

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecLen m_codec;
	bool m_is_connected = false;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {
	}

	virtual asyncio::Protocol* CreateProtocol() override {
		return new MyConnection(m_event_loop);
	}

private:
	asyncio::EventLoop& m_event_loop;
};

int main() {
	asyncio::EventLoop my_event_loop;
	MyConnectionFactory my_conn_factory(my_event_loop);
	my_event_loop.CreateConnection(my_conn_factory, "127.0.0.1", 9000);
	my_event_loop.RunForever();
	return 0;
}
