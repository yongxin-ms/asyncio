﻿#include <unordered_map>
#include "asyncio.h"

int g_cur_qps = 0;
std::shared_ptr<asyncio::TimerWrap> g_timer = nullptr;

class MySessionMgr;

class MySession : public asyncio::Protocol, std::enable_shared_from_this<MySession> {
public:
	MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
		: m_owner(owner)
		, m_event_loop(event_loop)
		, m_codec(std::bind(&MySession::OnMyMessageFunc, this, std::placeholders::_1))
		, m_sid(sid) {}
	virtual ~MySession() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override { return m_codec.GetRxBuffer(); }
	virtual void ConnectionMade(asyncio::TransportPtr transport) override;
	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override;
	virtual void DataReceived(size_t len) override { m_codec.Decode(m_transport, len); }

	virtual void EofReceived() override { m_transport->WriteEof(); }

	uint64_t GetSid() { return m_sid; }

	size_t Send(const char* data, size_t len) {
		auto ret = m_codec.Encode(data, len);
		m_transport->Write(ret);
		return ret->size();
	}

	void OnMyMessageFunc(std::shared_ptr<std::string> data) {
		Send(data->data(), data->size());
		g_cur_qps++;
	}

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecLen m_codec;
	const uint64_t m_sid;
};

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(MySessionMgr& owner, asyncio::EventLoop& event_loop)
		: m_owner(owner)
		, m_event_loop(event_loop) {}

	virtual asyncio::ProtocolPtr CreateProtocol() override {
		static uint64_t g_sid = 0;
		uint64_t sid = ++g_sid;
		return std::make_shared<MySession>(m_owner, m_event_loop, sid);
	}

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
};

class MySessionMgr {
public:
	MySessionMgr(asyncio::EventLoop& event_loop)
		: m_session_factory(*this, event_loop) {}

	MySessionFactory& GetSessionFactory() { return m_session_factory; }

	void OnSessionCreate(MySession* session) { m_sessions[session->GetSid()] = session; }

	void OnSessionDestroy(MySession* session) {
		m_sessions.erase(session->GetSid());
		delete session;
	}

	MySession* FindSessionFromSid(uint64_t sid) {
		auto it = m_sessions.find(sid);
		if (it == m_sessions.end()) {
			return nullptr;
		}
		return it->second;
	}

private:
	MySessionFactory m_session_factory;
	std::unordered_map<uint64_t, MySession*> m_sessions;
};

void MySession::ConnectionMade(asyncio::TransportPtr transport) {
	m_transport = transport;
	m_owner.OnSessionCreate(this);
}

void MySession::ConnectionLost(asyncio::TransportPtr transport, int err_code) {
	m_transport = nullptr;
	m_owner.OnSessionDestroy(this);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("ip cli_num\n");
		return 0;
	}

	int port = std::atoi(argv[1]);
	asyncio::EventLoop my_event_loop;
	MySessionMgr my_session_mgr(my_event_loop);
	my_event_loop.CreateServer(my_session_mgr.GetSessionFactory(), port);
	g_timer = my_event_loop.CallLater(1000, []() {
		ASYNCIO_LOG_DEBUG("Cur qps:%d", g_cur_qps);
		g_cur_qps = 0;
		g_timer->Start();
	});

	my_event_loop.RunForever();
	return 0;
}
