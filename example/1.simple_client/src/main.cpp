#include "asyncio.h"

class MyConnection : public asyncio::Protocol, std::enable_shared_from_this<MyConnection> {
public:
	MyConnection() { m_rx_buffer.resize(1024); }
	virtual ~MyConnection() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return std::make_pair(m_rx_buffer.data(), m_rx_buffer.size());
	}

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		ASYNCIO_LOG_DEBUG("ConnectionMade");

		std::string msg("hello,world!");
		Send(msg.data(), msg.size());
	}

	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override {
		m_transport = nullptr;
		ASYNCIO_LOG_DEBUG("ConnectionLost, ec:%d", err_code);
	}

	virtual void DataReceived(size_t len) override {

		//
		// 没有使用解码器，这里会有黏包问题
		// 如果要解决黏包问题需要使用解码器
		//
		ASYNCIO_LOG_DEBUG("DataReceived %lld byte(s): %s", len, m_rx_buffer.data());
	}

	virtual void EofReceived() override {
		ASYNCIO_LOG_DEBUG("EofReceived");
		m_transport->WriteEof();
	}

	size_t Send(const char* data, size_t len) {
		if (m_transport == nullptr)
			return 0;
		ASYNCIO_LOG_DEBUG("Send %lld byte(s): %s", len, data);
		m_transport->Write(data, len);
		return len;
	}

	void Close() {
		if (m_transport != nullptr) {
			m_transport->Close(asyncio::EC_SHUT_DOWN);
		}
	}

private:
	asyncio::TransportPtr m_transport;

	//
	// ProActor模式使用预先分配的缓冲区接收数据
	// 如果缓冲区不够，会分成多次接收
	//
	std::string m_rx_buffer;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	virtual ~MyConnectionFactory() {}
	virtual asyncio::ProtocolPtr CreateProtocol() override { return std::make_shared<MyConnection>(); }
};

int main() {
	asyncio::EventLoop my_event_loop;
	MyConnectionFactory my_conn_factory;
	my_event_loop.CreateConnection(my_conn_factory, "127.0.0.1", 9000);
	my_event_loop.RunForever();
	return 0;
}
