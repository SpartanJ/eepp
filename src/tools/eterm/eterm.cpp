#include "terminal/terminaldisplay.hpp"
#include <eepp/ee.hpp>

EE::Window::Window* win = NULL;
std::shared_ptr<TerminalDisplay> terminal = nullptr;

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

			if ( win->getInput()->getDoubleClickTrigger() ) {
				terminal->onMouseDoubleClick( win->getInput()->getMousePos(),
											  win->getInput()->getDoubleClickTrigger() );
			}

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
			terminal->setFocus( win->hasFocus() );
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

	if ( terminal )
		terminal->update();

	if ( terminal && terminal->isDirty() ) {
		win->clear();
		terminal->draw();
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

		if ( !terminal || terminal->hasTerminated() ) {
			terminal = TerminalDisplay::create( win, fontMono, PixelDensity::dpToPx( 11 ),
												win->getSize().asFloat() );
		}

		win->getInput()->pushCallback( &inputCallback );

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
