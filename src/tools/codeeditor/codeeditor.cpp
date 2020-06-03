#include "codeeditor.hpp"
#include <args/args.hxx>

App* appInstance = NULL;

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( NULL != mCurEditor && mCurEditor->isDirty() ) {
		mMsgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the code editor?\nAll changes will be lost." );
		mMsgBox->addEventListener( Event::MsgBoxConfirmClick,
								   [&]( const Event* ) { mWindow->close(); } );
		mMsgBox->addEventListener( Event::OnClose, [&]( const Event* ) { mMsgBox = NULL; } );
		mMsgBox->setTitle( "Close Code Editor?" );
		mMsgBox->center();
		mMsgBox->show();
		return false;
	} else {
		return true;
	}
}

bool App::onTabCloseRequestCallback( EE::Window::Window* ) {
	if ( NULL != mCurEditor && mCurEditor->isDirty() ) {
		mMsgBox =
			UIMessageBox::New( UIMessageBox::OK_CANCEL,
							   "Do you really want to close this tab?\nAll changes will be lost." );
		mMsgBox->addEventListener( Event::MsgBoxConfirmClick,
								   [&]( const Event* ) { closeCurrrentTab(); } );
		mMsgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
			mMsgBox = NULL;
			if ( mCurEditor )
				mCurEditor->setFocus();
		} );
		mMsgBox->setTitle( "Close Editor Tab?" );
		mMsgBox->center();
		mMsgBox->show();
		return false;
	} else {
		return true;
	}
}

void App::closeCurrrentTab() {
	if ( mCurEditor ) {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget ) {
			tabWidget->removeTab( (UITab*)mCurEditor->getData() );
			if ( tabWidget->getTabCount() > 0 ) {
				tabWidget->setTabSelected( tabWidget->getTabCount() - 1 );
				tabWidget->getSelectedTab()->getOwnedWidget()->setFocus();
			}
		}
	}
}

void App::splitEditor( const UIOrientation& orientation ) {
	UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
	if ( tabWidget ) {
		UISplitter* splitter = (UISplitter*)tabWidget->getParent();
		if ( splitter->getChildCount() < 3 ) {
			splitter->setOrientation( orientation );
			createEditorWithSplitter( splitter );
		}
	}
}

UICodeEditor* App::createCodeEditor() {
	UICodeEditor* codeEditor = UICodeEditor::New();
	codeEditor->setFontSize( 11 );
	codeEditor->getDocument().setCommand( "find", [&]() { findTextMessageBox(); } );
	codeEditor->getDocument().setCommand( "repeat-find", [&] { findText(); } );
	codeEditor->getDocument().setCommand( "close-app", [&] { closeApp(); } );
	codeEditor->getDocument().setCommand( "fullscreen-toggle",
										  [&]() { mWindow->toggleFullscreen(); } );
	codeEditor->getDocument().setCommand( "open-file", [&] { openFileDialog(); } );
	codeEditor->getDocument().setCommand( "console-toggle", [&] { mConsole->toggle(); } );
	codeEditor->getDocument().setCommand( "close-doc", [&] {
		if ( onTabCloseRequestCallback( mWindow ) ) {
			closeCurrrentTab();
		}
	} );
	codeEditor->getDocument().setCommand( "create-new", [&] {
		auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
		d.first->getTabWidget()->setTabSelected( d.first );
		d.second->setFocus();
	} );
	codeEditor->getDocument().setCommand( "next-doc", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mCurEditor->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			if ( tabIndex + 1 == tabWidget->getTabCount() ) {
				tab = tabWidget->setTabSelected( (Uint32)0 );
			} else {
				tab = tabWidget->setTabSelected( ( Uint32 )( tabIndex + 1 ) );
			}
			if ( tab )
				tab->getOwnedWidget()->setFocus();
		}
	} );
	codeEditor->getDocument().setCommand( "previous-doc", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mCurEditor->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			if ( tabIndex == 0 ) {
				tab = tabWidget->setTabSelected( (Uint32)tabWidget->getTabCount() - 1 );
			} else {
				tab = tabWidget->setTabSelected( ( Uint32 )( tabIndex - 1 ) );
			}
			if ( tab )
				tab->getOwnedWidget()->setFocus();
		}
	} );
	codeEditor->getDocument().setCommand( "split-horizontal",
										  [&] { splitEditor( UIOrientation::Horizontal ); } );
	codeEditor->getDocument().setCommand( "split-vertical",
										  [&] { splitEditor( UIOrientation::Vertical ); } );
	codeEditor->addKeyBindingString( "escape", "close-app", true );
	codeEditor->addKeyBindingString( "f2", "open-file", true );
	codeEditor->addKeyBindingString( "f3", "repeat-find", false );
	codeEditor->addKeyBindingString( "f12", "console-toggle", true );
	codeEditor->addKeyBindingString( "alt+return", "fullscreen-toggle", true );
	codeEditor->addKeyBindingString( "ctrl+s", "save", false );
	codeEditor->addKeyBindingString( "ctrl+f", "find", false );
	codeEditor->addKeyBindingString( "ctrl+q", "close-app", true );
	codeEditor->addKeyBindingString( "ctrl+o", "open-file", true );
	codeEditor->addKeyBindingString( "ctrl+l", "lock-toggle", true );
	codeEditor->addKeyBindingString( "ctrl+t", "create-new", true );
	codeEditor->addKeyBindingString( "ctrl+w", "close-doc", true );
	codeEditor->addKeyBindingString( "ctrl+tab", "next-doc", true );
	codeEditor->addKeyBindingString( "ctrl+shift+tab", "previous-doc", true );
	codeEditor->addKeyBindingString( "ctrl+shift+l", "split-horizontal", true );
	codeEditor->addKeyBindingString( "ctrl+shift+k", "split-vertical", true );
	codeEditor->addEventListener( Event::OnFocus, [&]( const Event* event ) {
		mCurEditor = event->getNode()->asType<UICodeEditor>();
		updateEditorTitle( mCurEditor );
	} );
	codeEditor->addEventListener( Event::OnTextChanged, [&]( const Event* event ) {
		updateEditorTitle( event->getNode()->asType<UICodeEditor>() );
	} );
	codeEditor->addEventListener( Event::OnSelectionChanged, [&]( const Event* event ) {
		updateEditorTitle( event->getNode()->asType<UICodeEditor>() );
	} );
	if ( NULL == mCurEditor ) {
		mCurEditor = codeEditor;
	}
	return codeEditor;
}

std::string App::titleFromEditor( UICodeEditor* editor ) {
	std::string title( editor->getDocument().getFilename() );
	return editor->getDocument().isDirty() ? title + "*" : title;
}

void App::updateEditorTitle( UICodeEditor* editor ) {
	std::string title( titleFromEditor( editor ) );
	if ( editor->getData() ) {
		UITab* tab = (UITab*)editor->getData();
		tab->setText( title );
	}
	setAppTitle( title );
}

std::pair<UITab*, UICodeEditor*> App::createCodeEditorInTabWidget( UITabWidget* tabWidget ) {
	if ( NULL == tabWidget )
		return std::make_pair( (UITab*)NULL, (UICodeEditor*)NULL );
	UICodeEditor* editor = createCodeEditor();
	UITab* tab = tabWidget->add( editor->getDocument().getFilename(), editor );
	editor->setData( (UintPtr)tab );
	tabWidget->addEventListener( Event::OnTabClosed, [&]( const Event* event ) {
		const TabEvent* tabEvent = static_cast<const TabEvent*>( event );
		if ( tabEvent->getTab()->getOwnedWidget() == mCurEditor ) {
			mCurEditor = NULL;
		}
		UITabWidget* tabWidget = tabEvent->getTab()->getTabWidget();
		if ( tabWidget->getTabCount() == 0 ) {
			auto d = createCodeEditorInTabWidget( tabWidget );
			d.first->getTabWidget()->setTabSelected( d.first );
			d.second->setFocus();
		}
	} );
	return std::make_pair( tab, editor );
}

UITabWidget* App::createEditorWithTabWidget( UISplitter* splitter ) {
	UITabWidget* tabWidget = UITabWidget::New();
	tabWidget->setParent( splitter );
	tabWidget->setTabsClosable( true );
	tabWidget->addEventListener( Event::OnTabSelected, [&]( const Event* event ) {
		UITabWidget* tabWidget = event->getNode()->asType<UITabWidget>();
		mCurEditor = tabWidget->getSelectedTab()->getOwnedWidget()->asType<UICodeEditor>();
		updateEditorTitle( mCurEditor );
		mCurEditor->setFocus();
	} );
	createCodeEditorInTabWidget( tabWidget );
	return tabWidget;
}

UISplitter* App::createEditorWithSplitter( Node* parent ) {
	if ( NULL == parent )
		return NULL;
	UISplitter* splitter = UISplitter::New();
	splitter->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	splitter->setParent( parent );
	auto d = createEditorWithTabWidget( splitter );
	d->getSelectedTab()->getOwnedWidget()->setFocus();
	return splitter;
}

UITabWidget* App::tabWidgetFromEditor( UICodeEditor* editor ) {
	if ( editor )
		return ( (UITab*)editor->getData() )->getTabWidget();
	return NULL;
}

void App::setAppTitle( const std::string& title ) {
	mWindow->setTitle( mWindowTitle + String( title.empty() ? "" : " - " + title ) );
}

void App::loadFileFromPath( const std::string& path, UICodeEditor* codeEditor ) {
	if ( NULL == codeEditor )
		codeEditor = mCurEditor;
	codeEditor->loadFromFile( path );
	updateEditorTitle( codeEditor );
}

void App::openFileDialog() {
	UICommonDialog* TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, "*" );
	TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	TGDialog->setTitle( "Open layout..." );
	TGDialog->setCloseWithKey( KEY_ESCAPE );
	TGDialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
		UITabWidget* tabWidget = d.first->getTabWidget();
		UITab* addedTab = d.first;
		loadFileFromPath( event->getNode()->asType<UICommonDialog>()->getFullPath(), d.second );
		tabWidget->setTabSelected( addedTab );
		UITab* firstTab = tabWidget->getTab( 0 );
		if ( addedTab != firstTab ) {
			UICodeEditor* editor = (UICodeEditor*)firstTab->getOwnedWidget();
			if ( editor->getDocument().isEmpty() ) {
				tabWidget->removeTab( firstTab );
			}
		}
	} );
	TGDialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mCurEditor )
			mCurEditor->setFocus();
	} );
	TGDialog->center();
	TGDialog->show();
}

void App::findText( String text ) {
	if ( text.empty() )
		text = mLastSearch;
	if ( text.empty() )
		return;
	mLastSearch = text;
	TextDocument& doc = mCurEditor->getDocument();
	TextPosition found = doc.find( text, doc.getSelection( true ).end() );
	if ( found.isValid() ) {
		doc.setSelection( {{found.line(), found.column() + (Int64)text.size()}, found} );
	} else {
		found = doc.find( text, {0, 0} );
		if ( found.isValid() ) {
			doc.setSelection( {{found.line(), found.column() + (Int64)text.size()}, found} );
		}
	}
}

void App::findTextMessageBox() {
	UIMessageBox* inputSearch = UIMessageBox::New( UIMessageBox::INPUT, "Find text..." );
	inputSearch->getTextInput()->setHint( "Find text..." );
	String text = mCurEditor->getDocument().getSelectedText();
	if ( !text.empty() )
		inputSearch->getTextInput()->setText( text );
	inputSearch->setCloseWithKey( KEY_ESCAPE );
	inputSearch->addEventListener( Event::MsgBoxConfirmClick, [&]( const Event* event ) {
		findText( event->getNode()->asType<UIMessageBox>()->getTextInput()->getText() );
	} );
	inputSearch->addEventListener( Event::OnClose,
								   [&]( const Event* ) { mCurEditor->setFocus(); } );
	inputSearch->setTitle( "Find" );
	inputSearch->getButtonOK()->setText( "Search" );
	inputSearch->center();
	inputSearch->show();
}

void App::closeApp() {
	if ( NULL == mMsgBox && onCloseRequestCallback( mWindow ) ) {
		mWindow->close();
	}
}

void App::mainLoop() {
	Input* input = mWindow->getInput();

	input->update();

	if ( input->isKeyUp( KEY_F6 ) ) {
		mUISceneNode->setHighlightFocus( !mUISceneNode->getHighlightFocus() );
		mUISceneNode->setHighlightOver( !mUISceneNode->getHighlightOver() );
	}

	if ( input->isKeyUp( KEY_F7 ) ) {
		mUISceneNode->setDrawBoxes( !mUISceneNode->getDrawBoxes() );
	}

	if ( input->isKeyUp( KEY_F8 ) ) {
		mUISceneNode->setDrawDebugData( !mUISceneNode->getDrawDebugData() );
	}

	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() || mConsole->isActive() ||
		 mConsole->isFading() ) {
		mWindow->clear();
		SceneManager::instance()->draw();
		mConsole->draw();
		mWindow->display();
	} else {
		Sys::sleep( Milliseconds( mWindow->isVisible() ? 1 : 16 ) );
	}
}

void App::onFileDropped( String file ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor = mCurEditor;
	if ( node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( !codeEditor->getDocument().isEmpty() ) {
			auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( codeEditor ) );
			codeEditor = d.second;
			d.first->getTabWidget()->setTabSelected( d.first );
		}
	}
	loadFileFromPath( file, codeEditor );
}

void App::onTextDropped( String text ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor = mCurEditor;
	if ( node->isType( UI_TYPE_CODEEDITOR ) )
		codeEditor = node->asType<UICodeEditor>();
	if ( codeEditor && !text.empty() ) {
		if ( text[text.size() - 1] != '\n' )
			text += '\n';
		codeEditor->getDocument().textInput( text );
	}
}

App::~App() {
	eeSAFE_DELETE( mConsole );
}

void App::init( const std::string& file ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	Float pixelDensity = currentDisplay->getPixelDensity();

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();

	std::string resPath( Sys::getProcessPath() );

	mWindow = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, mWindowTitle, WindowStyle::Default, WindowBackend::Default, 32,
						resPath + "assets/icon/ee.png", pixelDensity ),
		ContextSettings( true ) );

	if ( mWindow->isOpen() ) {
		mWindow->setCloseRequestCallback(
			[&]( auto* win ) -> bool { return onCloseRequestCallback( win ); } );

		mWindow->getInput()->pushCallback( [&]( InputEvent* event ) {
			if ( event->Type == InputEvent::FileDropped ) {
				onFileDropped( event->file.file );
			} else if ( event->Type == InputEvent::TextDropped ) {
				onTextDropped( event->textdrop.text );
			}
		} );

		PixelDensity::setPixelDensity( eemax( mWindow->getScale(), pixelDensity ) );

		mUISceneNode = UISceneNode::New();

		mUISceneNode->getUIThemeManager()->setDefaultFont( FontTrueType::New(
			"NotoSans-Regular", resPath + "assets/fonts/NotoSans-Regular.ttf" ) );

		Font* fontMono =
			FontTrueType::New( "monospace", resPath + "assets/fonts/DejaVuSansMono.ttf" );

		SceneManager::instance()->add( mUISceneNode );

		StyleSheetParser cssParser;
		if ( cssParser.loadFromFile( resPath + "assets/ui/breeze.css" ) ) {
			mUISceneNode->setStyleSheet( cssParser.getStyleSheet() );
		}

		mUISceneNode->getRoot()->addClass( "appbackground" );

		mBaseLayout = UILinearLayout::NewVertical();
		mBaseLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
		UISplitter* splitter = UISplitter::New();
		splitter->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
		splitter->setParent( mBaseLayout );

		createEditorWithSplitter( splitter );

		if ( !file.empty() ) {
			loadFileFromPath( file );
		}

		mConsole = eeNew( Console, ( fontMono, true, true, 1024 * 1000, 0, mWindow ) );

		mWindow->runMainLoop( &appLoop );
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

	appInstance = eeNew( App, () );
	appInstance->init( file.Get() );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
