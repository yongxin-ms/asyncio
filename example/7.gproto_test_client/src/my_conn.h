#pragma once
#include "asyncio.h"

class MyConnMgr;

class MyConnection
	: public std::enable_shared_from_this<MyConnection>
	, public asyncio::Protocol {
public:
	MyConnection(MyConnMgr& owner, asyncio::EventLoop& event_loop);
	virtual ~MyConnection() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override;
	virtual void ConnectionMade(asyncio::TransportPtr transport) override;
	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override;
	virtual void DataReceived(size_t len) override;
	virtual void EofReceived() override;

	size_t Send(uint32_t msg_id, const char* data, size_t len);
	void Close();
	void OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data);
	bool IsConnected();

private:
	MyConnMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecGProto m_codec;
};

using MyConnectionPtr = std::shared_ptr<MyConnection>;
