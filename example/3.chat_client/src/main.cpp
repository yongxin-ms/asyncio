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

	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return std::make_pair(&m_rx_buffer[0], m_rx_buffer.size());
	}

	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override {
		ASYNCIO_LOG_INFO("ConnectionMade");
		m_transport = transport;

		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this, transport]() {
			//
			// 连接建立之后每2秒钟发送一条消息
			//

			m_connected = true;
			if (m_say_timer == nullptr) {
				auto weak_self = self->weak_from_this();
				m_say_timer = m_event_loop.CallLater(
					2000,
					[weak_self]() {
						auto self = weak_self.lock();
						if (self == nullptr)
							return;
						auto msg = std::make_shared<std::string>("hello,world!");
						self->Send(msg);
					},
					asyncio::DelayTimer::RUN_FOREVER);
			} else {
				m_say_timer->Run(asyncio::DelayTimer::RUN_FOREVER);
			}
		});
	}

	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this, transport]() {
			ASYNCIO_LOG_INFO("ConnectionLost");

			//
			// 网络断开之后每3秒钟尝试一次重连，只到连上为止
			//
			auto weak_transport = transport->weak_from_this();
			m_reconnect_timer = m_event_loop.CallLater(3000, [weak_transport]() {
				auto transport = weak_transport.lock();
				if (transport == nullptr)
					return;
				ASYNCIO_LOG_INFO("Start Reconnect");
				transport->Connect();
			});

			//
			// 连接断开之后停止发送消息
			//
			if (m_say_timer != nullptr) {
				m_say_timer->Cancel();
			}

			m_connected = false;
		});
	}

	virtual void DataReceived(size_t len) override {
		std::string content(m_rx_buffer.data(), len);
		ASYNCIO_LOG_INFO("%s", content.data());
	}

	virtual void EofReceived() override {
		ASYNCIO_LOG_DEBUG("EofReceived");
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this]() { m_transport->WriteEof(); });
	}

	size_t Send(const asyncio::StringPtr& data) {
		if (!IsConnected())
			return 0;
		m_transport->Write(data);
		return data->size();
	}

	void Close() {
		if (IsConnected())
			m_transport->Close(asyncio::EC_SHUT_DOWN);
	}

	bool IsConnected() { return m_connected; }

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	bool m_connected = false;

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
	virtual asyncio::ProtocolPtr CreateProtocol() override { return std::make_shared<MyConnection>(m_event_loop); }

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
	my_event_loop.CreateConnection(my_conn_factory, "127.0.0.1", 9000);
	my_event_loop.RunForever();
	return 0;
}
