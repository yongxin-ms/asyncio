#include <asyncio.h>
#include "singleton.h"

int main() {
	asyncio::SetLogHandler(
		[](asyncio::Log::LogLevel lv, const char* msg) {
			std::string time_now = asyncio::util::Time::FormatDateTime(std::chrono::system_clock::now());
			switch (lv) {
			case asyncio::Log::kError:
				printf("%s Error: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kWarning:
				printf("%s Warning: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kInfo:
				printf("%s Info: %s\n", time_now.c_str(), msg);
				break;
			case asyncio::Log::kDebug:
				printf("%s Debug: %s\n", time_now.c_str(), msg);
				break;
			default:
				break;
			}
		},
		asyncio::Log::kDebug);

	int port = 9000;
	asyncio::EventLoop my_event_loop;
	auto http_server = my_event_loop.CreateHttpServer(port, [](asyncio::http::connection_ptr conn) {
		const auto& cmd = conn->get_req().action;
		const auto& body = conn->get_req().body;
		ASYNCIO_LOG_DEBUG("GmCmd:%s, Body:%s", cmd.data(), body.data());

		conn->get_rep().content = "123456";
		conn->do_write();
	});

	my_event_loop.RunForever();
	return 0;
}
