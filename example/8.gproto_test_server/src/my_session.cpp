#include "my_session.h"
#include "session_mgr.h"
#include "app.h"

MySession::MySession(MySessionMgr& owner, uint64_t sid)
	: m_owner(owner)
	, m_codec(*this, std::bind(&MySession::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2),
		  std::bind(&MySession::OnReceivedPong, this))
	, m_sid(sid) {}

MySession::~MySession() {
	ASYNCIO_LOG_DEBUG("MySession:%llu destroyed", m_sid);
}

std::pair<char*, size_t> MySession::GetRxBuffer() {
	return m_codec.GetRxBuffer();
}

void MySession::ConnectionMade(const asyncio::TransportPtr& transport) {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this, transport]() {
		ASYNCIO_LOG_DEBUG("ConnectionMade sid:%llu", GetSid());
		m_transport = transport;
		m_ping_counter = 0;
		m_ping_timer = g_EventLoop.CallLater(
			30000,
			[self, this]() {
				if (m_ping_counter > 2) {
					ASYNCIO_LOG_WARN("Keep alive failed Sid:%llu, Closing", GetSid());
					Close();
					m_ping_counter = 0;
				} else {
					m_codec.send_ping();
					m_ping_counter++;
				}
			},
			asyncio::DelayTimer::RUN_FOREVER);
		m_owner.OnSessionCreate(self);
	});
}

void MySession::ConnectionLost(const asyncio::TransportPtr& transport, int err_code) {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this]() {
		ASYNCIO_LOG_DEBUG("ConnectionLost sid:%llu", GetSid());
		m_owner.OnSessionDestroy(self);
	});
}

void MySession::DataReceived(size_t len) {
	m_codec.Decode(len);
}

size_t MySession::Write(const asyncio::StringPtr& s) {
	if (m_transport != nullptr) {
		return m_transport->Write(s);
	} else {
		return 0;
	}
}

void MySession::Close() {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this]() {
		if (m_transport != nullptr) {
			m_transport->Close();
		}
	});
}

size_t MySession::Send(uint32_t msg_id, const char* data, size_t len) {
	auto ret = m_codec.Encode(msg_id, data, len);
	return Write(ret);
}

void MySession::OnMyMessageFunc(uint32_t msg_id, const std::shared_ptr<std::string>& data) {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this, msg_id, data]() {
		m_owner.OnMessage(self, msg_id, data);
	});
}

void MySession::OnReceivedPong() {
	auto self = shared_from_this();
	g_EventLoop.QueueInLoop([self, this]() {
		m_ping_counter = 0;
	});
}
