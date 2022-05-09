#pragma once
#include <asyncio.h>
#include "codec/codec_gproto.h"

class MyConnMgr;

class MyConnection
	: public std::enable_shared_from_this<MyConnection>
	, public asyncio::Protocol {
public:
	explicit MyConnection(MyConnMgr& owner);
	virtual ~MyConnection();

	size_t Send(uint32_t msg_id, const char* data, size_t len);
	void Disconnect();
	bool IsConnected();

private:
	virtual std::pair<char*, size_t> GetRxBuffer() override;
	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override;
	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override;
	virtual bool DataReceived(size_t len) override;
	virtual size_t Write(const asyncio::StringPtr& s) override;

	void OnMyMessageFunc(uint32_t msg_id, const std::shared_ptr<std::string>& data);
	void OnReceivedPong();

private:
	MyConnMgr& m_owner;
	asyncio::TransportPtr m_transport;
	asyncio::CodecGProto m_codec;
	asyncio::DelayTimerPtr m_reconnect_timer;
	asyncio::DelayTimerPtr m_ping_timer;
	int m_ping_counter = 0;
};

using MyConnectionPtr = std::shared_ptr<MyConnection>;
