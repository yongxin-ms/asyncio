﻿#include <unordered_map>
#include <asyncio.h>

int g_cur_qps = 0;
asyncio::DelayTimerPtr g_timer = nullptr;

class MySessionMgr;

/*
 * 注意：这个结构得是POD的
 */
struct MyHeader {
	enum : uint32_t {
		MAGIC_NUM = 0x1a2b3c4d,
	};

	MyHeader(uint16_t msg_id = 0, uint16_t version = 0, uint32_t sequence = 0)
		: msg_id(msg_id)
		, version(version)
		, sequence(sequence) {}

	uint16_t msg_id;
	uint16_t version;
	uint32_t sequence;
};

class MySession
	: public std::enable_shared_from_this<MySession>
	, public asyncio::Protocol {
public:
	MySession(MySessionMgr& owner, asyncio::EventLoop& event_loop, uint64_t sid)
		: m_owner(owner)
		, m_event_loop(event_loop)
		, m_codec(std::bind(&MySession::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2))
		, m_sid(sid) {}

	virtual ~MySession() {
		ASYNCIO_LOG_DEBUG("MySession:%llu destroyed", m_sid);
	}

	uint64_t GetSid() {
		return m_sid;
	}

	size_t Send(const char* data, size_t len) {
		auto ret = m_codec.Encode(MyHeader(), data, uint32_t(len));
		return Write(ret);
	}

private:
	virtual std::pair<char*, size_t> GetRxBuffer() override {
		return m_codec.GetRxBuffer();
	}

	virtual void ConnectionMade(const asyncio::TransportPtr& transport) override;
	virtual void ConnectionLost(const asyncio::TransportPtr& transport, int err_code) override;
	virtual bool DataReceived(size_t len) override {
		return m_codec.Decode(len);
	}

	virtual size_t Write(const asyncio::StringPtr& s) override {
		if (m_transport != nullptr) {
			return m_transport->Write(s);
		} else {
			return 0;
		}
	}

	void OnMyMessageFunc(const MyHeader& header, const std::shared_ptr<std::string>& data) {
		auto self = shared_from_this();
		m_event_loop.QueueInLoop([self, data]() {
			self->Send(data->data(), data->size());
			g_cur_qps++;
		});
	}

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	asyncio::TransportPtr m_transport;
	asyncio::CodecUserHeader<MyHeader, MyHeader::MAGIC_NUM> m_codec;
	const uint64_t m_sid;
};

using MySessionPtr = std::shared_ptr<MySession>;

class MySessionFactory : public asyncio::ProtocolFactory {
public:
	MySessionFactory(MySessionMgr& owner, asyncio::EventLoop& event_loop)
		: m_owner(owner)
		, m_event_loop(event_loop) {}

	virtual asyncio::ProtocolPtr CreateProtocol() override {
		uint64_t sid = ++m_cur_sid;
		return std::make_shared<MySession>(m_owner, m_event_loop, sid);
	}

private:
	MySessionMgr& m_owner;
	asyncio::EventLoop& m_event_loop;
	std::atomic_uint64_t m_cur_sid;
};

class MySessionMgr {
public:
	explicit MySessionMgr(asyncio::EventLoop& event_loop)
		: m_session_factory(*this, event_loop) {}

	MySessionFactory& GetSessionFactory() {
		return m_session_factory;
	}

	void OnSessionCreate(const MySessionPtr& session) {
		assert(m_sessions.find(session->GetSid()) == m_sessions.end());
		m_sessions[session->GetSid()] = session;
	}

	void OnSessionDestroy(const MySessionPtr& session) {
		m_sessions.erase(session->GetSid());
	}

	MySessionPtr FindSessionFromSid(uint64_t sid) {
		auto it = m_sessions.find(sid);
		if (it == m_sessions.end()) {
			return nullptr;
		}
		return it->second;
	}

private:
	MySessionFactory m_session_factory;
	std::unordered_map<uint64_t, MySessionPtr> m_sessions;
};

void MySession::ConnectionMade(const asyncio::TransportPtr& transport) {
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, transport]() {
		m_transport = transport;
		m_owner.OnSessionCreate(self);
	});
}

void MySession::ConnectionLost(const asyncio::TransportPtr& transport, int err_code) {
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this]() {
		m_owner.OnSessionDestroy(self);
		m_transport = nullptr;
	});
}

int main(int argc, char* argv[]) {
	asyncio::SetLogHandler(
		[](asyncio::Log::LogLevel lv, const char* msg) {
			std::string time_now = asyncio::util::Time::FormatDateTime(std::chrono::system_clock::now());
			switch (lv) {
			case asyncio::Log::kError:
				printf("%s Error: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kWarning:
				printf("%s Warning: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kInfo:
				printf("%s Info: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kDebug:
				printf("%s Debug: %s\n", time_now.c_str(), msg);
				break;
			default:
				break;
			}
		},
		asyncio::Log::kDebug);

	if (argc < 2) {
		printf("port \n");
		return 0;
	}

	int port = std::atoi(argv[1]);
	asyncio::EventLoop my_event_loop;
	MySessionMgr my_session_mgr(my_event_loop);
	auto listener = my_event_loop.CreateServer(my_session_mgr.GetSessionFactory(), port);
	if (listener == nullptr) {
		ASYNCIO_LOG_ERROR("listen on %d failed", port);
		return 0;
	}

	ASYNCIO_LOG_INFO("listen on %d suc", port);
	g_timer = my_event_loop.CallLater(
		std::chrono::seconds(1),
		[]() {
			if (g_cur_qps != 0) {
				ASYNCIO_LOG_DEBUG("Cur qps:%d", g_cur_qps);
				g_cur_qps = 0;
			}
		},
		asyncio::RUN_FOREVER);

	my_event_loop.RunForever();
	return 0;
}
