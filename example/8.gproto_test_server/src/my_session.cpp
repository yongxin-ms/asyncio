#include "my_session.h"
#include "session_mgr.h"

MySession::MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
	: m_owner(owner)
	, m_event_loop(event_loop)
	, m_codec(std::bind(&MySession::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2))
	, m_sid(sid) {}

std::pair<char*, size_t> MySession::GetRxBuffer() {
	return m_codec.GetRxBuffer();
}

void MySession::ConnectionMade(asyncio::TransportPtr transport) {
	m_transport = transport;
	ASYNCIO_LOG_DEBUG("ConnectionMade sid:%llu", GetSid());

	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self]() { self->m_owner.OnSessionCreate(self); });
}

void MySession::ConnectionLost(asyncio::TransportPtr transport, int err_code) {
	m_transport = nullptr;
	ASYNCIO_LOG_DEBUG("ConnectionLost sid:%llu", GetSid());

	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self]() { self->m_owner.OnSessionDestroy(self); });
}

void MySession::DataReceived(size_t len) {
	m_codec.Decode(m_transport, len);
}

void MySession::EofReceived() {
	m_transport->WriteEof();
}

size_t MySession::Send(uint32_t msg_id, const char* data, size_t len) {
	if (m_transport == nullptr)
		return 0;

	auto ret = m_codec.Encode(msg_id, data, len);
	m_transport->Write(ret);
	return ret->size();
}

void MySession::Close() {
	if (m_transport != nullptr) {
		m_transport->Close(asyncio::EC_KICK);
	}
}

void MySession::OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, msg_id, data]() { m_owner.OnMessage(self, msg_id, data); });
}
