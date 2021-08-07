#pragma once
#include <protocol.h>

namespace asyncio {

class Transport : public std::enable_shared_from_this<Transport>{
public:
	Transport(Protocol& protocol)
		: m_protocol(protocol){};

	void Close();

	void Write(const char* data, size_t len);
	void WriteEof();

	std::string GetRemoteIp();

private:

	// Protocol生命周期肯定比Transport长
	Protocol& m_protocol;
};

}
