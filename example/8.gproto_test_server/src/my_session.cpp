#include "my_session.h"
#include "session_mgr.h"

MySession::MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
	: m_owner(owner)
	, m_event_loop(event_loop)
	, m_codec(std::bind(&MySession::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2),
			  std::bind(&MySession::OnReceivedPong, this))
	, m_sid(sid) {}

std::pair<char*, size_t> MySession::GetRxBuffer() {
	return m_codec.GetRxBuffer();
}

void MySession::ConnectionMade(asyncio::TransportPtr transport) {
	ASYNCIO_LOG_DEBUG("ConnectionMade sid:%llu", GetSid());
	m_codec.Init(transport);
	m_transport = transport;

	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, transport]() {
		m_ping_counter = 0;
		m_ping_timer = m_event_loop.CallLater(
			30000,
			[self, this]() {
				if (m_ping_counter > 2) {
					ASYNCIO_LOG_WARN("Keep alive failed Sid:%llu, Closing", GetSid());
					m_transport->Close(asyncio::EC_KEEP_ALIVE_FAIL);
					m_ping_counter = 0;
				} else {
					m_codec.send_ping();
					m_ping_counter++;
				}
			},
			asyncio::DelayTimer::RUN_FOREVER);
		self->m_owner.OnSessionCreate(self);
	});
}

void MySession::ConnectionLost(asyncio::TransportPtr transport, int err_code) {
	ASYNCIO_LOG_DEBUG("ConnectionLost sid:%llu", GetSid());

	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this]() {
		m_owner.OnSessionDestroy(self);
	});
}

void MySession::DataReceived(size_t len) {
	m_codec.Decode(len);
}

void MySession::EofReceived() {
	ASYNCIO_LOG_DEBUG("EofReceived");
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this]() {
		m_transport->WriteEof();
	});
}

size_t MySession::Send(uint32_t msg_id, const char* data, size_t len) {
	auto ret = m_codec.Encode(msg_id, data, len);
	m_transport->Write(ret);
	return ret->size();
}

void MySession::Kick() {
	m_transport->Close(asyncio::EC_KICK);
}

void MySession::OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, msg_id, data]() { m_owner.OnMessage(self, msg_id, data); });
}

void MySession::OnReceivedPong() {
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this]() { m_ping_counter = 0; });
}
