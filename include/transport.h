#pragma once
#include <exception>
#include <protocol.h>

namespace asyncio {

class Transport : public std::enable_shared_from_this<Transport> {
public:
	Transport(Protocol& protocol, bool is_client);

	void Reconnect();
	void Close();

	void Write(const char* data, size_t len);
	void WriteEof();

	const std::string& GetRemoteIp() const;

private:
	// Protocol生命周期肯定比Transport长
	Protocol& m_protocol;
	bool m_is_client = false;
	std::string m_remote_ip;
};

Transport::Transport(Protocol& protocol, bool is_client)
	: m_protocol(protocol)
	, m_is_client(is_client) {
}

void Transport::Reconnect() {
	if (!m_is_client) {
		throw "not a client, i can't reconnect";
	}
}

void Transport::Close() {
}

void Transport::Write(const char* data, size_t len) {
}

void Transport::WriteEof() {
}

const std::string& Transport::GetRemoteIp() const {
	return m_remote_ip;
}

} // namespace asyncio
