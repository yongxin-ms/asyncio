#include <functional>
#include "asyncio.h"

class MyConnection : public std::enable_shared_from_this<MyConnection>, public asyncio::Protocol {
public:
	MyConnection(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop)
		, m_codec(std::bind(&MyConnection::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2)) {}
	virtual ~MyConnection() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override { return m_codec.GetRxBuffer(); }

	virtual void ConnectionMade(asyncio::TransportPtr transport) override {
		m_transport = transport;
		ASYNCIO_LOG_DEBUG("ConnectionMade");

		//
		// 连接建立之后每2秒钟发送一条消息
		//
		auto self = shared_from_this();
		m_say_timer = m_event_loop.CallLater(2000, [self]() {
			std::string msg("hello,world!");
			self->Send(0, msg.data(), msg.size());

			if (self->m_say_timer != nullptr) {
				self->m_say_timer->Start();
			}
		});
	}

	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override {
		m_transport = nullptr;

		//
		// 网络断开之后每3秒钟尝试一次重连，只到连上为止
		//
		m_event_loop.CallLater(3000, [transport]() {
			ASYNCIO_LOG_DEBUG("Start Reconnect");
			transport->Connect();
		});

		//
		// 连接断开之后停止发送消息
		//
		if (m_say_timer != nullptr) {
			m_say_timer = nullptr;
		}
		
		ASYNCIO_LOG_DEBUG("ConnectionLost");
	}

	virtual void DataReceived(size_t len) override {
		m_codec.Decode(m_transport, len);
	}

	virtual void EofReceived() override { m_transport->WriteEof(); }

	size_t Send(uint32_t msg_id, const char* data, size_t len) {
		if (!IsConnected()) {
			return 0;
		}

		auto ret = m_codec.Encode(msg_id, data, len);
		m_transport->Write(ret);
		return ret->size();
	}

	void OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {
		ASYNCIO_LOG_DEBUG("OnMyMessageFunc: %s", data->data());
	}

	bool IsConnected() { return m_transport != nullptr; }

private:
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;

	//
	// 使用带消息id的解码器，解决了黏包问题
	// 还可以使用较小的缓冲区接收大包
	//
	asyncio::CodecX m_codec;
	std::shared_ptr<asyncio::DelayTimer> m_say_timer;
};

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {}
	virtual ~MyConnectionFactory() {}
	virtual asyncio::ProtocolPtr CreateProtocol() override { return std::make_shared<MyConnection>(m_event_loop); }

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
