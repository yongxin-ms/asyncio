#pragma once
#include <asyncio.h>
#include "codec/codec_gproto.h"

class MySessionMgr;

class MySession
	: public std::enable_shared_from_this<MySession>
	, public asyncio::Protocol {
public:
	MySession(MySessionMgr& owner, uint64_t sid);
	virtual ~MySession();

	uint64_t GetSid() {
		return m_sid;
	}

	size_t Send(uint32_t msg_id, const char* data, size_t len);

private:
	virtual std::pair<char*, size_t> GetRxBuffer() override;
	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override;
	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override;
	virtual void DataReceived(size_t len) override;
	virtual size_t Write(const asyncio::StringPtr& s) override;
	virtual void Close() override;

	void OnMyMessageFunc(uint32_t msg_id, const std::shared_ptr<std::string>& data);
	void OnReceivedPong();

private:
	MySessionMgr& m_owner;
	asyncio::TransportPtr m_transport;
	asyncio::CodecGProto m_codec;
	const uint64_t m_sid;
	asyncio::DelayTimerPtr m_ping_timer;
	int m_ping_counter = 0;
};

using MySessionPtr = std::shared_ptr<MySession>;
