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

	if (argc < 2) {
		printf("port \n");
		return 0;
	}

	int port = std::atoi(argv[1]);
	
	if (!App::Instance()->Init(port))
		return 0;

	App::Instance()->Run();
	return 0;
}
