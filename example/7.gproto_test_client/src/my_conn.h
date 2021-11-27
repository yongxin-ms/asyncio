#pragma once
#include <asyncio.h>
#include "codec/codec_gproto.h"

class MyConnMgr;

class MyConnection
	: public std::enable_shared_from_this<MyConnection>
	, public asyncio::Protocol {
public:
	MyConnection(MyConnMgr& owner, asyncio::EventLoop& event_loop);
	virtual ~MyConnection();

	virtual std::pair<char*, size_t> GetRxBuffer() override;
	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override;
	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override;
	virtual void DataReceived(size_t len) override;
	virtual void EofReceived() override;
	virtual void Release() override;

	size_t Send(uint32_t msg_id, const char* data, size_t len);
	void Close();
	void OnMyMessageFunc(uint32_t msg_id, const std::shared_ptr<std::string>& data);
	void OnReceivedPong();
	bool IsConnected();

private:
	MyConnMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecGProto m_codec;
	asyncio::DelayTimerPtr m_reconnect_timer;
	asyncio::DelayTimerPtr m_ping_timer;
	int m_ping_counter = 0;
	bool m_connected = false;
};

using MyConnectionPtr = std::shared_ptr<MyConnection>;
