#include <functional>
#include <unordered_map>
#include "asyncio.h"
#include "util.h"


/*
* 测试方法：
	nc 127.0.0.1 9000
	或者 ./ChatClient
*/

class MySessionMgr;

class MySession : public std::enable_shared_from_this<MySession>, public asyncio::Protocol{
public:
	MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
		: m_owner(owner)
		, m_event_loop(event_loop)
		, m_sid(sid) {
		m_rx_buffer.resize(1024);
	}
	virtual ~MySession() {}

	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return std::make_pair(&m_rx_buffer[0], m_rx_buffer.size());
	}
	virtual void ConnectionMade(asyncio::TransportPtr transport) override;
	virtual void ConnectionLost(asyncio::TransportPtr transport, int err_code) override;
	virtual void DataReceived(size_t len) override;
	virtual void EofReceived() override { 
		ASYNCIO_LOG_DEBUG("EofReceived");
		m_transport->WriteEof();
	}

	uint64_t GetSid() { return m_sid; }

	size_t Send(std::shared_ptr<std::string> data) {
		if (m_transport == nullptr)
			return 0;
		m_transport->Write(data);
		return data->size();
	}

	void Close() {
		if (m_transport != nullptr) {
			m_transport->Close(asyncio::EC_KICK);
		}
	}

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;

	//
	// ProActor模式使用预先分配的缓冲区接收数据
	// 如果缓冲区不够，会分成多次接收
	//
	std::string m_rx_buffer;
	const uint64_t m_sid;
};

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(MySessionMgr& owner, asyncio::EventLoop& event_loop)
		: m_owner(owner)
		, m_event_loop(event_loop) {}
	virtual ~MySessionFactory() {}
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

	void BroadcastToAll(std::shared_ptr<std::string> words) {
		for (auto& it : m_sessions) {
			auto& session = it.second;
			session->Send(words);
		}
	}

private:
	MySessionFactory m_session_factory;
	std::unordered_map<uint64_t, MySessionPtr> m_sessions;
};

void MySession::ConnectionMade(asyncio::TransportPtr transport) {
	m_transport = transport;

	auto data = std::make_shared<std::string>();
	asyncio::util::Text::Format(*data, "> Client[%s:%d %llu] joined\n", m_transport->GetRemoteIp().data(),
								m_transport->GetRemotePort(), GetSid());
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, data]() {
		m_owner.OnSessionCreate(self);
		m_owner.BroadcastToAll(data);
	});
}

void MySession::ConnectionLost(asyncio::TransportPtr transport, int err_code) {
	auto data = std::make_shared<std::string>();
	asyncio::util::Text::Format(*data, "> Client[%s:%d %llu] left\n", m_transport->GetRemoteIp().data(),
								m_transport->GetRemotePort(), GetSid());

	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, data]() {
		m_owner.OnSessionDestroy(self);
		m_owner.BroadcastToAll(data);
		m_transport = nullptr;
	});
}

void MySession::DataReceived(size_t len) {
	std::string content(m_rx_buffer.data(), len);
	if (content.size() == 1 && (content[0] == '\n' || content[0] == '\r')) {
		return;
	}

	auto data = std::make_shared<std::string>();
	asyncio::util::Text::Format(*data, "> Client[%s:%d %llu] say: %s", m_transport->GetRemoteIp().data(),
								m_transport->GetRemotePort(), GetSid(), content.data());
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, data, this]() { m_owner.BroadcastToAll(data); });
}

int main() {
	int port = 9000;
	asyncio::EventLoop my_event_loop(4);
	MySessionMgr my_session_mgr(my_event_loop);
	auto listener = my_event_loop.CreateServer(my_session_mgr.GetSessionFactory(), port);
	if (listener == nullptr) {
		ASYNCIO_LOG_ERROR("listen on %d failed", port);
		return 0;
	}

	ASYNCIO_LOG_INFO("listen on %d suc", port);
	my_event_loop.RunForever();
	return 0;
}
