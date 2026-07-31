#include "app.hpp"

EE_MAIN_FUNC int main(int argc, char *argv[]) {
	App * MyApp = eeNew( App, ( argc, argv ) );

	MyApp->process();

	eeDelete( MyApp );

	Engine::destroySingleton();

	EE::MemoryManager::showResults();

	return 0;
}
