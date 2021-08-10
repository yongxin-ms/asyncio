#pragma once
#include <string>
#include "transport.h"
#include "log.h"

namespace asyncio {

class Listener {
public:
	Listener(ProtocolFactory& protocol_factory)
		: m_protocol_factory(protocol_factory) {}
	Listener(const Listener&) = delete;
	const Listener& operator=(const Listener&) = delete;
	~Listener() { Stop(); }

	// 监听一个指定端口
	bool Listen(uint16_t port) {
		auto ep = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
		m_acceptor = std::make_unique<asio::ip::tcp::acceptor>(m_protocol_factory.AssignIOContext());
		m_acceptor->open(ep.protocol());
		if (!m_acceptor->is_open()) {
			return false;
		}

		//m_acceptor->set_option(asio::socket_base::reuse_address(true));
		m_acceptor->set_option(asio::ip::tcp::no_delay(true));
		asio::error_code ec;
		m_acceptor->bind(ep, ec);
		if (ec) {
			ASYNCIO_LOG_ERROR("bind port:%d failed, ec:%d", port, ec.value());
			return false;
		}

		m_acceptor->listen();
		Accept();
		return true;
	}

	void Stop() { m_acceptor->close(); }

private:
	void Accept() {
		auto session =
			std::make_shared<Transport>(m_protocol_factory.AssignIOContext(), m_protocol_factory.CreateProtocol());
		m_acceptor->async_accept(session->GetSocket(), [this, session](std::error_code ec) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (!m_acceptor->is_open()) {
				return;
			}

			if (!ec) {
				std::string remote_ip;
				asio::error_code ec;
				auto endpoint = session->GetSocket().remote_endpoint(ec);
				if (!ec) {
					remote_ip = endpoint.address().to_string();
				}

				session->SetRemoteIp(remote_ip);
				session->GetSocket().set_option(asio::ip::tcp::no_delay(true), ec);

				session->GetProtocol()->ConnectionMade(session);

				//可以在连接建立之后立刻断开它（调用session->Close()），完成一些比如ip黑名单、连接数控制等功能
				if (session->GetSocket().is_open()) {
					session->DoReadData();
				}
			} else {
				ASYNCIO_LOG_ERROR("Accept error:%s", ec.message().data());
				// acceptor->close();
			}

			Accept();
		});
	}

private:
	ProtocolFactory& m_protocol_factory;
	std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor;
};

} // namespace asyncio
