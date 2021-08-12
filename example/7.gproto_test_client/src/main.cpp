#include "app.h"

int main(int argc, char* argv[]) {
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
