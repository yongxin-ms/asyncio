#include <functional>
#include <unordered_map>
#include "asyncio.h"

class MySessionMgr;

class MySession : public std::enable_shared_from_this<MySession>, public asyncio::Protocol{
public:
	MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
		: m_owner(owner)
		, m_event_loop(event_loop)
		, m_codec(std::bind(&MySession::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2))
		, m_sid(sid) {}
	virtual ~MySession() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override { return m_codec.GetRxBuffer(); }
	virtual void ConnectionMade(asyncio::TransportPtr transport) override;
	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override;
	virtual void DataReceived(size_t len) override { m_codec.Decode(m_transport, len); }
	virtual void EofReceived() override { 
		ASYNCIO_LOG_DEBUG("EofReceived");
		m_transport->WriteEof();
	}

	uint64_t GetSid() { return m_sid; }

	size_t Send(uint32_t msg_id, const char* data, size_t len) {
		auto ret = m_codec.Encode(msg_id, data, len);
		m_transport->Write(ret);
		return ret->size();
	}

	void OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data);

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecX m_codec;
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

using MySessionPtr = std::shared_ptr<MySession>;

class MySessionMgr {
public:
	MySessionMgr(asyncio::EventLoop& event_loop)
		: m_session_factory(*this, event_loop) {}

	MySessionFactory& GetSessionFactory() { return m_session_factory; }

	void OnSessionCreate(MySessionPtr session) { 
		m_sessions[session->GetSid()] = session;
		ASYNCIO_LOG_DEBUG("session:%llu created", session->GetSid());
	}

	void OnSessionDestroy(MySessionPtr session) {
		ASYNCIO_LOG_DEBUG("session:%llu destroyed", session->GetSid());
		m_sessions.erase(session->GetSid());
	}

	MySessionPtr FindSessionFromSid(uint64_t sid) {
		auto it = m_sessions.find(sid);
		if (it == m_sessions.end()) {
			return nullptr;
		}
		return it->second;
	}

	void BroadcastToAll(const std::string& words) {
		for (auto it : m_sessions) {
			auto session = it.second;
			session->Send(0, words.data(), words.size());
		}
	}

private:
	MySessionFactory m_session_factory;
	std::unordered_map<uint64_t, MySessionPtr> m_sessions;
};

void MySession::ConnectionMade(asyncio::TransportPtr transport) {
	m_transport = transport;
	auto self = shared_from_this();
	m_owner.OnSessionCreate(self);
}

void MySession::ConnectionLost(asyncio::TransportPtr transport, int err_code) {
	m_transport = nullptr;
	auto self = shared_from_this();
	m_owner.OnSessionDestroy(self);
}

void MySession::OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {
	m_owner.BroadcastToAll(*data);
}

int main() {
	asyncio::EventLoop my_event_loop;
	MySessionMgr my_session_mgr(my_event_loop);
	my_event_loop.CreateServer(my_session_mgr.GetSessionFactory(), 9000);
	my_event_loop.RunForever();
	return 0;
}
