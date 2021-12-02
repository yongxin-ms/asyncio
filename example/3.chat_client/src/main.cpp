#include <functional>
#include <asyncio.h>

class MyConnection
	: public std::enable_shared_from_this<MyConnection>
	, public asyncio::Protocol {
public:
	MyConnection(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {
		m_rx_buffer.resize(1024);
	}

	~MyConnection() {
		ASYNCIO_LOG_INFO("MyConnection destroyed");
	}

	bool IsConnected() {
		return m_transport != nullptr;
	}

private:
	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return std::make_pair(&m_rx_buffer[0], m_rx_buffer.size());
	}

	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this, transport]() {
			ASYNCIO_LOG_INFO("ConnectionMade");
			m_transport = transport;

			//
			// 连接建立之后每2秒钟发送一条消息
			//

			m_say_timer = m_event_loop.CallLater(
				2000,
				[self, this]() {
					if (!IsConnected())
						return;
					auto msg = std::make_shared<std::string>("hello,world!");
					Write(msg);
				},
				asyncio::DelayTimer::RUN_FOREVER);
		});
	}

	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this, transport, err_code]() {
			ASYNCIO_LOG_INFO("ConnectionLost");

			if (err_code == asyncio::EC_SHUT_DOWN) {
				m_transport = nullptr;
				m_reconnect_timer = nullptr;
				m_say_timer = nullptr;
			} else {
				m_transport = nullptr;

				//
				// 网络断开之后每3秒钟尝试一次重连，只到连上为止
				//
				m_reconnect_timer = m_event_loop.CallLater(3000, [transport]() {
					ASYNCIO_LOG_INFO("Start Reconnect");
					transport->Connect();
				});

				//
				// 连接断开之后停止发送消息
				//
				m_say_timer = nullptr;
			}
		});
	}

	virtual void DataReceived(size_t len) override {
		std::string content(m_rx_buffer.data(), len);
		ASYNCIO_LOG_INFO("%s", content.data());
	}

	virtual size_t Write(const asyncio::StringPtr& s) override {
		if (m_transport != nullptr) {
			return m_transport->Write(s);
		} else {
			return 0;
		}
	}

	virtual void Close() override {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this]() {
			if (m_transport != nullptr) {
				m_transport->Close();
			}
		});
	}

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;

	//
	// ProActor模式使用预先分配的缓冲区接收数据
	// 如果缓冲区不够，会分成多次接收
	//
	std::string m_rx_buffer;
	asyncio::DelayTimerPtr m_say_timer;
	asyncio::DelayTimerPtr m_reconnect_timer;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {}

	virtual asyncio::ProtocolPtr CreateProtocol() override {
		return std::make_shared<MyConnection>(m_event_loop);
	}

private:
	asyncio::EventLoop& m_event_loop;
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
	MyConnectionFactory my_conn_factory(my_event_loop);
	auto conn = my_event_loop.CreateConnection(my_conn_factory, "127.0.0.1", 9000);
	my_event_loop.RunForever();
	return 0;
}
