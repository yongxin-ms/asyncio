#include <memory>
#include "session_mgr.h"
#include "my_session.h"
#include "id_worker.h"
#include "app.h"

MySessionFactory::MySessionFactory(MySessionMgr& owner, asyncio::EventLoop& event_loop, id_worker::IdWorker& idwork)
	: m_owner(owner)
	, m_event_loop(event_loop)
	, m_idwork(idwork) {}

asyncio::ProtocolPtr MySessionFactory::CreateProtocol() {
	return std::make_shared<MySession>(m_owner, m_event_loop, m_idwork.CreateId());
}

MySessionMgr::MySessionMgr(asyncio::EventLoop& event_loop, id_worker::IdWorker& idwork)
	: m_event_loop(event_loop)
	, m_session_factory(*this, event_loop, idwork) {}

bool MySessionMgr::Init(uint16_t port) {
	m_listener = m_event_loop.CreateServer(m_session_factory, port);
	if (m_listener == nullptr) {
		ASYNCIO_LOG_ERROR("listen on %d failed", port);
		return false;
	}

	ASYNCIO_LOG_INFO("listen on %d suc", port);
	return true;
}

void MySessionMgr::OnSessionCreate(const MySessionPtr& session) {
	m_sessions[session->GetSid()] = session;
}
void MySessionMgr::OnSessionDestroy(const MySessionPtr& session) {
	m_sessions.erase(session->GetSid());
}

MySessionPtr MySessionMgr::FindSessionFromSid(uint64_t sid) {
	auto it = m_sessions.find(sid);
	if (it == m_sessions.end()) {
		return nullptr;
	}
	return it->second;
}

size_t MySessionMgr::size() const {
	return m_sessions.size();
}

void MySessionMgr::OnMessage(const MySessionPtr& conn, uint32_t msg_id, const std::shared_ptr<std::string>& data) {
	conn->Send(msg_id, data->data(), data->size());
	App::Instance()->IncQps();
}
