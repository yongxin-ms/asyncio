#include "my_conn.h"
#include "conn_mgr.h"
#include "app.h"

MyConnection::MyConnection(MyConnMgr& owner)
	: m_owner(owner)
	, m_codec(*this ,std::bind(&MyConnection::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2),
		  std::bind(&MyConnection::OnReceivedPong, this)) {}

MyConnection::~MyConnection() {
	ASYNCIO_LOG_DEBUG("MyConnection destroyed");
}

std::pair<char*, size_t> MyConnection::GetRxBuffer() {
	return m_codec.GetRxBuffer();
}

void MyConnection::ConnectionMade(const asyncio::TransportPtr& transport) {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this, transport]() {
		ASYNCIO_LOG_DEBUG("ConnectionMade");
		m_transport = transport;
		m_ping_counter = 0;
		m_ping_timer = g_EventLoop.CallLater(
			30000,
			[self, this]() {
				if (!IsConnected())
					return;

				if (m_ping_counter > 2) {
					ASYNCIO_LOG_WARN("Keep alive failed, Closing");
					Close();
					m_ping_counter = 0;
				} else {
					m_codec.send_ping();
					m_ping_counter++;
				}
			},
			asyncio::DelayTimer::RUN_FOREVER);

		std::string msg("hello,world!");
		Send(0, msg.data(), msg.size());
	});
}

void MyConnection::ConnectionLost(const asyncio::TransportPtr& transport, int err_code) {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this, transport, err_code]() {
		ASYNCIO_LOG_DEBUG("ConnectionLost");
		if (err_code == asyncio::EC_SHUT_DOWN) {
			m_transport = nullptr;
			m_reconnect_timer = nullptr;
			m_ping_timer = nullptr;
		} else {
			m_transport = nullptr;
			m_reconnect_timer = g_EventLoop.CallLater(3000, [transport]() {
				ASYNCIO_LOG_DEBUG("Start Reconnect");
				transport->Connect();
			});
		}
	});
}

void MyConnection::DataReceived(size_t len) {
	m_codec.Decode(len);
}

size_t MyConnection::Write(const asyncio::StringPtr& s) {
	if (m_transport != nullptr) {
		return m_transport->Write(s);
	} else {
		return 0;
	}
}

void MyConnection::Close() {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this]() {
		if (m_transport != nullptr) {
			m_transport->Close();
		}
	});
}

size_t MyConnection::Send(uint32_t msg_id, const char* data, size_t len) {
	auto ret = m_codec.Encode(msg_id, data, len);
	Write(ret);
	return ret->size();
}

void MyConnection::Disconnect() {
	Close();
}

void MyConnection::OnMyMessageFunc(uint32_t msg_id, const std::shared_ptr<std::string>& data) {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this, msg_id, data]() {
		m_owner.OnMessage(self, msg_id, data);
	});
}

void MyConnection::OnReceivedPong() {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this]() {
		m_ping_counter = 0;
	});
}

bool MyConnection::IsConnected() {
	return m_transport != nullptr;
}
