#pragma once
#include <string>
#include <asyncio/transport.h>
#include <asyncio/log.h>
#include <asyncio/context_pool.h>
#include <asyncio/obj_counter.h>

namespace asyncio {

static void fail(asio::error_code ec, char const* what) {
	ASYNCIO_LOG_ERROR("%s : %s", what, ec.message().data());
}

class Listener
	: public std::enable_shared_from_this<Listener>
	, public ObjCounter<Listener> {
public:
	Listener(IOContext& main_context, const ContextPoolPtr& worker_io, ProtocolFactory& protocol_factory)
		: m_main_context(main_context)
		, m_worker_io(worker_io)
		, m_protocol_factory(protocol_factory)
		, m_acceptor(main_context) {}
	Listener(const Listener&) = delete;
	Listener& operator=(const Listener&) = delete;
	~Listener() {
		ASYNCIO_LOG_DEBUG("Listener destroyed");
	}

	// 监听一个指定端口
	bool Listen(uint16_t port) {
		auto ep = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
		asio::error_code ec;
		m_acceptor.open(ep.protocol(), ec);
		if (ec) {
			fail(ec, "open");
			return false;
		}

		m_acceptor.set_option(asio::socket_base::reuse_address(true), ec);
		if (ec) {
			fail(ec, "set_option reuse_address");
			return false;
		}

		m_acceptor.bind(ep, ec);
		if (ec) {
			fail(ec, "bind");
			return false;
		}

		m_acceptor.listen(asio::socket_base::max_listen_connections, ec);
		if (ec) {
			fail(ec, "listen");
			return false;
		}

		Accept();
		ASYNCIO_LOG_DEBUG("start listen %d", port);
		return true;
	}

	void Stop() {
		m_acceptor.close();
	}

private:
	IOContext& GetIOContext() {
		auto worker_io = m_worker_io.lock();
		if (worker_io == nullptr)
			return m_main_context;
		else
			return worker_io->NextContext();
	}

	void Accept() {
		auto protocol = m_protocol_factory.CreateProtocol();
		auto session = std::make_shared<Transport>(GetIOContext(), protocol);
		auto self = shared_from_this();
		m_acceptor.async_accept(session->GetSocket(), [self, this, session, protocol](std::error_code ec) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (!m_acceptor.is_open()) {
				return;
			}

			if (!ec) {
				std::string remote_ip;
				uint16_t remote_port = 0;
				auto endpoint = session->GetSocket().remote_endpoint(ec);
				if (!ec) {
					remote_ip = endpoint.address().to_string();
					remote_port = endpoint.port();
				} else {
					fail(ec, "remote_endpoint");
				}

				ASYNCIO_LOG_DEBUG("a client %s:%d connected", remote_ip.data(), remote_port);

				session->SetRemoteIp(remote_ip);
				session->SetRemotePort(remote_port);

				protocol->ConnectionMade(session);

				//可以在连接建立之后立刻断开它（调用session->Close()），完成一些比如ip黑名单、连接数控制等功能
				if (session->GetSocket().is_open()) {
					session->DoReadData();
				}
			} else {
				fail(ec, "accept");
			}

			Accept();
		});
	}

private:
	IOContext& m_main_context;
	std::weak_ptr<ContextPool> m_worker_io;
	ProtocolFactory& m_protocol_factory;
	asio::ip::tcp::acceptor m_acceptor;
};

} // namespace asyncio
