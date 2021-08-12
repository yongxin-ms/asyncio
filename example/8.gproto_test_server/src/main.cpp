#include "app.h"

int main(int argc, char* argv[]) {
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
