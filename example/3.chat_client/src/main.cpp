#include <functional>
#include "asyncio.h"

class MyConnection : public asyncio::Protocol {
public:
	MyConnection(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop)
		, m_codec(std::bind(&MyConnection::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2)) {}

	virtual std::pair<char*, size_t> GetRxBuffer() override { return m_codec.GetRxBuffer(); }

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		m_is_connected = true;
		ASYNCIO_LOG_DEBUG("ConnectionMade");

		std::string msg("hello,world!");
		Send(0, msg.data(), msg.size());
	}

	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override {
		m_is_connected = false;
		m_event_loop.CallLater(3000, [transport]() {
			ASYNCIO_LOG_DEBUG("Start Reconnect");
			transport->Reconnect();
		});
		
		ASYNCIO_LOG_DEBUG("ConnectionLost");
	}

	virtual void DataReceived(size_t len) override {
		m_codec.Decode(m_transport, len);
	}

	virtual void EofReceived() override { m_transport->WriteEof(); }

	size_t Send(uint32_t msg_id, const char* data, size_t len) {
		if (!m_is_connected) {
			return 0;
		}

		auto ret = m_codec.Encode(msg_id, data, len);
		m_transport->Write(ret);
		return ret->size();
	}

	void OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {
		ASYNCIO_LOG_DEBUG("OnMyMessageFunc: %s", data->data());
	}

	bool IsConnected() { return m_is_connected; }

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecX m_codec;
	bool m_is_connected = false;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {}

	virtual asyncio::Protocol* CreateProtocol() override { return new MyConnection(m_event_loop); }

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
