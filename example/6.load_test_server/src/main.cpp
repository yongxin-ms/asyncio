#include "asyncio.h"

class MySession : public asyncio::Protocol {
public:
	MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
		: m_owner(owner)
		, m_event_loop(event_loop)
		, m_codec(std::bind(&MySession::OnMyMessageFunc, this, holder1))
		, m_sid(sid) {
	}

	virtual void ConnectionMade(TransportPtr transport) override {
		m_transport = transport;
		m_owner.OnSessionCreate(this);
	}

	virtual void ConnectionLost(int err_code) override {
		m_transport = null_ptr;
		m_owner.OnSessionDestroy(this);
	}

	virtual void DataReceived(const char* data, size_t len) override {
		m_codec.Decode(m_transport, data, len);
	}

	virtual void EofReceived() override {
		m_transport->WriteEof();
	}

	uint64_t GetSid() {
		return m_sid;
	}

	size_t Send(const char* data, size_t len) {
		if (!m_is_connected) {
			return 0;
		}

		return m_transport->Write(m_codec.Encode(data, len));
	}

	void OnMyMessageFunc(std::shared_ptr<std::string> data) {
	}

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecLen m_codec;
	const uint64_t m_sid;
}

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(asyncio::EventLoop& event_loop)
		: m_event_loop(event_loop) {
	}

	virtual Protocol* CreateProtocol() override {
		static uint64_t g_sid = 0;
		uint64_t sid = ++g_sid;
		auto new_session = new MySession(m_event_loop, sid);
	}

private:
	asyncio::EventLoop& m_event_loop;
}

class MySessionMgr {
public:
	MySessionMgr(asyncio::EventLoop& event_loop)
		: m_session_factory(event_loop) {
	}

	MySessionFactory& GetSessionFactory() {
		return m_session_factory;
	}

	void OnSessionCreate(MySession* session) {
		m_sessions[session->GetSid()] = session;
	}

	void OnSessionDestroy(MySession* session) {
		m_sessions.erase(session->GetSid());
		delete session;
	}

	MySession* FindSessionFromSid(uint64_t sid) {
		auto it = m_sessions.find(sid);
		if (it == m_sessions.end()) {
			return null_ptr;
		}
		return it->second;
	}

private:
	MySessionFactory m_session_factory;
	std::map<uint64_t, MySession*> m_sessions;
}

int main() {
	auto my_event_loop = new asyncio::EventLoop();
	auto my_session_mgr = new MySessionMgr(*my_event_loop);
	my_event_loop->CreateServer(&my_session_mgr->GetSessionFactory(), 9000);
	my_event_loop->RunForever();
	return 0;
}
