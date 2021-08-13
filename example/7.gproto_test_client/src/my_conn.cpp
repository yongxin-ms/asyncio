#include "my_conn.h"
#include "conn_mgr.h"

MyConnection::MyConnection(MyConnMgr& owner, asyncio::EventLoop& event_loop)
	: m_owner(owner)
	, m_event_loop(event_loop)
	, m_codec(std::bind(&MyConnection::OnMyMessageFunc, this, std::placeholders::_1, std::placeholders::_2),
			  std::bind(&MyConnection::OnReceivedPong, this)) {}

std::pair<char*, size_t> MyConnection::GetRxBuffer() {
	return m_codec.GetRxBuffer();
}

void MyConnection::ConnectionMade(asyncio::TransportPtr transport) {
	m_transport = transport;
	ASYNCIO_LOG_DEBUG("ConnectionMade");

	m_ping_counter = 0;
	auto self = shared_from_this();
	m_ping_timer = m_transport->CallLater(10000, [self, this]() {
		if (m_transport != nullptr) {
			if (m_ping_counter > 2) {
				ASYNCIO_LOG_WARN("Keep alive failed, Closing");
				m_transport->Close(asyncio::EC_KEEP_ALIVE_FAIL);
			} else {
				ASYNCIO_LOG_DEBUG("Ping Timer");
				m_codec.send_ping(m_transport);
				m_ping_counter++;
			}
		}
		m_ping_timer->Start();
	});

	// std::string msg("hello,world!");
	// Send(0, msg.data(), msg.size());
}

void MyConnection::ConnectionLost(asyncio::TransportPtr transport, int err_code) {
	m_transport = nullptr;
	ASYNCIO_LOG_DEBUG("ConnectionLost");

	m_event_loop.CallLater(3000, [transport]() {
		ASYNCIO_LOG_DEBUG("Start Reconnect");
		transport->Connect();
	});
}

void MyConnection::DataReceived(size_t len) {
	m_codec.Decode(m_transport, len);
}

void MyConnection::EofReceived() {
	m_transport->WriteEof();
}

size_t MyConnection::Send(uint32_t msg_id, const char* data, size_t len) {
	if (!IsConnected()) {
		return 0;
	}

	auto ret = m_codec.Encode(msg_id, data, len);
	m_transport->Write(ret);
	return ret->size();
}

void MyConnection::Close() {
	if (m_transport != nullptr) {
		m_transport->Close(asyncio::EC_SHUT_DOWN);
	}
}

void MyConnection::OnMyMessageFunc(uint32_t msg_id, std::shared_ptr<std::string> data) {
	auto self = shared_from_this();
	m_event_loop.QueueInLoop([self, this, msg_id, data]() { m_owner.OnMessage(self, msg_id, data); });
}

void MyConnection::OnReceivedPong() {
	m_ping_counter = 0;
}

bool MyConnection::IsConnected() {
	return m_transport != nullptr;
}
