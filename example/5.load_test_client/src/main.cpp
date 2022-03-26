#include <functional>
#include <asyncio.h>

int g_cur_qps = 0;

class MyConnection
	: public std::enable_shared_from_this<MyConnection>
	, public asyncio::Protocol {
public:
	explicit MyConnection(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop)
		, m_codec(*this, std::bind(&MyConnection::OnMyMessageFunc, this, std::placeholders::_1), 2 * 1024) {}

	~MyConnection() {
		ASYNCIO_LOG_DEBUG("MyConnection destroyed");
	}

	size_t Send(const char* data, size_t len) {
		auto ret = m_codec.Encode(data, len);
		return Write(ret);
	}

	bool IsConnected() {
		return m_transport != nullptr;
	}

	void SendHello() {
		static std::string msg("hello,world!");
		Send(msg.data(), msg.size());
	}

private:
	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return m_codec.GetRxBuffer();
	}

	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override {
		transport->SetNoDelay(false);
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this, transport]() {
			m_transport = transport;

			SendHello();
		});
	}

	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this, transport, err_code]() {
			if (err_code == asyncio::EC_SHUT_DOWN) {
				m_transport = nullptr;
				m_reconnect_timer = nullptr;
			} else {
				m_transport = nullptr;
				m_reconnect_timer = m_event_loop.CallLater(std::chrono::seconds(3), [transport]() {
					ASYNCIO_LOG_DEBUG("Start Reconnect");
					transport->Connect();
				});
			}
		});
	}

	virtual void DataReceived(size_t len) override {
		m_codec.Decode(len);
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

	void OnMyMessageFunc(const std::shared_ptr<std::string>& data) {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, this]() {
			SendHello();
			g_cur_qps++;
		});
	}

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecLen m_codec;
	asyncio::DelayTimerPtr m_reconnect_timer;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	explicit MyConnectionFactory(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {}

	virtual asyncio::ProtocolPtr CreateProtocol() override {
		return std::make_shared<MyConnection>(m_event_loop);
	}

private:
	asyncio::EventLoop& m_event_loop;
};

int main(int argc, char* argv[]) {
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

	if (argc < 4) {
		printf("ip port cli_num\n");
		return 0;
	}

	std::string ip = argv[1];
	int port = std::atoi(argv[2]);
	int cli_num = std::atoi(argv[3]);

	asyncio::EventLoop my_event_loop;
	MyConnectionFactory my_conn_factory(my_event_loop);
	std::vector<asyncio::ProtocolPtr> conns;
	for (int i = 0; i < cli_num; i++) {
		conns.push_back(my_event_loop.CreateConnection(my_conn_factory, ip, port));
	}

	auto qps_timer = my_event_loop.CallLater(
		std::chrono::seconds(1),
		[]() {
			if (g_cur_qps != 0) {
				ASYNCIO_LOG_DEBUG("Cur qps:%d", g_cur_qps);
				g_cur_qps = 0;
			}
		},
		asyncio::RUN_FOREVER);

	my_event_loop.RunForever();
	return 0;
}
