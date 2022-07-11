#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace asyncio {
namespace http {

struct header {
	std::string name;
	std::string value;
};

/// A request received from a client.
struct request {
	std::string method;
	std::string uri;
	int http_version_major;
	int http_version_minor;
	std::vector<header> headers;

	std::string action;
	std::map<std::string, std::string> params;
	std::string content_type;
	std::string content_length;
	std::string body;
	int get_body_length = 0;
};

class connection;
class connection_manager;
class server;

typedef std::shared_ptr<connection> connection_ptr;
typedef std::function<void(connection_ptr conn)> request_handler;

typedef std::shared_ptr<server> server_ptr;

} // namespace http

} // namespace asyncio
