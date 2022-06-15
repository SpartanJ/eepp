#include "eterminaldisplay.hpp"
#include <eepp/ee.hpp>

EE::Window::Window* win = NULL;
std::shared_ptr<ETerminalDisplay> terminal = nullptr;

void tryInitTerminal( Font* fontDefault ) {
	if ( !terminal || terminal->HasTerminated() ) {
		auto spacingChar = fontDefault->getGlyph( 'A', 12, false );
		auto charWidth = spacingChar.advance;
		auto charHeight = 12;
		Sizef contentRegion = win->getSize().asFloat();

		auto columns = (int)std::floor( std::max( 1.0f, contentRegion.x / charWidth ) );
		auto rows = (int)std::floor( std::max( 1.0f, contentRegion.y / charHeight ) );

		terminal =
			ETerminalDisplay::Create( win, fontDefault, columns, rows, "/bin/sh", {}, "", 0 );
	}
}

void mainLoop() {
	win->clear();

	win->getInput()->update();

	if ( win->getInput()->isKeyDown( KEY_ESCAPE ) ) {
		win->close();
	}

	if ( terminal ) {
		terminal->Update();

		terminal->Draw( Rectf( { 0, 0 }, win->getSize().asFloat() ) );
	}

	win->display();
}

EE_MAIN_FUNC int main( int, char*[] ) {
	win = Engine::instance()->createWindow( WindowSettings( 960, 640, "eepp - Empty Window" ),
											ContextSettings( true ) );

	if ( win->isOpen() ) {
		win->setClearColor( RGB( 50, 50, 50 ) );

		FontTrueType* fontMono = FontTrueType::New( "monospace" );
		fontMono->loadFromFile( "assets/fonts/DejaVuSansMono.ttf" );

		tryInitTerminal( fontMono );

		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
