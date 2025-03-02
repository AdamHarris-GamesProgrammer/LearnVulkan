#include <stdexcept>
#include <vector>

#include "Application.h"

///////////////////////////////////////////
int main() {
	Application app;
	app.Init(1920,1080, "Hello Triangle");

	try {
		app.Run();
	}
	catch (const std::exception& e) {
		printf(e.what() + '\n');
		return 1;
	}

	app.Cleanup();

	return 0;
}