#ifdef _WIN32
#	include <windows.h>
#else
#	include <signal.h>
#endif
#include "app.h"

#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD CtrlType) {
	switch (CtrlType) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		App::Instance()->Stop();
		break;

	default:
		break;
	}

	return TRUE;
}
#else
void signal_hander(int signo) //自定义一个函数处理信号
{
	printf("catch a signal:%d\n:", signo);
	App::Instance()->Stop();
}
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);
#else
	signal(SIGINT, signal_hander);
	signal(SIGTERM, signal_hander);
	signal(SIGKILL, signal_hander);
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
