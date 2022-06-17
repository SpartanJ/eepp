#include "eterminaldisplay.hpp"
#include <eepp/ee.hpp>

EE::Window::Window* win = NULL;
std::shared_ptr<ETerminalDisplay> terminal = nullptr;

void inputCallback( InputEvent* event ) {
	if ( !terminal )
		return;

	switch ( event->Type ) {
		case InputEvent::Window: {
			break;
		}
		case InputEvent::KeyUp:

			break;
		case InputEvent::KeyDown:
			terminal->onKeyDown( event->key.keysym.sym, event->key.keysym.unicode,
								 event->key.keysym.mod, event->key.keysym.scancode );
			break;
		case InputEvent::TextInput:
			terminal->onTextInput( event->text.text );
		case InputEvent::SysWM:
		case InputEvent::VideoResize:
		case InputEvent::VideoExpose: {
		}
	}
}

void tryInitTerminal( Font* fontDefault ) {
	if ( !terminal || terminal->HasTerminated() ) {
		auto fontSize = 15;
		auto charWidth = fontDefault->getGlyph( 'A', fontSize, false ).advance;
		auto charHeight = terminal ? terminal->getFontSize() : fontSize;
		Sizef contentRegion = win->getSize().asFloat();

		auto columns = (int)std::floor( std::max( 1.0f, contentRegion.x / charWidth ) );
		auto rows = (int)std::floor( std::max( 1.0f, contentRegion.y / charHeight ) );

		terminal =
			ETerminalDisplay::Create( win, fontDefault, columns, rows, "/usr/bin/fish", {}, "", 0 );
		terminal->setFontSize( charHeight );
	}
}

void mainLoop() {
	win->clear();

	win->getInput()->update();

	if ( terminal ) {
		terminal->Update();

		terminal->Draw( Rectf( { 0, 0 }, win->getSize().asFloat() ) );
	}

	win->display();
}

EE_MAIN_FUNC int main( int, char*[] ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	win = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, "eterm", WindowStyle::Default, WindowBackend::Default, 32,
						"assets/icon/ee.png" ),
		ContextSettings( true ) );

	if ( win->isOpen() ) {
		win->setClearColor( RGB( 50, 50, 50 ) );

		FontTrueType* fontMono = FontTrueType::New( "monospace" );
		fontMono->loadFromFile( "assets/fonts/DejaVuSansMono.ttf" );

		tryInitTerminal( fontMono );

		win->getInput()->pushCallback( &inputCallback );

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
