/* asyncio implementation.
 *
 * Copyright (c) 2019-2022, Will <will at i007 dot cc>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of asyncio nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <exception>
#include <deque>
#include <asio.hpp>
#include <asyncio/protocol.h>
#include <asyncio/log.h>
#include <asyncio/type.h>
#include <asyncio/obj_counter.h>

namespace asyncio {

using IOContext = asio::io_context;
using IOWorker = asio::executor_work_guard<asio::io_context::executor_type>;

enum ErrorCode {
	EC_OK = 0,
	EC_ERROR = 20000,
	EC_SHUT_DOWN, // 主动关闭
};

class Transport
	: public std::enable_shared_from_this<Transport>
	, public ObjCounter<Transport> {
public:
	// 作为客户端去连接服务器
	Transport(IOContext& context, const ProtocolPtr& protocol, const std::string& host, uint16_t port)
		: m_context(context)
		, m_protocol(protocol)
		, m_is_client(true)
		, m_remote_ip(host)
		, m_remote_port(port)
		, m_socket(m_context) {}

	// 作为服务器接受客户端的连接
	Transport(IOContext& context, const ProtocolPtr& protocol)
		: m_context(context)
		, m_protocol(protocol)
		, m_is_client(false)
		, m_remote_port(0)
		, m_socket(m_context) {}

	virtual ~Transport() {
		ASYNCIO_LOG_DEBUG("transport destroyed, is_client(%d), remote_ip(%s), remote_port(%d)",
			m_is_client,
			m_remote_ip.data(),
			m_remote_port);
	}

	Transport(const Transport&) = delete;
	Transport& operator=(const Transport&) = delete;

	void Connect() {
		auto self = shared_from_this();
		asio::post(m_context, [self, this]() {
			if (!m_is_client) {
				throw std::runtime_error("not a client, i can't reconnect");
			}

			ASYNCIO_LOG_DEBUG("start connecting to %s:%d", m_remote_ip.data(), m_remote_port);

			asio::ip::tcp::resolver resolver(m_context);
			auto endpoints = resolver.resolve(m_remote_ip, std::to_string(m_remote_port));
			asio::async_connect(m_socket, endpoints, [self, this](std::error_code ec, asio::ip::tcp::endpoint) {
				if (!ec) {
					ASYNCIO_LOG_DEBUG("connect to %s:%d suc", m_remote_ip.data(), m_remote_port);
					if (auto protocol = m_protocol.lock(); protocol != nullptr) {
						protocol->ConnectionMade(self);
					}
					DoReadData();
				} else {
					ASYNCIO_LOG_DEBUG("connect to %s:%d failed", m_remote_ip.data(), m_remote_port);
					if (auto protocol = m_protocol.lock(); protocol != nullptr) {
						protocol->ConnectionLost(self, ec.value());
					}
				}
			});
		});
	}

	void DoReadData() {
		auto self = shared_from_this();
		if (auto protocol = m_protocol.lock(); protocol != nullptr) {
			auto [buffer, buffer_size] = protocol->GetRxBuffer();
			if (buffer == nullptr || buffer_size == 0) {
				throw std::runtime_error("wrong rx buffer");
			}

			m_socket.async_read_some(
				asio::buffer(buffer, buffer_size), [self, this](std::error_code ec, std::size_t length) {
					if (!ec) {
						if (auto protocol = m_protocol.lock(); protocol != nullptr) {
							if (!protocol->DataReceived(length)) {
								InnerClose(EC_ERROR);
								return;
							}
						}
						DoReadData();
					} else {
						InnerClose(ec.value());
					}
				});
		}
	}

	void Close();
	size_t Write(const StringPtr& msg);

	void SetRemoteIp(const std::string& remote_ip) {
		m_remote_ip = remote_ip;
	}

	const auto& GetRemoteIp() const {
		return m_remote_ip;
	}

	void SetRemotePort(uint16_t remote_port) {
		m_remote_port = remote_port;
	}

	auto GetRemotePort() const {
		return m_remote_port;
	}

	auto& GetSocket() {
		return m_socket;
	}

	void SetNoDelay(bool b) {
		std::error_code ec;
		m_socket.set_option(asio::ip::tcp::no_delay(b), ec);
	}

private:
	void DoWrite() {
		auto self = shared_from_this();
		asio::async_write(m_socket,
			asio::buffer(m_writeMsgs.front()->data(), m_writeMsgs.front()->size()),
			[self, this](std::error_code ec, std::size_t /*length*/) {
				if (!ec) {
					m_writeMsgs.pop_front();
					if (!m_writeMsgs.empty()) {
						DoWrite();
					}
				} else {
					InnerClose(ec.value());
				}
			});
	}

	//在io线程中关闭
	void InnerClose(int err_code) {
		if (m_socket.is_open()) {
			ASYNCIO_LOG_DEBUG("InnerClose with ec:%d", err_code);
			m_socket.close();

			if (auto protocol = m_protocol.lock(); protocol != nullptr) {
				auto self = shared_from_this();
				protocol->ConnectionLost(self, err_code);
			}
		}
	}

private:
	IOContext& m_context;
	std::weak_ptr<Protocol> m_protocol;
	const bool m_is_client;
	std::string m_remote_ip;
	uint16_t m_remote_port;

	asio::ip::tcp::socket m_socket;
	std::deque<StringPtr> m_writeMsgs;
};

void Transport::Close() {
	auto self = shared_from_this();
	int err_code = EC_SHUT_DOWN;
	asio::post(m_context, [self, this, err_code]() {
		InnerClose(err_code);
	});
}

size_t Transport::Write(const StringPtr& msg) {
	auto self = shared_from_this();
	asio::post(m_context, [self, this, msg]() {
		bool write_in_progress = !m_writeMsgs.empty();
		m_writeMsgs.push_back(msg);
		if (!write_in_progress) {
			DoWrite();
		}
	});

	return msg->size();
}

} // namespace asyncio
