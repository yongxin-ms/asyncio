#include <signal.h>
#include "app.h"

int main(int argc, char* argv[]) {
	auto exit = [](int) { App::Instance()->Stop(); };
	::signal(SIGINT, exit);
	::signal(SIGABRT, exit);
	::signal(SIGSEGV, exit);
	::signal(SIGTERM, exit);
#if defined(_WIN32)
	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);
	::signal(SIGBREAK, exit);
#else
	::signal(SIGKILL, exit);
	::signal(SIGHUP, exit);
#endif

	if (argc < 4) {
		printf("ip port cli_num\n");
		return 0;
	}

	std::string ip = argv[1];
	int port = std::atoi(argv[2]);
	int cli_num = std::atoi(argv[3]);

	App::Instance()->Init(ip, port, cli_num);
	App::Instance()->Run();
	return 0;
}
