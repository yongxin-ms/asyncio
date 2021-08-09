#pragma once
#include <memory>
#include <asio.hpp>

namespace asyncio {

class Transport;
using TransportPtr = std::shared_ptr<Transport>;

class Protocol {
public:
	virtual std::pair<char*, size_t> GetRxBuffer() = 0;
	virtual void ConnectionMade(TransportPtr transport) = 0;
	virtual void ConnectionLost(TransportPtr transport, int err_code) = 0;
	virtual void DataReceived(size_t len) = 0;
	virtual void EofReceived() = 0;
};

using ProtocolPtr = std::shared_ptr<Protocol>;
using IOContext = asio::io_context;
using IOWorker = asio::executor_work_guard<asio::io_context::executor_type>;

class ProtocolFactory {
public:
	virtual IOContext& AssignIOContext() = 0;
	virtual ProtocolPtr CreateProtocol() = 0;
};

} // namespace asyncio
