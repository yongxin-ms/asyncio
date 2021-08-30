#include "asyncio.h"
#include "singleton.h"

int main() {
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
