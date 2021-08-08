#pragma once
#include <exception>
#include <deque>
#include <asio.hpp>
#include "protocol.h"

namespace asyncio {

class Transport : public std::enable_shared_from_this<Transport> {
public:
	// 作为客户端去连接服务器
	Transport(asio::io_context& context, Protocol& protocol, const std::string& host, uint16_t port)
		: m_context(context)
		, m_protocol(protocol)
		, m_is_client(true)
		, m_remote_ip(host)
		, m_remote_port(port)
		, m_socket(m_context) {
		m_rx_buffer.resize(1024);
		Connect();
	}

	// 作为服务器接受客户端的连接
	Transport(asio::io_context& context, Protocol& protocol)
		: m_context(context)
		, m_protocol(protocol)
		, m_is_client(false)
		, m_socket(m_context) {
		m_rx_buffer.resize(1024);
	}

	void Connect() {
		if (!m_is_client) {
			throw "not a client, i can't reconnect";
		}

		asio::ip::tcp::resolver resolver(m_context);
		auto endpoints = resolver.resolve(m_remote_ip, std::to_string(m_remote_port));
		auto self = shared_from_this();
		asio::async_connect(m_socket, endpoints, [self](std::error_code ec, asio::ip::tcp::endpoint) {
			if (!ec) {
				self->m_protocol.ConnectionMade(self);
				self->DoReadData();
			} else {
				self->m_protocol.ConnectionLost(ec.value());
			}
		});
	}

	void Reconnect() {
		Close(0);
		Connect();
	}

	void DoReadData() {
		auto self = shared_from_this();
		m_socket.async_read_some(
			asio::buffer(m_rx_buffer.data(), m_rx_buffer.size()), [self](std::error_code ec, std::size_t length) {
				if (!ec) {
					self->m_protocol.DataReceived(self->m_rx_buffer.data(), length);
					self->DoReadData();
				} else {
					self->Close(ec.value());
				}
			});
	}

	void Close(int err_code);

	void Write(const std::shared_ptr<std::string>& msg);
	void Write(const char* msg, size_t len) {
		auto data = std::make_shared<std::string>(msg, len);
		Write(data);
	}
	void WriteEof();

	void SetRemoteIp(const std::string& remote_ip) { m_remote_ip = remote_ip; }
	const std::string& GetRemoteIp() const { return m_remote_ip; }

	asio::ip::tcp::socket& GetSocket() { return m_socket; }

	Protocol& GetProtocol() { return m_protocol; }

private:
	void DoWrite() {
		auto self = shared_from_this();
		asio::async_write(m_socket, asio::buffer(m_writeMsgs.front()->data(), m_writeMsgs.front()->size()),
			[self](std::error_code ec, std::size_t /*length*/) {
				if (!ec) {
					self->m_writeMsgs.pop_front();
					if (!self->m_writeMsgs.empty()) {
						self->DoWrite();
					}
				} else {
					self->Close(ec.value());
				}
			});
	}

private:
	asio::io_context& m_context;

	// Protocol生命周期肯定比Transport长
	Protocol& m_protocol;
	bool m_is_client = false;
	std::string m_remote_ip;
	uint16_t m_remote_port;

	asio::ip::tcp::socket m_socket;
	std::string m_rx_buffer;
	std::deque<std::shared_ptr<std::string>> m_writeMsgs;
};

void Transport::Close(int err_code) {
	if (!m_socket.is_open())
		return;
	m_socket.close();

	auto self = shared_from_this();
	m_protocol.ConnectionLost(err_code);
}

void Transport::Write(const std::shared_ptr<std::string>& msg) {
	auto self = shared_from_this();
	asio::post(self->m_context, [self, msg]() {
		bool write_in_progress = !self->m_writeMsgs.empty();
		self->m_writeMsgs.push_back(msg);
		if (!write_in_progress) {
			self->DoWrite();
		}
	});
}

void Transport::WriteEof() {}

} // namespace asyncio
