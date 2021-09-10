#pragma once
#include <asyncio.h>
#include "my_conn.h"

class MyConnMgr;

class MyConnectionFactory : public asyncio::ProtocolFactory {
public:
	MyConnectionFactory(MyConnMgr& owner, asyncio::EventLoop& event_loop);
	virtual ~MyConnectionFactory();
	virtual asyncio::ProtocolPtr CreateProtocol() override;

private:
	MyConnMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
};

class MyConnMgr {
public:
	MyConnMgr(asyncio::EventLoop& event_loop);

	void StartConnect(const std::string& ip, uint16_t port, int conn_count);
	void OnMessage(MyConnectionPtr conn, uint32_t msg_id, std::shared_ptr<std::string> data);

private:
	asyncio::EventLoop& m_event_loop;
	MyConnectionFactory m_conn_factory;
	std::vector<MyConnectionPtr> m_conns;
};
