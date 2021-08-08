#pragma once
#include <memory>

namespace asyncio {

class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

class Protocol {
public:
	virtual std::pair<char*, size_t> GetRxBuffer() = 0;
	virtual void ConnectionMade(TransportPtr transport) = 0;
	virtual void ConnectionLost(TransportPtr transport, int err_code) = 0;
	virtual void DataReceived(size_t len) = 0;
	virtual void EofReceived() = 0;
};

class ProtocolFactory {
public:
	virtual Protocol* CreateProtocol() = 0;
};

} // namespace asyncio
