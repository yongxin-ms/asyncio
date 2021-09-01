#pragma once
#include <exception>
#include <deque>
#include <asio.hpp>
#include "protocol.h"
#include "log.h"

namespace asyncio {

using IOContext = asio::io_context;
using IOWorker = asio::executor_work_guard<asio::io_context::executor_type>;

enum ErrorCode {
	EC_OK = 0,
	EC_ERROR = 20000,
	EC_KEEP_ALIVE_FAIL,	 // 保持连接失败
	EC_SHUT_DOWN,		 // 主动关闭
	EC_KICK,			 // 踢人
	EC_PACKET_OVER_SIZE, //报文超长
};

class Transport : public std::enable_shared_from_this<Transport> {
public:
	// 作为客户端去连接服务器
	Transport(IOContext& context, ProtocolPtr protocol, const std::string& host, uint16_t port)
		: m_context(context)
		, m_protocol(protocol)
		, m_is_client(true)
		, m_remote_ip(host)
		, m_remote_port(port)
		, m_socket(m_context) {}

	// 作为服务器接受客户端的连接
	Transport(IOContext& context, ProtocolPtr protocol)
		: m_context(context)
		, m_protocol(protocol)
		, m_is_client(false)
		, m_remote_port(0)
		, m_socket(m_context) {}

	virtual ~Transport() {}

	void Connect() {
		auto self = shared_from_this();
		asio::post(m_context, [self, this]() {
			if (!m_is_client) {
				throw "not a client, i can't reconnect";
			}

			ASYNCIO_LOG_DEBUG("start connecting to %s:%d", m_remote_ip.data(), m_remote_port);

			asio::ip::tcp::resolver resolver(m_context);
			auto endpoints = resolver.resolve(m_remote_ip, std::to_string(m_remote_port));
			asio::async_connect(m_socket, endpoints, [self, this](std::error_code ec, asio::ip::tcp::endpoint) {
				if (!ec) {
					ASYNCIO_LOG_DEBUG("connect to %s:%d suc", m_remote_ip.data(), m_remote_port);
					self->m_protocol->ConnectionMade(self);
					self->DoReadData();
				} else {
					ASYNCIO_LOG_DEBUG("connect to %s:%d failed", m_remote_ip.data(), m_remote_port);
					self->m_protocol->ConnectionLost(self, ec.value());
				}
			});
		});
	}

	void DoReadData() {
		auto self = shared_from_this();
		auto rx_buffer = self->m_protocol->GetRxBuffer();
		if (rx_buffer.first == nullptr || rx_buffer.second == 0) {
			throw "wrong rx buffer";
		}

		m_socket.async_read_some(asio::buffer(rx_buffer.first, rx_buffer.second),
								 [self](std::error_code ec, std::size_t length) {
									 if (!ec) {
										 self->m_protocol->DataReceived(length);
										 self->DoReadData();
									 } else {
										 self->InnerClose(ec.value());
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
	void SetRemotePort(uint16_t remote_port) { m_remote_port = remote_port; }
	uint16_t GetRemotePort() const { return m_remote_port; }

	asio::ip::tcp::socket& GetSocket() { return m_socket; }
	ProtocolPtr GetProtocol() { return m_protocol; }
	IOContext& GetIOContext() { return m_context; }

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
								  self->InnerClose(ec.value());
							  }
						  });
	}

	//在io线程中关闭
	void InnerClose(int err_code) {
		ASYNCIO_LOG_DEBUG("InnerClose with ec:%d", err_code);
		if (!m_socket.is_open())
			return;
		m_socket.close();
		auto self = shared_from_this();
		m_protocol->ConnectionLost(self, err_code);
	}

private:
	IOContext& m_context;

	// Protocol生命周期肯定比Transport长
	ProtocolPtr m_protocol;
	bool m_is_client;
	std::string m_remote_ip;
	uint16_t m_remote_port;

	asio::ip::tcp::socket m_socket;
	std::deque<std::shared_ptr<std::string>> m_writeMsgs;
};

void Transport::Close(int err_code) {
	auto self = shared_from_this();
	asio::post(self->m_context, [self, this, err_code]() { InnerClose(err_code); });
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
