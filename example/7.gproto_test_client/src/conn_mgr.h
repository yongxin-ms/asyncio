#pragma once
#include "my_conn.h"

class MyConnMgr;

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(MyConnMgr& owner);
	virtual asyncio::ProtocolPtr CreateProtocol() override;

private:
	MyConnMgr& m_owner;
};

class MyConnMgr {
public:
	MyConnMgr();

	void StartConnect(const std::string& ip, uint16_t port, int conn_count);
	void OnMessage(const MyConnectionPtr& conn, uint32_t msg_id, const std::shared_ptr<std::string>& data);

private:
	MyConnectionFactory m_conn_factory;
	std::vector<MyConnectionPtr> m_conns;
};
