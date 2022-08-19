#include <memory>
#include "conn_mgr.h"
#include "my_conn.h"
#include "app.h"

MyConnectionFactory::MyConnectionFactory(MyConnMgr& owner)
	: m_owner(owner) {}

asyncio::ProtocolPtr MyConnectionFactory::CreateProtocol() {
	return std::make_shared<MyConnection>(m_owner);
}

MyConnMgr::MyConnMgr()
	: m_conn_factory(*this) {}

void MyConnMgr::StartConnect(const std::string& ip, uint16_t port, int conn_count) {
	for (auto& conn : m_conns) {
		conn->Disconnect(false);
	}
	m_conns.clear();

	for (int i = 0; i < conn_count; i++) {
		auto conn = g_EventLoop.CreateConnection(m_conn_factory, ip, port);
		MyConnectionPtr my_conn = std::static_pointer_cast<MyConnection>(conn);
		m_conns.push_back(my_conn);
	}
}

void MyConnMgr::OnMessage(const MyConnectionPtr& conn, uint32_t msg_id, const std::shared_ptr<std::string>& data) {
	conn->Send(msg_id, data->data(), data->size());
	App::Instance()->IncQps();
}
