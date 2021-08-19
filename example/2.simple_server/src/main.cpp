#include "asyncio.h"

class MySession : public asyncio::Protocol, std::enable_shared_from_this<MySession> {
public:
	MySession() { m_rx_buffer.resize(1024); }
	virtual ~MySession() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return std::make_pair(&m_rx_buffer[0], m_rx_buffer.size());
	}

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		ASYNCIO_LOG_DEBUG("ConnectionMade");
	}

	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override {
		m_transport = nullptr;
		ASYNCIO_LOG_DEBUG("ConnectionLost, ec:%d", err_code);
	}

	virtual void DataReceived(size_t len) override {

		//
		// 没有使用解码器，这里会有黏包问题，如果要解决黏包问题需要使用解码器
		//
		ASYNCIO_LOG_DEBUG("DataReceived %lld byte(s): %s", len, m_rx_buffer.data());

		std::string ack("Your words: ");
		ack.append(m_rx_buffer.data(), len);
		Send(ack.data(), ack.size());
	}

	virtual void EofReceived() override {
		ASYNCIO_LOG_DEBUG("EofReceived");
		m_transport->WriteEof();
	}

	size_t Send(const char* data, size_t len) {
		if (m_transport == nullptr) {
			return 0;
		}

		ASYNCIO_LOG_DEBUG("Send %lld byte(s): %s", len, data);
		m_transport->Write(data, len);
		return len;
	}

	void Close() {
		if (m_transport != nullptr) {
			m_transport->Close(asyncio::EC_KICK);
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

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	virtual ~MySessionFactory() {}
	virtual asyncio::ProtocolPtr CreateProtocol() override { return std::make_shared<MySession>(); }
};

int main() {
	int port = 9000;
	asyncio::EventLoop my_event_loop;
	MySessionFactory my_session_factory;
	auto listener = my_event_loop.CreateServer(my_session_factory, port);
	if (listener == nullptr) {
		ASYNCIO_LOG_ERROR("listen on %d failed", port);
		return 0;
	}

	ASYNCIO_LOG_INFO("listen on %d suc", port);
	my_event_loop.RunForever();
	return 0;
}
