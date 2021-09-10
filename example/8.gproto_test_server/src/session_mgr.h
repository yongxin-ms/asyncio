#pragma once
#include <unordered_map>
#include "my_session.h"

class MySessionMgr;
namespace id_worker {
class IdWorker;
}

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(MySessionMgr& owner, asyncio::EventLoop& event_loop, id_worker::IdWorker& idwork);
	virtual ~MySessionFactory();
	virtual asyncio::ProtocolPtr CreateProtocol() override;

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	id_worker::IdWorker& m_idwork;
};

class MySessionMgr {
public:
	MySessionMgr(asyncio::EventLoop& event_loop, id_worker::IdWorker& idwork);

	bool Init(uint16_t port);

	void OnSessionCreate(MySessionPtr session);
	void OnSessionDestroy(MySessionPtr session);
	void OnMessage(MySessionPtr conn, uint32_t msg_id, std::shared_ptr<std::string> data);

	MySessionPtr FindSessionFromSid(uint64_t sid);
	size_t size() const;

private:
	asyncio::EventLoop& m_event_loop;
	MySessionFactory m_session_factory;
	std::unordered_map<uint64_t, MySessionPtr> m_sessions;
	asyncio::ListenerPtr m_listener;
};
