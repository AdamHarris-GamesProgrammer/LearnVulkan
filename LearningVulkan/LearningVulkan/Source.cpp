#include <cstdlib>
#include <iostream>
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
		std::cerr << e.what() << std::endl;
		return 1;
	}

	app.Cleanup();

	return 0;
}