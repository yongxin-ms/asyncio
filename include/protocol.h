#pragma once

namespace asyncio {

class Transport {
public:
	void close();

	void set_protocol(Protocol* protocol);
	Protocol* get_protocol();

	void write(const char* data, size_t len);
	void write_eof();

	std::string getpeername();

private:
	Protocol* protocol_;
};

class Protocol {
public:
	virtual void connection_made(Transport* transport) = 0;
	virtual void connection_lost(int err_code) = 0;
	virtual void data_received(const char* data, size_t len) = 0;
	virtual void eof_received() = 0;
};

class ProtocolFactory {
public:
	virtual Protocol* create_protocol() = 0;
};

} // namespace asyncio
