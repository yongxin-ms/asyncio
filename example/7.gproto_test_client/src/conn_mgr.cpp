#include <memory>
#include "conn_mgr.h"
#include "my_conn.h"
#include "app.h"

MyConnectionFactory::MyConnectionFactory(MyConnMgr& owner, asyncio::EventLoop& event_loop)
	: m_owner(owner)
	, m_event_loop(event_loop) {
}

asyncio::ProtocolPtr MyConnectionFactory::CreateProtocol() {
	return std::make_shared<MyConnection>(m_owner, m_event_loop);
}

MyConnMgr::MyConnMgr(asyncio::EventLoop& event_loop)
	: m_event_loop(event_loop)
	, m_conn_factory(*this, event_loop) {
}

void MyConnMgr::StartConnect(const std::string& ip, uint16_t port, int conn_count) {
	for (auto& conn : m_conns) {
		conn->Close();
	}
	m_conns.clear();

	for (int i = 0; i < conn_count; i++) {
		auto conn = m_event_loop.CreateConnection(m_conn_factory, ip, port);
		MyConnectionPtr my_conn = std::static_pointer_cast<MyConnection>(conn);
		m_conns.push_back(my_conn);
	}
}

void MyConnMgr::OnMessage(const MyConnectionPtr& conn, uint32_t msg_id, const std::shared_ptr<std::string>& data) {
	conn->Send(msg_id, data->data(), data->size());
	App::Instance()->IncQps();
}
