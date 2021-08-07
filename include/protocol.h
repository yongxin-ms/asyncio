#pragma once

namespace asyncio {

class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

class Protocol {
public:
	virtual void ConnectionMade(TransportPtr transport) = 0;
	virtual void ConnectionLost(int err_code) = 0;
	virtual void DataReceived(const char* data, size_t len) = 0;
	virtual void EofReceived() = 0;
};

class ProtocolFactory {
public:
	virtual Protocol* CreateProtocol() = 0;
};

} // namespace asyncio
