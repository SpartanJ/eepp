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

bool App::tryTabClose( UICodeEditor* editor ) {
	if ( NULL != editor && editor->isDirty() ) {
		mMsgBox =
			UIMessageBox::New( UIMessageBox::OK_CANCEL,
							   "Do you really want to close this tab?\nAll changes will be lost." );
		mMsgBox->addEventListener( Event::MsgBoxConfirmClick,
								   [&, editor]( const Event* ) { closeEditorTab( editor ); } );
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
		closeEditorTab( editor );
		return true;
	}
}

void App::closeEditorTab( UICodeEditor* editor ) {
	if ( editor ) {
		UITabWidget* tabWidget = tabWidgetFromEditor( editor );
		if ( tabWidget ) {
			if ( !( editor->getDocument().isEmpty() &&
					!tabWidget->getParent()->isType( UI_TYPE_SPLITTER ) &&
					tabWidget->getTabCount() == 1 ) ) {
				tabWidget->removeTab( (UITab*)editor->getData() );
			}
		}
	}
}

void App::splitEditor( const SplitDirection& direction, UICodeEditor* editor ) {
	if ( !editor )
		return;
	UIOrientation orientation =
		direction == SplitDirection::Left || direction == SplitDirection::Right
			? UIOrientation::Horizontal
			: UIOrientation::Vertical;
	UITabWidget* tabWidget = tabWidgetFromEditor( editor );
	if ( !tabWidget )
		return;
	Node* parent = tabWidget->getParent();
	UISplitter* parentSplitter = NULL;
	bool wasFirst = true;

	if ( parent->isType( UI_TYPE_SPLITTER ) ) {
		parentSplitter = parent->asType<UISplitter>();
		wasFirst = parentSplitter->getFirstWidget() == tabWidget;
		if ( !parentSplitter->isFull() ) {
			parentSplitter->setOrientation( orientation );
			createEditorWithTabWidget( parentSplitter );
			if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
				parentSplitter->swap();
			return;
		}
	}

	UISplitter* splitter = UISplitter::New();
	splitter->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	splitter->setOrientation( orientation );
	tabWidget->detach();
	splitter->setParent( parent );
	tabWidget->setParent( splitter );
	createEditorWithTabWidget( splitter );
	if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
		splitter->swap();

	if ( parentSplitter ) {
		if ( wasFirst && parentSplitter->getFirstWidget() != splitter ) {
			parentSplitter->swap();
		} else if ( !wasFirst && parentSplitter->getLastWidget() != splitter ) {
			parentSplitter->swap();
		}
	}
	eeASSERT( mBaseLayout->getChildCount() <= 1 );
}

UICodeEditor* App::createCodeEditor() {
	UICodeEditor* codeEditor = UICodeEditor::New();
	codeEditor->setFontSize( 11 );
	codeEditor->getDocument().setCommand( "save-doc", [&, codeEditor] {
		if ( codeEditor->save() )
			updateEditorTitle( codeEditor );
	} );
	codeEditor->getDocument().setCommand( "find", [&] { findTextMessageBox(); } );
	codeEditor->getDocument().setCommand( "repeat-find", [&] { findText(); } );
	codeEditor->getDocument().setCommand( "close-app", [&] { closeApp(); } );
	codeEditor->getDocument().setCommand( "fullscreen-toggle",
										  [&]() { mWindow->toggleFullscreen(); } );
	codeEditor->getDocument().setCommand( "open-file", [&] { openFileDialog(); } );
	codeEditor->getDocument().setCommand( "console-toggle", [&] { mConsole->toggle(); } );
	codeEditor->getDocument().setCommand( "close-doc", [&] { tryTabClose( mCurEditor ); } );
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
	codeEditor->getDocument().setCommand(
		"split-right", [&] { splitEditor( SplitDirection::Right, mCurEditor ); } );
	codeEditor->getDocument().setCommand(
		"split-bottom", [&] { splitEditor( SplitDirection::Bottom, mCurEditor ); } );
	codeEditor->getDocument().setCommand(
		"split-left", [&] { splitEditor( SplitDirection::Left, mCurEditor ); } );
	codeEditor->getDocument().setCommand( "split-top",
										  [&] { splitEditor( SplitDirection::Top, mCurEditor ); } );
	codeEditor->getDocument().setCommand( "split-swap", [&] {
		if ( UISplitter* splitter = splitterFromEditor( mCurEditor ) )
			splitter->swap();
	} );
	codeEditor->addKeyBindingString( "escape", "close-app", true );
	codeEditor->addKeyBindingString( "f2", "open-file", true );
	codeEditor->addKeyBindingString( "f3", "repeat-find", false );
	codeEditor->addKeyBindingString( "f12", "console-toggle", true );
	codeEditor->addKeyBindingString( "alt+return", "fullscreen-toggle", true );
	codeEditor->addKeyBindingString( "ctrl+s", "save-doc", false );
	codeEditor->addKeyBindingString( "ctrl+f", "find", false );
	codeEditor->addKeyBindingString( "ctrl+q", "close-app", true );
	codeEditor->addKeyBindingString( "ctrl+o", "open-file", true );
	codeEditor->addKeyBindingString( "ctrl+l", "lock-toggle", true );
	codeEditor->addKeyBindingString( "ctrl+t", "create-new", true );
	codeEditor->addKeyBindingString( "ctrl+w", "close-doc", true );
	codeEditor->addKeyBindingString( "ctrl+tab", "next-doc", true );
	codeEditor->addKeyBindingString( "ctrl+shift+tab", "previous-doc", true );
	codeEditor->addKeyBindingString( "ctrl+shift+j", "split-left", true );
	codeEditor->addKeyBindingString( "ctrl+shift+l", "split-right", true );
	codeEditor->addKeyBindingString( "ctrl+shift+i", "split-top", true );
	codeEditor->addKeyBindingString( "ctrl+shift+k", "split-bottom", true );
	codeEditor->addKeyBindingString( "ctrl+shift+s", "split-swap", true );
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

void App::focusSomeEditor( Node* searchFrom ) {
	UICodeEditor* editor =
		searchFrom ? searchFrom->findByType<UICodeEditor>( UI_TYPE_CODEEDITOR )
				   : mUISceneNode->getRoot()->findByType<UICodeEditor>( UI_TYPE_CODEEDITOR );
	if ( searchFrom && !editor )
		editor = mUISceneNode->getRoot()->findByType<UICodeEditor>( UI_TYPE_CODEEDITOR );
	if ( editor && tabWidgetFromEditor( editor ) && !tabWidgetFromEditor( editor )->isClosing() ) {
		editor->setFocus();
	} else {
		UITabWidget* tabW = mUISceneNode->getRoot()->findByType<UITabWidget>( UI_TYPE_TABWIDGET );
		if ( tabW && tabW->getTabCount() > 0 ) {
			tabW->setTabSelected( tabW->getTabCount() - 1 );
		}
	}
}

void App::onTabClosed( const TabEvent* tabEvent ) {
	UICodeEditor* editor = mCurEditor;
	if ( tabEvent->getTab()->getOwnedWidget() == mCurEditor ) {
		mCurEditor = NULL;
	}
	UITabWidget* tabWidget = tabEvent->getTab()->getTabWidget();
	if ( tabWidget->getTabCount() == 0 ) {
		UISplitter* splitter = splitterFromEditor( editor );
		if ( splitter ) {
			if ( splitter->isFull() ) {
				tabWidget->close();
				// Remove splitter if it's redundant
				Node* parent = splitter->getParent();
				if ( parent->isType( UI_TYPE_SPLITTER ) ) {
					UISplitter* parentSplitter = parent->asType<UISplitter>();
					Node* remainingNode = tabWidget == splitter->getFirstWidget()
											  ? splitter->getLastWidget()
											  : splitter->getFirstWidget();
					bool wasFirst = parentSplitter->getFirstWidget() == splitter;
					remainingNode->detach();
					splitter->setParent( mUISceneNode->getRoot() );
					splitter->setVisible( false );
					splitter->setEnabled( false );
					splitter->close();
					remainingNode->setParent( parentSplitter );
					if ( wasFirst )
						parentSplitter->swap();
					focusSomeEditor( parentSplitter );
				} else {
					// Then this is the main splitter
					Node* remainingNode = tabWidget == splitter->getFirstWidget()
											  ? splitter->getLastWidget()
											  : splitter->getFirstWidget();
					splitter->setParent( mUISceneNode->getRoot() );
					splitter->setVisible( false );
					splitter->setEnabled( false );
					splitter->close();
					eeASSERT( parent->getChildCount() == 0 );
					remainingNode->setParent( parent );
					focusSomeEditor( NULL );
				}
				eeASSERT( mBaseLayout->getChildCount() == 1 );
				return;
			}
		}
		auto d = createCodeEditorInTabWidget( tabWidget );
		d.first->getTabWidget()->setTabSelected( d.first );
		d.second->setFocus();
	} else {
		tabWidget->setTabSelected( tabWidget->getTabCount() - 1 );
		tabWidget->getSelectedTab()->getOwnedWidget()->setFocus();
	}
}

std::pair<UITab*, UICodeEditor*> App::createCodeEditorInTabWidget( UITabWidget* tabWidget ) {
	if ( NULL == tabWidget )
		return std::make_pair( (UITab*)NULL, (UICodeEditor*)NULL );
	UICodeEditor* editor = createCodeEditor();
	UITab* tab = tabWidget->add( editor->getDocument().getFilename(), editor );
	editor->setData( (UintPtr)tab );
	tabWidget->addEventListener( Event::OnTabClosed, [&]( const Event* event ) {
		onTabClosed( static_cast<const TabEvent*>( event ) );
	} );
	return std::make_pair( tab, editor );
}

UITabWidget* App::createEditorWithTabWidget( Node* parent ) {
	UITabWidget* tabWidget = UITabWidget::New();
	tabWidget->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	tabWidget->setParent( parent );
	tabWidget->setTabsClosable( true );
	tabWidget->setHideTabBarOnSingleTab( true );
	tabWidget->setAllowRearrangeTabs( true );
	tabWidget->addEventListener( Event::OnTabSelected, [&]( const Event* event ) {
		UITabWidget* tabWidget = event->getNode()->asType<UITabWidget>();
		mCurEditor = tabWidget->getSelectedTab()->getOwnedWidget()->asType<UICodeEditor>();
		updateEditorTitle( mCurEditor );
		mCurEditor->setFocus();
	} );
	tabWidget->setTabTryCloseCallback( [&]( UITab* tab ) -> bool {
		tryTabClose( tab->getOwnedWidget()->asType<UICodeEditor>() );
		return false;
	} );
	createCodeEditorInTabWidget( tabWidget );
	return tabWidget;
}

UITabWidget* App::tabWidgetFromEditor( UICodeEditor* editor ) {
	if ( editor )
		return ( (UITab*)editor->getData() )->getTabWidget();
	return NULL;
}

UISplitter* App::splitterFromEditor( UICodeEditor* editor ) {
	if ( editor && editor->getParent()->getParent()->getParent()->isType( UI_TYPE_SPLITTER ) )
		return editor->getParent()->getParent()->getParent()->asType<UISplitter>();
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
	if ( !mCurEditor || text.empty() )
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
	if ( !mCurEditor )
		return;
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

		Font* font =
			FontTrueType::New( "NotoSans-Regular", resPath + "assets/fonts/NotoSans-Regular.ttf" );

		Font* fontMono =
			FontTrueType::New( "monospace", resPath + "assets/fonts/DejaVuSansMono.ttf" );

		mUISceneNode->getUIThemeManager()->setDefaultFont( font );

		SceneManager::instance()->add( mUISceneNode );

		StyleSheetParser cssParser;
		if ( cssParser.loadFromFile( resPath + "assets/ui/breeze.css" ) ) {
			mUISceneNode->setStyleSheet( cssParser.getStyleSheet() );
		}

		mUISceneNode->getRoot()->addClass( "appbackground" );

		mBaseLayout = UIRelativeLayout::New();
		mBaseLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

		createEditorWithTabWidget( mBaseLayout );

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
