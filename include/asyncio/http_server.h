#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <asio.hpp>
#include <asyncio/http_base.h>
#include <asyncio/util.h>

namespace asyncio {
namespace http {

/// Parser for incoming requests.
class request_parser {
public:
	/// Construct ready to parse the request method.
	request_parser()
		: state_(method_start){};

	/// Reset to initial parser state.
	void reset() { state_ = method_start; };

	/// Result of parse.
	enum result_type { good, bad, indeterminate };

	/// Parse some data. The enum return value is good when a complete request has
	/// been parsed, bad if the data is invalid, indeterminate when more data is
	/// required. The InputIterator return value indicates how much of the input
	/// has been consumed.
	template <typename InputIterator>
	std::tuple<result_type, InputIterator> parse(request& req, InputIterator begin, InputIterator end) {
		while (begin != end) {
			result_type result = consume(req, *begin++);
			if (result == good || result == bad)
				return std::make_tuple(result, begin);
		}
		return std::make_tuple(indeterminate, begin);
	}

private:
	/// Handle the next character of input.
	result_type consume(request& req, char input) {
		switch (state_) {
		case method_start:
			if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
				return bad;
			} else {
				state_ = method;
				req.method.push_back(input);
				return indeterminate;
			}
		case method:
			if (input == ' ') {
				state_ = uri;
				return indeterminate;
			} else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
				return bad;
			} else {
				req.method.push_back(input);
				return indeterminate;
			}
		case uri:
			if (input == ' ') {
				state_ = http_version_h;
				return indeterminate;
			} else if (is_ctl(input)) {
				return bad;
			} else {
				req.uri.push_back(input);
				return indeterminate;
			}
		case http_version_h:
			if (input == 'H') {
				state_ = http_version_t_1;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_t_1:
			if (input == 'T') {
				state_ = http_version_t_2;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_t_2:
			if (input == 'T') {
				state_ = http_version_p;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_p:
			if (input == 'P') {
				state_ = http_version_slash;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_slash:
			if (input == '/') {
				req.http_version_major = 0;
				req.http_version_minor = 0;
				state_ = http_version_major_start;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_major_start:
			if (is_digit(input)) {
				req.http_version_major = req.http_version_major * 10 + input - '0';
				state_ = http_version_major;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_major:
			if (input == '.') {
				state_ = http_version_minor_start;
				return indeterminate;
			} else if (is_digit(input)) {
				req.http_version_major = req.http_version_major * 10 + input - '0';
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_minor_start:
			if (is_digit(input)) {
				req.http_version_minor = req.http_version_minor * 10 + input - '0';
				state_ = http_version_minor;
				return indeterminate;
			} else {
				return bad;
			}
		case http_version_minor:
			if (input == '\r') {
				state_ = expecting_newline_1;
				return indeterminate;
			} else if (is_digit(input)) {
				req.http_version_minor = req.http_version_minor * 10 + input - '0';
				return indeterminate;
			} else {
				return bad;
			}
		case expecting_newline_1:
			if (input == '\n') {
				state_ = header_line_start;
				return indeterminate;
			} else {
				return bad;
			}
		case header_line_start:
			if (input == '\r') {
				state_ = expecting_newline_3;
				return indeterminate;
			} else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
				state_ = header_lws;
				return indeterminate;
			} else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
				return bad;
			} else {
				req.headers.push_back(header());
				req.headers.back().name.push_back(input);
				state_ = header_name;
				return indeterminate;
			}
		case header_lws:
			if (input == '\r') {
				state_ = expecting_newline_2;
				return indeterminate;
			} else if (input == ' ' || input == '\t') {
				return indeterminate;
			} else if (is_ctl(input)) {
				return bad;
			} else {
				state_ = header_value;
				req.headers.back().value.push_back(input);
				return indeterminate;
			}
		case header_name:
			if (input == ':') {
				state_ = space_before_header_value;
				return indeterminate;
			} else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
				return bad;
			} else {
				req.headers.back().name.push_back(input);
				return indeterminate;
			}
		case space_before_header_value:
			if (input == ' ') {
				state_ = header_value;
				return indeterminate;
			} else {
				return bad;
			}
		case header_value:
			if (input == '\r') {
				state_ = expecting_newline_2;
				return indeterminate;
			} else if (is_ctl(input)) {
				return bad;
			} else {
				req.headers.back().value.push_back(input);
				if (req.headers.back().name == "Content-Length") {
					req.content_length = req.headers.back().value;
				}
				if (req.headers.back().name == "Content-Type") {
					req.content_type = req.headers.back().value;
				}
				return indeterminate;
			}
		case expecting_newline_2:
			if (input == '\n') {
				state_ = header_line_start;
				return indeterminate;
			} else {
				return bad;
			}
		case expecting_newline_3:
			if (input == '\n') {
				state_ = body_start;
				if (std::atoi(req.content_length.data()) == 0) {
					return good;
				} else {
					return indeterminate;
				}
			} else {
				return indeterminate;
			}
		case body_start:
			++req.get_body_length;
			req.body.push_back(input);
			if (req.get_body_length == std::atoi(req.content_length.data())) {
				req.get_body_length = 0;
				return good;
			}
			return indeterminate;
		default:
			return bad;
		}
	}

	/// Check if a byte is an HTTP character.
	static bool is_char(int c) { return c >= 0 && c <= 127; }

	/// Check if a byte is an HTTP control character.
	static bool is_ctl(int c) { return (c >= 0 && c <= 31) || (c == 127); }

	/// Check if a byte is defined as an HTTP tspecial character.
	static bool is_tspecial(int c) {
		switch (c) {
		case '(':
		case ')':
		case '<':
		case '>':
		case '@':
		case ',':
		case ';':
		case ':':
		case '\\':
		case '"':
		case '/':
		case '[':
		case ']':
		case '?':
		case '=':
		case '{':
		case '}':
		case ' ':
		case '\t':
			return true;
		default:
			return false;
		}
	}

	/// Check if a byte is a digit.
	static bool is_digit(int c) { return c >= '0' && c <= '9'; }

	/// The current state of the parser.
	enum state {
		method_start,
		method,
		uri,
		http_version_h,
		http_version_t_1,
		http_version_t_2,
		http_version_p,
		http_version_slash,
		http_version_major_start,
		http_version_major,
		http_version_minor_start,
		http_version_minor,
		expecting_newline_1,
		header_line_start,
		header_lws,
		header_name,
		space_before_header_value,
		header_value,
		expecting_newline_2,
		expecting_newline_3,
		body_start,
		body_end
	} state_;
};

class url_parser {
private:
	static bool url_decode(const std::string& in, std::string& out) {
		out.clear();
		out.reserve(in.size());
		for (std::size_t i = 0; i < in.size(); ++i) {
			if (in[i] == '%') {
				if (i + 3 <= in.size()) {
					int value = 0;
					std::istringstream is(in.substr(i + 1, 2));
					if (is >> std::hex >> value) {
						out += static_cast<char>(value);
						i += 2;
					} else {
						return false;
					}
				} else {
					return false;
				}
			} else if (in[i] == '+') {
				out += ' ';
			} else {
				out += in[i];
			}
		}
		return true;
	}

public:
	static bool parse_uri(const std::string& uri, std::string& action, std::map<std::string, std::string>& params) {
		std::string request_path;
		std::string data = "";
		if (!url_decode(uri, request_path)) {
			return false;
		}

		if (request_path.size() == 0 || request_path[0] != '/') {
			action = "";
			params.clear();
			return false;
		}

		// /status?id=33&name=xxx
		size_t pos = request_path.find('?', 1);
		if (pos == std::string::npos) {
			action = request_path.substr(1);
			params.clear();
			return true;
		}

		action = request_path.substr(1, pos - 1);

		std::vector<std::string> param_str;
		util::Text::SplitStr(param_str, request_path.substr(pos + 1), '&');
		for (size_t i = 0; i < param_str.size(); i++) {
			std::vector<std::string> param_pair;
			util::Text::SplitStr(param_pair, param_str[i], '=');
			if (param_pair.size() == 2) {
				params[param_pair[0]] = param_pair[1];
			}
		}

		return true;
	}
};

/// A reply to be sent to a client.
struct reply {
	/// The status of the reply.
	enum status_type {
		no_token = -3,
		outdate = -2,
		bad_token = -1,

		ok = 200,
		created = 201,
		accepted = 202,
		no_content = 204,
		multiple_choices = 300,
		moved_permanently = 301,
		moved_temporarily = 302,
		not_modified = 304,
		bad_request = 400,
		unauthorized = 401,
		forbidden = 403,
		not_found = 404,
		internal_server_error = 500,
		not_implemented = 501,
		bad_gateway = 502,
		service_unavailable = 503
	} status;

	/// The headers to be included in the reply.
	std::vector<header> headers;

	/// The content to be sent in the reply.
	std::string content;

	/// Convert the reply into a vector of buffers. The buffers do not own the
	/// underlying memory blocks, therefore the reply object must remain valid and
	/// not be changed until the write operation has completed.
	inline std::vector<asio::const_buffer> to_buffers();

	/// Get a stock reply.
	inline static reply stock_reply(status_type status);
};

namespace status_strings {

const std::string ok = "HTTP/1.0 200 OK\r\n";
const std::string created = "HTTP/1.0 201 Created\r\n";
const std::string accepted = "HTTP/1.0 202 Accepted\r\n";
const std::string no_content = "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices = "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently = "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily = "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified = "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request = "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized = "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden = "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found = "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error = "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented = "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway = "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable = "HTTP/1.0 503 Service Unavailable\r\n";

inline asio::const_buffer to_buffer(reply::status_type status) {
	switch (status) {
	case reply::ok:
		return asio::buffer(ok);
	case reply::created:
		return asio::buffer(created);
	case reply::accepted:
		return asio::buffer(accepted);
	case reply::no_content:
		return asio::buffer(no_content);
	case reply::multiple_choices:
		return asio::buffer(multiple_choices);
	case reply::moved_permanently:
		return asio::buffer(moved_permanently);
	case reply::moved_temporarily:
		return asio::buffer(moved_temporarily);
	case reply::not_modified:
		return asio::buffer(not_modified);
	case reply::bad_request:
		return asio::buffer(bad_request);
	case reply::unauthorized:
		return asio::buffer(unauthorized);
	case reply::forbidden:
		return asio::buffer(forbidden);
	case reply::not_found:
		return asio::buffer(not_found);
	case reply::internal_server_error:
		return asio::buffer(internal_server_error);
	case reply::not_implemented:
		return asio::buffer(not_implemented);
	case reply::bad_gateway:
		return asio::buffer(bad_gateway);
	case reply::service_unavailable:
		return asio::buffer(service_unavailable);
	default:
		return asio::buffer(internal_server_error);
	}
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = {':', ' '};
const char crlf[] = {'\r', '\n'};

} // namespace misc_strings

std::vector<asio::const_buffer> reply::to_buffers() {
	std::vector<asio::const_buffer> buffers;
	buffers.push_back(status_strings::to_buffer(status));
	for (std::size_t i = 0; i < headers.size(); ++i) {
		header& h = headers[i];
		buffers.push_back(asio::buffer(h.name));
		buffers.push_back(asio::buffer(misc_strings::name_value_separator));
		buffers.push_back(asio::buffer(h.value));
		buffers.push_back(asio::buffer(misc_strings::crlf));
	}
	buffers.push_back(asio::buffer(misc_strings::crlf));
	buffers.push_back(asio::buffer(content));
	return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] = "<html>"
					   "<head><title>Created</title></head>"
					   "<body><h1>201 Created</h1></body>"
					   "</html>";
const char accepted[] = "<html>"
						"<head><title>Accepted</title></head>"
						"<body><h1>202 Accepted</h1></body>"
						"</html>";
const char no_content[] = "<html>"
						  "<head><title>No Content</title></head>"
						  "<body><h1>204 Content</h1></body>"
						  "</html>";
const char multiple_choices[] = "<html>"
								"<head><title>Multiple Choices</title></head>"
								"<body><h1>300 Multiple Choices</h1></body>"
								"</html>";
const char moved_permanently[] = "<html>"
								 "<head><title>Moved Permanently</title></head>"
								 "<body><h1>301 Moved Permanently</h1></body>"
								 "</html>";
const char moved_temporarily[] = "<html>"
								 "<head><title>Moved Temporarily</title></head>"
								 "<body><h1>302 Moved Temporarily</h1></body>"
								 "</html>";
const char not_modified[] = "<html>"
							"<head><title>Not Modified</title></head>"
							"<body><h1>304 Not Modified</h1></body>"
							"</html>";
const char bad_request[] = "<html>"
						   "<head><title>Bad Request</title></head>"
						   "<body><h1>400 Bad Request</h1></body>"
						   "</html>";
const char unauthorized[] = "<html>"
							"<head><title>Unauthorized</title></head>"
							"<body><h1>401 Unauthorized</h1></body>"
							"</html>";
const char forbidden[] = "<html>"
						 "<head><title>Forbidden</title></head>"
						 "<body><h1>403 Forbidden</h1></body>"
						 "</html>";
const char not_found[] = "<html>"
						 "<head><title>Not Found</title></head>"
						 "<body><h1>404 Not Found</h1></body>"
						 "</html>";
const char internal_server_error[] = "<html>"
									 "<head><title>Internal Server Error</title></head>"
									 "<body><h1>500 Internal Server Error</h1></body>"
									 "</html>";
const char not_implemented[] = "<html>"
							   "<head><title>Not Implemented</title></head>"
							   "<body><h1>501 Not Implemented</h1></body>"
							   "</html>";
const char bad_gateway[] = "<html>"
						   "<head><title>Bad Gateway</title></head>"
						   "<body><h1>502 Bad Gateway</h1></body>"
						   "</html>";
const char service_unavailable[] = "<html>"
								   "<head><title>Service Unavailable</title></head>"
								   "<body><h1>503 Service Unavailable</h1></body>"
								   "</html>";

inline std::string to_string(reply::status_type status) {
	switch (status) {
	case reply::ok:
		return ok;
	case reply::created:
		return created;
	case reply::accepted:
		return accepted;
	case reply::no_content:
		return no_content;
	case reply::multiple_choices:
		return multiple_choices;
	case reply::moved_permanently:
		return moved_permanently;
	case reply::moved_temporarily:
		return moved_temporarily;
	case reply::not_modified:
		return not_modified;
	case reply::bad_request:
		return bad_request;
	case reply::unauthorized:
		return unauthorized;
	case reply::forbidden:
		return forbidden;
	case reply::not_found:
		return not_found;
	case reply::internal_server_error:
		return internal_server_error;
	case reply::not_implemented:
		return not_implemented;
	case reply::bad_gateway:
		return bad_gateway;
	case reply::service_unavailable:
		return service_unavailable;
	default:
		return internal_server_error;
	}
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status) {
	reply rep;
	rep.status = status;
	rep.content = stock_replies::to_string(status);
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = std::to_string(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "text/html";
	return rep;
}

class connection : public std::enable_shared_from_this<connection> {
public:
	/// Construct a connection with the given socket.
	explicit connection(asio::ip::tcp::socket socket, connection_manager& manager, request_handler& handler)
		: socket_(std::move(socket))
		, connection_manager_(manager)
		, request_handler_(handler) {}

	connection(const connection&) = delete;
	const connection& operator=(const connection&) = delete;

	/// Start the first asynchronous operation for the connection.
	void start() {
		asio::error_code ec;
		auto endpoint = socket_.remote_endpoint(ec);
		if (!ec) {
			remote_ip_ = endpoint.address().to_string();
		}

		do_read();
	}

	/// Stop all asynchronous operations associated with the connection.
	void stop() { socket_.close(); }

	/// Perform an asynchronous write operation.
	inline void do_write();

	const request& get_req() const { return request_; }
	reply& get_rep() { return reply_; }
	const std::string& get_remote_ip() const { return remote_ip_; }

private:
	/// Perform an asynchronous read operation.
	inline void do_read();

	/// Socket for the connection.
	asio::ip::tcp::socket socket_;

	/// The manager for this connection.
	connection_manager& connection_manager_;

	/// The handler used to process the incoming request.
	request_handler& request_handler_;

	/// Buffer for incoming data.
	std::array<char, 8192> buffer_;

	/// The incoming request.
	request request_;

	/// The parser for the incoming request.
	request_parser request_parser_;

	/// The reply to be sent back to the client.
	reply reply_;

	std::string remote_ip_;
};

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class connection_manager {
public:
	/// Construct a connection manager.
	connection_manager() {}
	connection_manager(const connection_manager&) = delete;
	const connection_manager& operator=(const connection_manager&) = delete;

	/// Add the specified connection to the manager and start it.
	void start(connection_ptr c) {
		connections_.insert(c);
		c->start();
	}

	/// Stop the specified connection.
	void stop(connection_ptr c) {
		connections_.erase(c);
		c->stop();
	}

	/// Stop all connections.
	void stop_all() {
		for (auto c : connections_)
			c->stop();
		connections_.clear();
	}

private:
	/// The managed connections.
	std::set<connection_ptr> connections_;
};

void connection::do_read() {
	auto self(shared_from_this());
	socket_.async_read_some(asio::buffer(buffer_), [this, self](std::error_code ec, std::size_t bytes_transferred) {
		if (!ec) {
			request_parser::result_type result;
			std::tie(result, std::ignore) =
				request_parser_.parse(request_, buffer_.data(), buffer_.data() + bytes_transferred);

			if (result == request_parser::good) {
				// request_handler_.handle_request(request_, reply_);
				// do_write();

				// 在request_handler_里面调用do_write()函数可以实现异步
				url_parser::parse_uri(request_.uri, request_.action, request_.params);
				request_handler_(self);
			} else if (result == request_parser::bad) {
				reply_ = reply::stock_reply(reply::bad_request);
				do_write();
			} else {
				do_read();
			}
		} else if (ec != asio::error::operation_aborted) {
			connection_manager_.stop(shared_from_this());
		}
	});
}

void connection::do_write() {
	auto self(shared_from_this());
	asio::async_write(socket_, reply_.to_buffers(), [this, self](std::error_code ec, std::size_t) {
		if (!ec) {
			// Initiate graceful connection closure.
			asio::error_code ignored_ec;
			socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
		}

		if (ec != asio::error::operation_aborted) {
			connection_manager_.stop(shared_from_this());
		}
	});
}

/// The top-level class of the HTTP server.
class server {
public:
	/// Construct the server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	explicit server(asio::io_context& context, uint16_t port, request_handler handler)
		: io_context_(context)
		, acceptor_(io_context_)
		, connection_manager_()
		, request_handler_(handler) {
		// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
		asio::ip::tcp::resolver resolver(io_context_);
		auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

		do_accept();
	}

	server(const server&) = delete;
	const server& operator=(const server&) = delete;

private:
	/// Perform an asynchronous accept operation.
	void do_accept() {
		acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (!acceptor_.is_open()) {
				return;
			}

			if (!ec) {
				connection_manager_.start(
					std::make_shared<connection>(std::move(socket), connection_manager_, request_handler_));
			}

			do_accept();
		});
	}

	/// The io_context used to perform asynchronous operations.
	asio::io_context& io_context_;

	/// Acceptor used to listen for incoming connections.
	asio::ip::tcp::acceptor acceptor_;

	/// The connection manager which owns all live connections.
	connection_manager connection_manager_;

	/// The handler for all incoming requests.
	request_handler request_handler_;
};

} // namespace http

} // namespace asyncio
