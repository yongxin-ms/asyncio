#pragma once

namespace asyncio {

class Transport {
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

class Protocol {
public:
	virtual void ConnectionMade(Transport* transport) = 0;
	virtual void ConnectionLost(int err_code) = 0;
	virtual void DataReceived(const char* data, size_t len) = 0;
	virtual void EofReceived() = 0;
};

class ProtocolFactory {
public:
	virtual Protocol* CreateProtocol() = 0;
};

} // namespace asyncio
