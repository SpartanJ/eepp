#include "eterminaldisplay.hpp"
#include <eepp/ee.hpp>

EE::Window::Window* win = NULL;
std::shared_ptr<ETerminalDisplay> terminal = nullptr;

void inputCallback( InputEvent* event ) {
	if ( !terminal )
		return;

	switch ( event->Type ) {
		case InputEvent::MouseMotion: {
			terminal->onMouseMotion( win->getInput()->getMousePos(),
									 win->getInput()->getPressTrigger() );
			break;
		}
		case InputEvent::MouseButtonDown: {
			terminal->onMouseDown( win->getInput()->getMousePos(),
								   win->getInput()->getPressTrigger() );
			break;
		}
		case InputEvent::MouseButtonUp: {
			terminal->onMouseUp( win->getInput()->getMousePos(),
								 win->getInput()->getReleaseTrigger() );
			break;
		}
		case InputEvent::Window: {
			break;
		}
		case InputEvent::KeyUp: {
			break;
		}
		case InputEvent::KeyDown: {
			terminal->onKeyDown( event->key.keysym.sym, event->key.keysym.unicode,
								 event->key.keysym.mod, event->key.keysym.scancode );
			break;
		}
		case InputEvent::TextInput: {
			terminal->onTextInput( event->text.text );
			break;
		}
		case InputEvent::VideoExpose:
			terminal->invalidate();
			break;
		case InputEvent::VideoResize: {
			terminal->setPosition( { 0, 0 } );
			terminal->setSize( win->getSize().asFloat() );
			break;
		}
	}
}

void mainLoop() {
	win->getInput()->update();
	terminal->Update();

	if ( terminal->isDirty() ) {
		win->clear();
		terminal->Draw( win->hasFocus() );
		win->display();
	} else {
		win->getInput()->waitEvent( Milliseconds( win->hasFocus() ? 16 : 100 ) );
	}
}

EE_MAIN_FUNC int main( int, char*[] ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Sizei winSize( 1280, 720 );
	win = Engine::instance()->createWindow(
		WindowSettings( winSize.getWidth(), winSize.getHeight(), "eterm", WindowStyle::Default,
						WindowBackend::Default, 32, "assets/icon/ee.png",
						currentDisplay->getPixelDensity() ),
		ContextSettings( true ) );

	if ( win->isOpen() ) {
		win->setClearColor( RGB( 0, 0, 0 ) );

		FontTrueType* fontMono = FontTrueType::New( "monospace" );
		fontMono->loadFromFile( "assets/fonts/DejaVuSansMono.ttf" );

		if ( !terminal || terminal->HasTerminated() ) {
			auto fontSize = PixelDensity::dpToPx( 12 );
			auto charWidth = fontMono->getGlyph( 'A', fontSize, false ).advance;
			auto charHeight = terminal ? terminal->getFontSize() : fontSize;
			Sizef contentRegion = win->getSize().asFloat();

			auto columns = (int)std::floor( std::max( 1.0f, contentRegion.x / charWidth ) );
			auto rows = (int)std::floor( std::max( 1.0f, contentRegion.y / charHeight ) );

			std::string shell;
			const char* shellenv = getenv( "SHELL" );
			if ( shellenv != nullptr ) {
				shell = shellenv;
			} else {
				shell = "/bin/bash";
			}

			terminal = ETerminalDisplay::Create( win, fontMono, columns, rows, shell, {}, "", 0 );
			terminal->setFontSize( charHeight );
			terminal->setSize( win->getSize().asFloat() );
		}

		win->getInput()->pushCallback( &inputCallback );

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}