#pragma once

namespace asyncio {

class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

class Codec {
public:
	virtual void Decode(TransportPtr transport, const char* data, size_t len) = 0;
};

} // namespace asyncio
