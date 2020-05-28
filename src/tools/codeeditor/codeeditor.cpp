#include <args/args.hxx>
#include <eepp/ee.hpp>

EE::Window::Window* win = NULL;
UISceneNode* uiSceneNode = NULL;
UICodeEditor* codeEditor = NULL;
Console* console = NULL;
std::string curFile = "untitled";
const std::string& windowTitle = "eepp - Code Editor";
bool docDirtyState = false;
UIMessageBox* MsgBox = NULL;

bool onCloseRequestCallback( EE::Window::Window* ) {
	if ( NULL != codeEditor && codeEditor->isDirty() ) {
		MsgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the code editor?\nAll changes will be lost." );
		MsgBox->addEventListener( Event::MsgBoxConfirmClick, []( const Event* ) { win->close(); } );
		MsgBox->addEventListener( Event::OnClose, []( const Event* ) { MsgBox = NULL; } );
		MsgBox->setTitle( "Close Code Editor?" );
		MsgBox->center();
		MsgBox->show();
		return false;
	} else {
		return true;
	}
}

void setAppTitle( const std::string& title ) {
	win->setTitle( windowTitle + String( title.empty() ? "" : " - " + title ) );
}

void loadFileFromPath( const std::string& path ) {
	codeEditor->loadFromFile( path );
	curFile = FileSystem::fileNameFromPath( path );
	setAppTitle( curFile );
}

void openFileDialog() {
	UICommonDialog* TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, "*" );
	TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	TGDialog->setTitle( "Open layout..." );
	TGDialog->addEventListener( Event::OpenFile, []( const Event* event ) {
		loadFileFromPath( event->getNode()->asType<UICommonDialog>()->getFullPath() );
	} );
	TGDialog->center();
	TGDialog->show();
}

void mainLoop() {
	Input* input = win->getInput();

	input->update();

	if ( codeEditor->isDirty() != docDirtyState ) {
		docDirtyState = codeEditor->isDirty();
		setAppTitle( docDirtyState ? curFile + "*" : curFile );
	}

	if ( ( input->isControlPressed() && input->isKeyUp( KEY_O ) ) || input->isKeyUp( KEY_F2 ) ) {
		openFileDialog();
	}

	if ( input->isKeyUp( KEY_F6 ) ) {
		uiSceneNode->setHighlightFocus( !uiSceneNode->getHighlightFocus() );
		uiSceneNode->setHighlightOver( !uiSceneNode->getHighlightOver() );
	}

	if ( input->isKeyUp( KEY_F7 ) ) {
		uiSceneNode->setDrawBoxes( !uiSceneNode->getDrawBoxes() );
	}

	if ( input->isKeyUp( KEY_F8 ) ) {
		uiSceneNode->setDrawDebugData( !uiSceneNode->getDrawDebugData() );
	}

	if ( input->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox && onCloseRequestCallback( win ) ) {
		win->close();
	}

	if ( input->isAltPressed() && input->isKeyUp( KEY_RETURN ) ) {
		win->toggleFullscreen();
	}

	if ( input->isKeyUp( KEY_F3 ) ) {
		console->toggle();
	}

	if ( input->isControlPressed() && input->isKeyUp( KEY_L ) ) {
		codeEditor->setLocked( !codeEditor->isLocked() );
	}

	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() ) {
		win->clear();
		SceneManager::instance()->draw();
		console->draw();
		win->display();
	} else {
		Sys::sleep( Milliseconds( win->isVisible() ? 1 : 16 ) );
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	args::ArgumentParser parser( "eepp Code Editor" );
	args::Positional<std::string> file( parser, "file", "The file path" );

	try {
		parser.ParseCLI( argc, argv );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	Float pixelDensity = currentDisplay->getPixelDensity();

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();

	std::string resPath( Sys::getProcessPath() );

	win = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, windowTitle, WindowStyle::Default, WindowBackend::Default, 32,
						resPath + "assets/icon/ee.png", pixelDensity ),
		ContextSettings( true ) );

	if ( win->isOpen() ) {
		win->setCloseRequestCallback( cb::Make1( onCloseRequestCallback ) );

		win->getInput()->pushCallback( []( InputEvent* event ) {
			if ( NULL == codeEditor )
				return;

			if ( event->Type == InputEvent::FileDropped ) {
				loadFileFromPath( event->file.file );
			} else if ( event->Type == InputEvent::TextDropped ) {
				codeEditor->getDocument().textInput( event->textdrop.text );
			}
		} );

		PixelDensity::setPixelDensity( eemax( win->getScale(), pixelDensity ) );

		uiSceneNode = UISceneNode::New();

		uiSceneNode->getUIThemeManager()->setDefaultFont( FontTrueType::New(
			"NotoSans-Regular", resPath + "assets/fonts/NotoSans-Regular.ttf" ) );

		Font* fontMono =
			FontTrueType::New( "monospace", resPath + "assets/fonts/DejaVuSansMono.ttf" );

		SceneManager::instance()->add( uiSceneNode );

		StyleSheetParser cssParser;
		if ( cssParser.loadFromFile( resPath + "assets/ui/breeze.css" ) ) {
			uiSceneNode->setStyleSheet( cssParser.getStyleSheet() );
		}

		std::string layout = R"xml(
			<LinearLayout layout_width="match_parent"
						  layout_height="match_parent"
						  orientation="vertical">
				<CodeEditor id="code_edit"
					layout_width="match_parent"
					layout_height="match_parent"
					 />
			</LinearLayout>
		)xml";
		uiSceneNode->loadLayoutFromString( layout );

		uiSceneNode->bind( "code_edit", codeEditor );
		codeEditor->setFontSize( 11 );
		codeEditor->addKeyBindingString( "ctrl+s", "save" );

		if ( file ) {
			loadFileFromPath( file.Get() );
		}

		console = eeNew( Console, ( fontMono, true, true, 1024 * 1000, 0, win ) );

		win->runMainLoop( &mainLoop );
	}

	eeSAFE_DELETE( console );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
