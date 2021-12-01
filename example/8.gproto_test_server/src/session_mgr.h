#pragma once
#include <unordered_map>
#include "my_session.h"

class MySessionMgr;
namespace id_worker {
class IdWorker;
}

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(MySessionMgr& owner);
	virtual asyncio::ProtocolPtr CreateProtocol() override;

private:
	MySessionMgr& m_owner;
};

class MySessionMgr {
public:
	MySessionMgr();

	bool Init(uint16_t port);

	void OnSessionCreate(const MySessionPtr& session);
	void OnSessionDestroy(const MySessionPtr& session);
	void OnMessage(const MySessionPtr& conn, uint32_t msg_id, const std::shared_ptr<std::string>& data);

	MySessionPtr FindSessionFromSid(uint64_t sid);
	size_t size() const;

private:
	MySessionFactory m_session_factory;
	std::unordered_map<uint64_t, MySessionPtr> m_sessions;
	asyncio::ListenerPtr m_listener;
};
