#pragma once

namespace asyncio {

class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

class Codec {
public:
	Codec(TransportPtr transport) : m_transport(transport) {}

	virtual void Decode(const char* data, size_t len) = 0;

protected:
	TransportPtr m_transport;
};

}