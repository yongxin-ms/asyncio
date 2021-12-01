#include <asyncio.h>

class MyConnection
	: public asyncio::Protocol
	, std::enable_shared_from_this<MyConnection> {
public:
	MyConnection() {
		m_rx_buffer.resize(1024);
	}

	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return std::make_pair(&m_rx_buffer[0], m_rx_buffer.size());
	}

	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override {
		ASYNCIO_LOG_DEBUG("ConnectionMade");
		m_transport = transport;

		std::string msg("hello,world!");
		Send(msg.data(), msg.size());
	}

	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override {
		ASYNCIO_LOG_DEBUG("ConnectionLost, ec:%d", err_code);
		m_transport = nullptr;
	}

	virtual void DataReceived(size_t len) override {

		//
		// 没有使用解码器，这里会有黏包问题
		// 如果要解决黏包问题需要使用解码器
		//
		ASYNCIO_LOG_DEBUG("DataReceived %lld byte(s): %s", len, m_rx_buffer.data());
	}

	virtual size_t Write(const asyncio::StringPtr& s) override {
		if (m_transport != nullptr) {
			return m_transport->Write(s);
		} else {
			return 0;
		}
	}

	virtual void Close() override {
		if (m_transport != nullptr) {
			m_transport->Close();
		}
	}

	size_t Send(const char* data, size_t len) {
		ASYNCIO_LOG_DEBUG("Send %lld byte(s): %s", len, data);
		auto s = std::make_shared<std::string>(data, len);
		return Write(s);
	}

	bool IsConnected() {
		return m_transport != nullptr;
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
	virtual asyncio::ProtocolPtr CreateProtocol() override {
		return std::make_shared<MyConnection>();
	}
};

int main() {
	asyncio::SetLogHandler(
		[](asyncio::Log::LogLevel lv, const char* msg) {
			std::string time_now = asyncio::util::Time::FormatDateTime(std::chrono::system_clock::now());
			switch (lv) {
			case asyncio::Log::kError:
				printf("%s Error: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kWarning:
				printf("%s Warning: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kInfo:
				printf("%s Info: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kDebug:
				printf("%s Debug: %s\n", time_now.c_str(), msg);
				break;
			default:
				break;
			}
		},
		asyncio::Log::kDebug);

	asyncio::EventLoop my_event_loop;
	MyConnectionFactory my_conn_factory;
	my_event_loop.CreateConnection(my_conn_factory, "127.0.0.1", 9000);
	my_event_loop.RunForever();
	return 0;
}
