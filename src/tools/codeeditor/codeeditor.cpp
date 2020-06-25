#include "codeeditor.hpp"
#include <args/args.hxx>

App* appInstance = nullptr;

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( nullptr != mCurEditor && mCurEditor->isDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the code editor?\nAll changes will be lost." );
		msgBox->addEventListener( Event::MsgBoxConfirmClick,
								  [&]( const Event* ) { mWindow->close(); } );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( "Close " + mWindowTitle + "?" );
		msgBox->center();
		msgBox->show();
		return false;
	} else {
		return true;
	}
}

bool App::tryTabClose( UICodeEditor* editor ) {
	if ( nullptr != editor && editor->isDirty() ) {
		UIMessageBox* msgBox =
			UIMessageBox::New( UIMessageBox::OK_CANCEL,
							   "Do you really want to close this tab?\nAll changes will be lost." );
		msgBox->addEventListener( Event::MsgBoxConfirmClick,
								  [&, editor]( const Event* ) { closeEditorTab( editor ); } );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
			msgBox = nullptr;
			if ( mCurEditor )
				mCurEditor->setFocus();
		} );
		msgBox->setTitle( "Close Tab?" );
		msgBox->center();
		msgBox->show();
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
	UISplitter* parentSplitter = nullptr;
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
}

void App::switchToTab( Int32 index ) {
	UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
	if ( tabWidget ) {
		tabWidget->setTabSelected( eeclamp<Int32>( index, 0, tabWidget->getTabCount() - 1 ) );
	}
}

UITabWidget* App::findPreviousSplit( UICodeEditor* editor ) {
	if ( !editor )
		return nullptr;
	UISplitter* splitter = splitterFromEditor( editor );
	if ( !splitter )
		return nullptr;
	UITabWidget* tabWidget = tabWidgetFromEditor( editor );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.rbegin(), mTabWidgets.rend(), tabWidget );
		if ( it != mTabWidgets.rend() && ++it != mTabWidgets.rend() ) {
			return *it;
		}
	}
	return nullptr;
}

void App::switchPreviousSplit( UICodeEditor* editor ) {
	UITabWidget* tabWidget = findPreviousSplit( editor );
	if ( tabWidget && tabWidget->getTabSelected() &&
		 tabWidget->getTabSelected()->getOwnedWidget() ) {
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} else {
		tabWidget = findNextSplit( editor );
		if ( tabWidget && tabWidget->getTabSelected() &&
			 tabWidget->getTabSelected()->getOwnedWidget() ) {
			tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
		}
	}
}

UITabWidget* App::findNextSplit( UICodeEditor* editor ) {
	if ( !editor )
		return nullptr;
	UISplitter* splitter = splitterFromEditor( editor );
	if ( !splitter )
		return nullptr;
	UITabWidget* tabWidget = tabWidgetFromEditor( editor );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
		if ( it != mTabWidgets.end() && ++it != mTabWidgets.end() ) {
			return *it;
		}
	}
	return nullptr;
}

void App::switchNextSplit( UICodeEditor* editor ) {
	UITabWidget* tabWidget = findNextSplit( editor );
	if ( tabWidget && tabWidget->getTabSelected() &&
		 tabWidget->getTabSelected()->getOwnedWidget() ) {
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} else {
		tabWidget = findPreviousSplit( editor );
		if ( tabWidget && tabWidget->getTabSelected() &&
			 tabWidget->getTabSelected()->getOwnedWidget() ) {
			tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
		}
	}
}

void App::applyColorScheme( const SyntaxColorScheme& colorScheme ) {
	for ( UITabWidget* tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			tabWidget->getTab( i )->getOwnedWidget()->asType<UICodeEditor>()->setColorScheme(
				colorScheme );
		}
	}
	updateColorSchemeMenu();
}

void App::saveDoc() {
	if ( mCurEditor->getDocument().hasFilepath() ) {
		if ( mCurEditor->save() )
			updateEditorState();
	} else {
		saveFileDialog();
	}
}

void App::forEachEditor( std::function<void( UICodeEditor* )> run ) {
	for ( auto tabWidget : mTabWidgets )
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ )
			run( tabWidget->getTab( i )->getOwnedWidget()->asType<UICodeEditor>() );
}

void App::zoomIn() {
	forEachEditor( []( UICodeEditor* editor ) { editor->fontSizeGrow(); } );
}

void App::zoomOut() {
	forEachEditor( []( UICodeEditor* editor ) { editor->fontSizeShrink(); } );
}

void App::zoomReset() {
	forEachEditor( []( UICodeEditor* editor ) { editor->fontSizeReset(); } );
}

UICodeEditor* App::createCodeEditor() {
	UICodeEditor* codeEditor = UICodeEditor::NewOpt( false, true );
	TextDocument& doc = codeEditor->getDocument();
	codeEditor->setFontSize( mConfig.editor.fontSize );
	codeEditor->setEnableColorPickerOnSelection( true );
	codeEditor->setColorScheme( mColorSchemes[mCurrentColorScheme] );
	codeEditor->setShowLineNumber( mConfig.editor.showLineNumbers );
	codeEditor->setShowWhitespaces( mConfig.editor.showWhiteSpaces );
	codeEditor->setHighlightMatchingBracket( mConfig.editor.highlightMatchingBracket );
	codeEditor->setHorizontalScrollBarEnabled( mConfig.editor.horizontalScrollbar );
	codeEditor->setHighlightCurrentLine( mConfig.editor.highlightCurrentLine );
	codeEditor->setTabWidth( mConfig.editor.tabWidth );
	codeEditor->setLineBreakingColumn( mConfig.editor.lineBreakingColumn );
	doc.setAutoDetectIndentType( mConfig.editor.autoDetectIndentType );
	doc.setLineEnding( mConfig.editor.windowsLineEndings ? TextDocument::LineEnding::CRLF
														 : TextDocument::LineEnding::LF );
	doc.setTrimTrailingWhitespaces( mConfig.editor.trimTrailingWhitespaces );
	doc.setIndentType( mConfig.editor.indentSpaces ? TextDocument::IndentType::IndentSpaces
												   : TextDocument::IndentType::IndentTabs );
	doc.setForceNewLineAtEndOfFile( mConfig.editor.forceNewLineAtEndOfFile );
	doc.setIndentWidth( mConfig.editor.indentWidth );
	doc.setBOM( mConfig.editor.writeUnicodeBOM );

	/* global commands */
	doc.setCommand( "move-to-previous-line", [&] {
		if ( mCurEditor )
			mCurEditor->moveToPreviousLine();
	} );
	doc.setCommand( "move-to-next-line", [&] {
		if ( mCurEditor )
			mCurEditor->moveToNextLine();
	} );
	doc.setCommand( "select-to-previous-line", [&] {
		if ( mCurEditor )
			mCurEditor->selectToPreviousLine();
	} );
	doc.setCommand( "select-to-next-line", [&] {
		if ( mCurEditor )
			mCurEditor->selectToNextLine();
	} );
	doc.setCommand( "move-scroll-up", [&] {
		if ( mCurEditor )
			mCurEditor->moveScrollUp();
	} );
	doc.setCommand( "move-scroll-down", [&] {
		if ( mCurEditor )
			mCurEditor->moveScrollDown();
	} );
	doc.setCommand( "indent", [&] {
		if ( mCurEditor )
			mCurEditor->indent();
	} );
	doc.setCommand( "unindent", [&] {
		if ( mCurEditor )
			mCurEditor->unindent();
	} );
	doc.setCommand( "copy", [&] {
		if ( mCurEditor )
			mCurEditor->copy();
	} );
	doc.setCommand( "cut", [&] {
		if ( mCurEditor )
			mCurEditor->cut();
	} );
	doc.setCommand( "paste", [&] {
		if ( mCurEditor )
			mCurEditor->paste();
	} );
	doc.setCommand( "font-size-grow", [&] { zoomIn(); } );
	doc.setCommand( "font-size-shrink", [&] { zoomOut(); } );
	doc.setCommand( "font-size-reset", [&] { zoomReset(); } );
	doc.setCommand( "lock", [&] {
		if ( mCurEditor ) {
			mCurEditor->setLocked( true );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "unlock", [&] {
		if ( mCurEditor ) {
			mCurEditor->setLocked( false );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "lock-toggle", [&] {
		if ( mCurEditor ) {
			mCurEditor->setLocked( !mCurEditor->isLocked() );
			updateDocumentMenu();
		}
	} );
	codeEditor->addUnlockedCommand( "copy" );
	codeEditor->addUnlockedCommand( "select-all" );
	/* global commands */

	doc.setCommand( "switch-to-previous-colorscheme", [&] {
		auto it = mColorSchemes.find( mCurrentColorScheme );
		auto prev = std::prev( it, 1 );
		if ( prev != mColorSchemes.end() ) {
			setColorScheme( prev->first );
		} else {
			setColorScheme( mColorSchemes.rbegin()->first );
		}
	} );
	doc.setCommand( "switch-to-next-colorscheme", [&] {
		auto it = mColorSchemes.find( mCurrentColorScheme );
		if ( ++it != mColorSchemes.end() )
			mCurrentColorScheme = it->first;
		else
			mCurrentColorScheme = mColorSchemes.begin()->first;
		applyColorScheme( mColorSchemes[mCurrentColorScheme] );
	} );
	doc.setCommand( "switch-to-previous-split", [&] { switchPreviousSplit( mCurEditor ); } );
	doc.setCommand( "switch-to-next-split", [&] { switchNextSplit( mCurEditor ); } );
	doc.setCommand( "save-doc", [&] { saveDoc(); } );
	doc.setCommand( "save-as-doc", [&] { saveFileDialog(); } );
	doc.setCommand( "find-replace", [&] { showFindView(); } );
	doc.setCommand( "repeat-find", [&] {
		findNextText( "", mSearchBarLayout->find<UICheckBox>( "case_sensitive" )->isChecked() );
	} );
	doc.setCommand( "close-app", [&] { closeApp(); } );
	doc.setCommand( "fullscreen-toggle", [&]() {
		mWindow->toggleFullscreen();
		mViewMenu->find( "fullscreen-mode" )
			->asType<UIMenuCheckBox>()
			->setActive( !mWindow->isWindowed() );
	} );
	doc.setCommand( "open-file", [&] { openFileDialog(); } );
	doc.setCommand( "console-toggle", [&] {
		mConsole->toggle();
		bool lock = mConsole->isActive();
		forEachEditor( [lock]( UICodeEditor* editor ) { editor->setLocked( lock ); } );
	} );
	doc.setCommand( "close-doc", [&] { tryTabClose( mCurEditor ); } );
	doc.setCommand( "create-new", [&] {
		auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
		d.first->getTabWidget()->setTabSelected( d.first );
	} );
	doc.setCommand( "next-doc", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mCurEditor->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			switchToTab( ( tabIndex + 1 ) % tabWidget->getTabCount() );
		}
	} );
	doc.setCommand( "previous-doc", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mCurEditor->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			Int32 newTabIndex = (Int32)tabIndex - 1;
			switchToTab( newTabIndex < 0 ? tabWidget->getTabCount() - newTabIndex : newTabIndex );
		}
	} );
	for ( int i = 1; i <= 10; i++ )
		doc.setCommand( String::format( "switch-to-tab-%d", i ), [&, i] { switchToTab( i - 1 ); } );
	doc.setCommand( "split-right", [&] { splitEditor( SplitDirection::Right, mCurEditor ); } );
	doc.setCommand( "split-bottom", [&] { splitEditor( SplitDirection::Bottom, mCurEditor ); } );
	doc.setCommand( "split-left", [&] { splitEditor( SplitDirection::Left, mCurEditor ); } );
	doc.setCommand( "split-top", [&] { splitEditor( SplitDirection::Top, mCurEditor ); } );
	doc.setCommand( "split-swap", [&] {
		if ( UISplitter* splitter = splitterFromEditor( mCurEditor ) )
			splitter->swap();
	} );

	codeEditor->addEventListener( Event::OnFocus, [&]( const Event* event ) {
		setCurrentEditor( event->getNode()->asType<UICodeEditor>() );
	} );
	codeEditor->addEventListener( Event::OnTextChanged, [&]( const Event* event ) {
		updateEditorTitle( event->getNode()->asType<UICodeEditor>() );
	} );
	codeEditor->addEventListener( Event::OnSelectionChanged, [&]( const Event* event ) {
		updateEditorTitle( event->getNode()->asType<UICodeEditor>() );
	} );

	codeEditor->addKeyBindingString( "f2", "open-file", true );
	codeEditor->addKeyBindingString( "f3", "repeat-find", false );
	codeEditor->addKeyBindingString( "f12", "console-toggle", true );
	codeEditor->addKeyBindingString( "alt+return", "fullscreen-toggle", true );
	codeEditor->addKeyBindingString( "alt+keypad enter", "fullscreen-toggle", true );
	codeEditor->addKeyBindingString( "ctrl+s", "save-doc", false );
	codeEditor->addKeyBindingString( "ctrl+f", "find-replace", false );
	codeEditor->addKeyBindingString( "ctrl+q", "close-app", true );
	codeEditor->addKeyBindingString( "ctrl+o", "open-file", true );
	codeEditor->addKeyBindingString( "ctrl+l", "lock-toggle", true );
	codeEditor->addKeyBindingString( "ctrl+t", "create-new", true );
	codeEditor->addKeyBindingString( "ctrl+w", "close-doc", true );
	codeEditor->addKeyBindingString( "ctrl+tab", "next-doc", true );
	codeEditor->addKeyBindingString( "ctrl+shift+tab", "previous-doc", true );
	codeEditor->addKeyBindingString( "alt+shift+j", "split-left", true );
	codeEditor->addKeyBindingString( "alt+shift+l", "split-right", true );
	codeEditor->addKeyBindingString( "alt+shift+i", "split-top", true );
	codeEditor->addKeyBindingString( "alt+shift+k", "split-bottom", true );
	codeEditor->addKeyBindingString( "alt+shift+s", "split-swap", true );
	codeEditor->addKeyBindingString( "ctrl+alt+j", "switch-to-previous-split", true );
	codeEditor->addKeyBindingString( "ctrl+alt+l", "switch-to-next-split", true );
	codeEditor->addKeyBindingString( "ctrl+alt+n", "switch-to-previous-colorscheme", true );
	codeEditor->addKeyBindingString( "ctrl+alt+m", "switch-to-next-colorscheme", true );

	for ( int i = 1; i <= 10; i++ ) {
		codeEditor->addKeyBindingString( String::format( "ctrl+%d", i ),
										 String::format( "switch-to-tab-%d", i ), true );
		codeEditor->addKeyBindingString( String::format( "alt+%d", i ),
										 String::format( "switch-to-tab-%d", i ), true );
	}

	if ( nullptr == mCurEditor ) {
		setCurrentEditor( codeEditor );
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
		UITabWidget* tabW = tabWidgetFromEditor( editor );
		if ( tabW && tabW->getTabCount() > 0 ) {
			tabW->setTabSelected( tabW->getTabSelected() );
		}
	} else {
		UITabWidget* tabW = mUISceneNode->getRoot()->findByType<UITabWidget>( UI_TYPE_TABWIDGET );
		if ( tabW && tabW->getTabCount() > 0 ) {
			tabW->setTabSelected( tabW->getTabSelected() );
		}
	}
}

void App::closeTabWidgets( UISplitter* splitter ) {
	Node* node = splitter->getFirstChild();
	while ( node ) {
		if ( node->isType( UI_TYPE_TABWIDGET ) ) {
			auto it =
				std::find( mTabWidgets.begin(), mTabWidgets.end(), node->asType<UITabWidget>() );
			if ( it != mTabWidgets.end() ) {
				mTabWidgets.erase( it );
			}
		} else if ( node->isType( UI_TYPE_SPLITTER ) ) {
			closeTabWidgets( node->asType<UISplitter>() );
		}
		node = node->getNextNode();
	}
}

void App::addRemainingTabWidgets( Node* widget ) {
	if ( widget->isType( UI_TYPE_TABWIDGET ) ) {
		if ( std::find( mTabWidgets.begin(), mTabWidgets.end(), widget->asType<UITabWidget>() ) ==
			 mTabWidgets.end() ) {
			mTabWidgets.push_back( widget->asType<UITabWidget>() );
		}
	} else if ( widget->isType( UI_TYPE_SPLITTER ) ) {
		UISplitter* splitter = widget->asType<UISplitter>();
		addRemainingTabWidgets( splitter->getFirstWidget() );
		addRemainingTabWidgets( splitter->getLastWidget() );
	}
}

void App::closeSplitter( UISplitter* splitter ) {
	splitter->setParent( mUISceneNode->getRoot() );
	splitter->setVisible( false );
	splitter->setEnabled( false );
	splitter->close();
	closeTabWidgets( splitter );
}

void App::onTabClosed( const TabEvent* tabEvent ) {
	UICodeEditor* editor = mCurEditor;
	if ( tabEvent->getTab()->getOwnedWidget() == mCurEditor ) {
		setCurrentEditor( nullptr );
	}
	UITabWidget* tabWidget = tabEvent->getTab()->getTabWidget();
	if ( tabWidget->getTabCount() == 0 ) {
		UISplitter* splitter = splitterFromEditor( editor );
		if ( splitter ) {
			if ( splitter->isFull() ) {
				tabWidget->close();
				auto itWidget = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
				if ( itWidget != mTabWidgets.end() ) {
					mTabWidgets.erase( itWidget );
				}

				// Remove splitter if it's redundant
				Node* parent = splitter->getParent();
				if ( parent->isType( UI_TYPE_SPLITTER ) ) {
					UISplitter* parentSplitter = parent->asType<UISplitter>();
					Node* remainingNode = tabWidget == splitter->getFirstWidget()
											  ? splitter->getLastWidget()
											  : splitter->getFirstWidget();
					bool wasFirst = parentSplitter->getFirstWidget() == splitter;
					remainingNode->detach();
					closeSplitter( splitter );
					remainingNode->setParent( parentSplitter );
					addRemainingTabWidgets( remainingNode );
					if ( wasFirst )
						parentSplitter->swap();
					focusSomeEditor( parentSplitter );
				} else {
					// Then this is the main splitter
					Node* remainingNode = tabWidget == splitter->getFirstWidget()
											  ? splitter->getLastWidget()
											  : splitter->getFirstWidget();
					closeSplitter( splitter );
					eeASSERT( parent->getChildCount() == 0 );
					remainingNode->setParent( parent );
					addRemainingTabWidgets( remainingNode );
					focusSomeEditor( nullptr );
				}
				return;
			}
		}
		auto d = createCodeEditorInTabWidget( tabWidget );
		d.first->getTabWidget()->setTabSelected( d.first );
	} else {
		tabWidget->setTabSelected( eemin( tabWidget->getTabCount() - 1, tabEvent->getTabIndex() ) );
	}
}

std::pair<UITab*, UICodeEditor*> App::createCodeEditorInTabWidget( UITabWidget* tabWidget ) {
	if ( nullptr == tabWidget )
		return std::make_pair( (UITab*)nullptr, (UICodeEditor*)nullptr );
	UICodeEditor* editor = createCodeEditor();
	editor->addEventListener( Event::OnDocumentChanged, [&]( const Event* event ) {
		updateEditorTitle( event->getNode()->asType<UICodeEditor>() );
	} );
	UITab* tab = tabWidget->add( editor->getDocument().getFilename(), editor );
	editor->setData( (UintPtr)tab );
	return std::make_pair( tab, editor );
}

void App::removeUnusedTab( UITabWidget* tabWidget ) {
	if ( tabWidget && tabWidget->getTabCount() == 2 &&
		 tabWidget->getTab( 0 )
			 ->getOwnedWidget()
			 ->asType<UICodeEditor>()
			 ->getDocument()
			 .isEmpty() ) {
		tabWidget->removeTab( (Uint32)0 );
	}
}

UITabWidget* App::createEditorWithTabWidget( Node* parent ) {
	UICodeEditor* prevCurEditor = mCurEditor;
	UITabWidget* tabWidget = UITabWidget::New();
	tabWidget->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	tabWidget->setParent( parent );
	tabWidget->setTabsClosable( true );
	tabWidget->setHideTabBarOnSingleTab( true );
	tabWidget->setAllowRearrangeTabs( true );
	tabWidget->setAllowDragAndDropTabs( true );
	tabWidget->addEventListener( Event::OnTabSelected, [&]( const Event* event ) {
		UITabWidget* tabWidget = event->getNode()->asType<UITabWidget>();
		setCurrentEditor( tabWidget->getTabSelected()->getOwnedWidget()->asType<UICodeEditor>() );
	} );
	tabWidget->setTabTryCloseCallback( [&]( UITab* tab ) -> bool {
		tryTabClose( tab->getOwnedWidget()->asType<UICodeEditor>() );
		return false;
	} );
	tabWidget->addEventListener( Event::OnTabClosed, [&]( const Event* event ) {
		onTabClosed( static_cast<const TabEvent*>( event ) );
	} );
	auto editorData = createCodeEditorInTabWidget( tabWidget );
	// Open same document in the new split
	if ( prevCurEditor && prevCurEditor != editorData.second &&
		 !prevCurEditor->getDocument().isEmpty() )
		editorData.second->setDocument( prevCurEditor->getDocumentRef() );
	mTabWidgets.push_back( tabWidget );
	return tabWidget;
}

UITabWidget* App::tabWidgetFromEditor( UICodeEditor* editor ) {
	if ( editor )
		return ( (UITab*)editor->getData() )->getTabWidget();
	return nullptr;
}

UISplitter* App::splitterFromEditor( UICodeEditor* editor ) {
	if ( editor && editor->getParent()->getParent()->getParent()->isType( UI_TYPE_SPLITTER ) )
		return editor->getParent()->getParent()->getParent()->asType<UISplitter>();
	return nullptr;
}

void App::setAppTitle( const std::string& title ) {
	mWindow->setTitle( mWindowTitle + String( title.empty() ? "" : " - " + title ) );
}

bool App::loadFileFromPath( const std::string& path, UICodeEditor* codeEditor ) {
	if ( FileSystem::isDirectory( path ) )
		return false;
	if ( nullptr == codeEditor )
		codeEditor = mCurEditor;
	codeEditor->setColorScheme( mColorSchemes[mCurrentColorScheme] );
	bool ret = codeEditor->loadFromFile( path );
	updateEditorTitle( codeEditor );
	if ( codeEditor == mCurEditor )
		updateCurrentFiletype();
	removeUnusedTab( tabWidgetFromEditor( codeEditor ) );

	auto found = std::find( mRecentFiles.begin(), mRecentFiles.end(), path );
	if ( found != mRecentFiles.end() )
		mRecentFiles.erase( found );
	mRecentFiles.insert( mRecentFiles.begin(), path );
	if ( mRecentFiles.size() > 10 )
		mRecentFiles.resize( 10 );
	updateRecentFiles();
	return ret;
}

void App::loadFileFromPathInNewTab( const std::string& path ) {
	auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
	UITabWidget* tabWidget = d.first->getTabWidget();
	UITab* addedTab = d.first;
	loadFileFromPath( path, d.second );
	tabWidget->setTabSelected( addedTab );
}

void App::setCurrentEditor( UICodeEditor* editor ) {
	mCurEditor = editor;
	updateEditorState();
}

void App::openFileDialog() {
	UIFileDialog* dialog = UIFileDialog::New();
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Open File" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		loadFileFromPathInNewTab( event->getNode()->asType<UIFileDialog>()->getFullPath() );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mCurEditor && !SceneManager::instance()->isShootingDown() )
			mCurEditor->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::saveFileDialog() {
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Save File As" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string filename( mCurEditor->getDocument().getFilename() );
	if ( FileSystem::fileExtension( mCurEditor->getDocument().getFilename() ).empty() )
		filename += mCurEditor->getSyntaxDefinition().getFileExtension();
	dialog->setFileName( filename );
	dialog->addEventListener( Event::SaveFile, [&]( const Event* event ) {
		if ( mCurEditor ) {
			std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
			if ( !path.empty() && !FileSystem::isDirectory( path ) &&
				 FileSystem::fileCanWrite( FileSystem::fileRemoveFileName( path ) ) ) {
				if ( mCurEditor->getDocument().save( path ) ) {
					updateEditorState();
				} else {
					UIMessageBox* msg =
						UIMessageBox::New( UIMessageBox::OK, "Couldn't write the file." );
					msg->setTitle( "Error" );
					msg->show();
				}
			} else {
				UIMessageBox* msg =
					UIMessageBox::New( UIMessageBox::OK, "You must set a name to the file." );
				msg->setTitle( "Error" );
				msg->show();
			}
		}
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mCurEditor && !SceneManager::instance()->isShootingDown() )
			mCurEditor->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::findPrevText( String text, const bool& caseSensitive ) {
	if ( text.empty() )
		text = mLastSearch;
	if ( !mCurEditor || text.empty() )
		return;
	mLastSearch = text;
	TextDocument& doc = mCurEditor->getDocument();
	TextPosition found = doc.findLast( text, doc.getSelection( true ).start(), caseSensitive );
	if ( found.isValid() ) {
		doc.setSelection( {doc.positionOffset( found, text.size() ), found} );
	} else {
		found = doc.findLast( text, doc.endOfDoc() );
		if ( found.isValid() ) {
			doc.setSelection( {doc.positionOffset( found, text.size() ), found} );
		}
	}
}

void App::findNextText( String text, const bool& caseSensitive ) {
	if ( text.empty() )
		text = mLastSearch;
	if ( !mCurEditor || text.empty() )
		return;
	mLastSearch = text;
	TextDocument& doc = mCurEditor->getDocument();
	TextPosition found = doc.find( text, doc.getSelection( true ).end(), caseSensitive );
	if ( found.isValid() ) {
		doc.setSelection( {doc.positionOffset( found, text.size() ), found} );
	} else {
		found = doc.find( text, {0, 0} );
		if ( found.isValid() ) {
			doc.setSelection( {doc.positionOffset( found, text.size() ), found} );
		}
	}
}

void App::replaceSelection( const String& replacement ) {
	if ( !mCurEditor || !mCurEditor->getDocument().hasSelection() )
		return;
	mCurEditor->getDocument().replaceSelection( replacement );
}

void App::replaceAll( String find, const String& replace, const bool& caseSensitive ) {
	if ( !mCurEditor )
		return;
	if ( find.empty() )
		find = mLastSearch;
	if ( !mCurEditor || find.empty() )
		return;
	mLastSearch = find;
	TextDocument& doc = mCurEditor->getDocument();
	TextPosition found;
	TextPosition startedPosition = doc.getSelection().start();
	doc.setSelection( doc.startOfDoc() );
	do {
		found = doc.find( find, doc.getSelection( true ).end(), caseSensitive );
		if ( found.isValid() ) {
			doc.setSelection( {doc.positionOffset( found, find.size() ), found} );
			doc.replaceSelection( replace );
		}
	} while ( found.isValid() );
	doc.setSelection( startedPosition );
}

void App::findAndReplace( String find, String replace, const bool& caseSensitive ) {
	if ( find.empty() )
		find = mLastSearch;
	if ( !mCurEditor || find.empty() )
		return;
	mLastSearch = find;
	TextDocument& doc = mCurEditor->getDocument();
	if ( doc.hasSelection() && doc.getSelectedText() == find ) {
		replaceSelection( replace );
	} else {
		findNextText( find, caseSensitive );
	}
}

void App::runCommand( const std::string& command ) {
	if ( mCurEditor )
		mCurEditor->getDocument().execute( command );
}

void App::loadConfig() {
	std::string path( Sys::getConfigPath( "ecode" ) );
	if ( !FileSystem::fileExists( path ) )
		FileSystem::makeDir( path );
	FileSystem::dirPathAddSlashAtEnd( path );
	mIni.loadFromFile( path + "config.cfg" );
	mIniState.loadFromFile( path + "prev_state.cfg" );
	std::string recent = mIniState.getValue( "files", "recentfiles", "" );
	mRecentFiles = String::split( recent, ';' );
	mCurrentColorScheme = mConfig.editor.colorScheme =
		mIni.getValue( "editor", "colorscheme", "lite" );
	mConfig.editor.fontSize = mIni.getValueF( "editor", "font_size", 11 );
	mConfig.window.size.setWidth(
		mIniState.getValueI( "window", "width", PixelDensity::dpToPxI( 1280 ) ) );
	mConfig.window.size.setHeight(
		mIniState.getValueI( "window", "height", PixelDensity::dpToPxI( 720 ) ) );
	mConfig.window.maximized = mIniState.getValueB( "window", "maximized", false );
	mConfig.window.pixelDensity = mIniState.getValueF( "window", "pixeldensity" );
	mConfig.editor.showLineNumbers = mIni.getValueB( "editor", "show_line_numbers", true );
	mConfig.editor.showWhiteSpaces = mIni.getValueB( "editor", "show_white_spaces", true );
	mConfig.editor.highlightMatchingBracket =
		mIni.getValueB( "editor", "highlight_matching_brackets", true );
	mConfig.editor.highlightCurrentLine =
		mIni.getValueB( "editor", "highlight_current_line", true );
	mConfig.editor.horizontalScrollbar = mIni.getValueB( "editor", "horizontal_scrollbar", false );
	mConfig.ui.fontSize = mIni.getValueF( "ui", "font_size", 11 );
	mConfig.editor.trimTrailingWhitespaces =
		mIni.getValueB( "editor", "trim_trailing_whitespaces", false );
	mConfig.editor.forceNewLineAtEndOfFile =
		mIni.getValueB( "editor", "force_new_line_at_end_of_file", false );
	mConfig.editor.autoDetectIndentType =
		mIni.getValueB( "editor", "auto_detect_indent_type", true );
	mConfig.editor.writeUnicodeBOM = mIni.getValueB( "editor", "write_bom", false );
	mConfig.editor.indentWidth = mIni.getValueI( "editor", "indent_width", 4 );
	mConfig.editor.indentSpaces = mIni.getValueB( "editor", "indent_spaces", false );
	mConfig.editor.windowsLineEndings = mIni.getValueB( "editor", "windows_line_endings", false );
	mConfig.editor.tabWidth = eemax( 2, mIni.getValueI( "editor", "tab_width", 4 ) );
	mConfig.editor.lineBreakingColumn =
		eemax( 0, mIni.getValueI( "editor", "line_breaking_column", 100 ) );
}

void App::saveConfig() {
	mConfig.editor.colorScheme = mCurrentColorScheme;
	mConfig.window.size = mWindow->getLastWindowedSize();
	mConfig.window.maximized = mWindow->isMaximized();
	mIni.setValue( "editor", "colorscheme", mConfig.editor.colorScheme );
	mIniState.setValueI( "window", "width", mConfig.window.size.getWidth() );
	mIniState.setValueI( "window", "height", mConfig.window.size.getHeight() );
	mIniState.setValueB( "window", "maximized", mConfig.window.maximized );
	mIniState.setValueF( "window", "pixeldensity", mConfig.window.pixelDensity );
	mIniState.setValue( "files", "recentfiles", String::join( mRecentFiles, ';' ) );
	mIni.setValueB( "editor", "show_line_numbers", mConfig.editor.showLineNumbers );
	mIni.setValueB( "editor", "show_white_spaces", mConfig.editor.showWhiteSpaces );
	mIni.setValueB( "editor", "highlight_matching_brackets",
					mConfig.editor.highlightMatchingBracket );
	mIni.setValueB( "editor", "highlight_current_line", mConfig.editor.highlightCurrentLine );
	mIni.setValueB( "editor", "horizontal_scrollbar", mConfig.editor.horizontalScrollbar );
	mIni.setValueF( "editor", "font_size", mConfig.editor.fontSize );
	mIni.setValueF( "ui", "font_size", mConfig.ui.fontSize );
	mIni.setValueB( "editor", "trim_trailing_whitespaces", mConfig.editor.trimTrailingWhitespaces );
	mIni.setValueB( "editor", "force_new_line_at_end_of_file",
					mConfig.editor.forceNewLineAtEndOfFile );
	mIni.setValueB( "editor", "auto_detect_indent_type", mConfig.editor.autoDetectIndentType );
	mIni.setValueB( "editor", "write_bom", mConfig.editor.writeUnicodeBOM );
	mIni.setValueI( "editor", "indent_width", mConfig.editor.indentWidth );
	mIni.setValueB( "editor", "indent_spaces", mConfig.editor.indentSpaces );
	mIni.setValueB( "editor", "windows_line_endings", mConfig.editor.windowsLineEndings );
	mIni.setValueI( "editor", "tab_width", mConfig.editor.tabWidth );
	mIni.setValueI( "editor", "line_breaking_column", mConfig.editor.lineBreakingColumn );
	mIni.writeFile();
	mIniState.writeFile();
}

void App::initSearchBar() {
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mSearchBarLayout->execute( cmd );
		} );
	};
	auto addReturnListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::OnPressEnter, [this, cmd]( const Event* ) {
			mSearchBarLayout->execute( cmd );
		} );
	};
	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	UITextInput* replaceInput = mSearchBarLayout->find<UITextInput>( "search_replace" );
	UICheckBox* caseSensitiveChk = mSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	findInput->addEventListener( Event::OnTextChanged, [&, findInput,
														caseSensitiveChk]( const Event* ) {
		if ( mCurEditor ) {
			if ( !findInput->getText().empty() ) {
				mCurEditor->getDocument().setSelection( mCurEditor->getDocument().startOfDoc() );
				findNextText( findInput->getText(), caseSensitiveChk->isChecked() );
			} else {
				mCurEditor->getDocument().setSelection(
					mCurEditor->getDocument().getSelection().start() );
			}
		}
	} );
	mSearchBarLayout->addCommand( "close-searchbar", [&] {
		mSearchBarLayout->setEnabled( false )->setVisible( false );
		mCurEditor->setFocus();
	} );
	mSearchBarLayout->addCommand( "repeat-find", [this, findInput, caseSensitiveChk] {
		findNextText( findInput->getText(), caseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "replace-all", [this, findInput, replaceInput, caseSensitiveChk] {
		replaceAll( findInput->getText(), replaceInput->getText(), caseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "find-and-replace",
								  [this, findInput, replaceInput, caseSensitiveChk] {
									  findAndReplace( findInput->getText(), replaceInput->getText(),
													  caseSensitiveChk->isChecked() );
								  } );
	mSearchBarLayout->addCommand( "find-prev", [this, findInput, caseSensitiveChk] {
		findPrevText( findInput->getText(), caseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "replace-selection", [this, replaceInput] {
		replaceSelection( replaceInput->getText() );
	} );
	mSearchBarLayout->addCommand( "change-case", [caseSensitiveChk] {
		caseSensitiveChk->setChecked( !caseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->getKeyBindings().addKeybindsString( {
		{"f3", "repeat-find"},
		{"ctrl+g", "repeat-find"},
		{"escape", "close-searchbar"},
		{"ctrl+r", "replace-all"},
		{"ctrl+s", "change-case"},
	} );
	addReturnListener( findInput, "repeat-find" );
	addReturnListener( replaceInput, "find-and-replace" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "find_prev" ), "find-prev" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "find_next" ), "repeat-find" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace" ), "replace-selection" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace_find" ), "find-and-replace" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace_all" ), "replace-all" );
	addClickListener( mSearchBarLayout->find<UIWidget>( "searchbar_close" ), "close-searchbar" );
	replaceInput->addEventListener( Event::OnTabNavigate,
									[findInput]( const Event* ) { findInput->setFocus(); } );
}

void App::showFindView() {
	if ( !mCurEditor )
		return;
	mSearchBarLayout->setEnabled( true )->setVisible( true );
	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	findInput->setFocus();
	String text = mCurEditor->getDocument().getSelectedText();
	if ( !text.empty() ) {
		findInput->setText( text );
		findInput->getDocument().selectAll();
	} else if ( !findInput->getText().empty() ) {
		findInput->getDocument().selectAll();
	}
	mCurEditor->getDocument().setActiveClient( mCurEditor );
}

void App::closeApp() {
	if ( onCloseRequestCallback( mWindow ) )
		mWindow->close();
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

	Time elapsed = SceneManager::instance()->getElapsed();
	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() || mConsole->isActive() ||
		 mConsole->isFading() ) {
		mWindow->clear();
		SceneManager::instance()->draw();
		mConsole->draw( elapsed );
		mWindow->display();
	} else {
		Sys::sleep( Milliseconds( mWindow->hasFocus() ? 1 : 16 ) );
	}
}

void App::onFileDropped( String file ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor = mCurEditor;
	if ( !node )
		node = codeEditor;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
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
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) )
		codeEditor = node->asType<UICodeEditor>();
	if ( codeEditor && !text.empty() ) {
		if ( text[text.size() - 1] != '\n' )
			text += '\n';
		codeEditor->getDocument().textInput( text );
	}
}

App::~App() {
	saveConfig();
	eeSAFE_DELETE( mConsole );
}

void App::updateRecentFiles() {
	UINode* node = nullptr;
	if ( mSettingsMenu && ( node = mSettingsMenu->getItem( "Recent Files" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();
		uiMenuSubMenu->setEnabled( !mRecentFiles.empty() );
		menu->removeAll();
		menu->removeEventsOfType( Event::OnItemClicked );
		if ( mRecentFiles.empty() )
			return;
		for ( auto file : mRecentFiles )
			menu->add( file );
		menu->addSeparator();
		menu->add( "Clear Menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
			if ( txt != "Clear Menu" ) {
				std::string path( txt.toUtf8() );
				if ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) {
					loadFileFromPathInNewTab( path );
				}
			} else {
				mRecentFiles.clear();
				updateRecentFiles();
			}
		} );
	}
}

UIMenu* App::createViewMenu() {
	mViewMenu = UIPopUpMenu::New();
	mViewMenu->addCheckBox( "Show Line Numbers" )->setActive( mConfig.editor.showLineNumbers );
	mViewMenu->addCheckBox( "Show White Space" )->setActive( mConfig.editor.showWhiteSpaces );
	mViewMenu->addCheckBox( "Highlight Matching Bracket" )
		->setActive( mConfig.editor.highlightMatchingBracket );
	mViewMenu->addCheckBox( "Highlight Current Line" )
		->setActive( mConfig.editor.highlightCurrentLine );
	mViewMenu->addCheckBox( "Enable Horizontal ScrollBar" )
		->setActive( mConfig.editor.horizontalScrollbar );
	mViewMenu->addSeparator();
	mViewMenu->add( "Editor Font Size", findIcon( "font-size" ) );
	mViewMenu->add( "UI Font Size", findIcon( "font-size" ) );
	mViewMenu->add( "Line Breaking Column" );
	mViewMenu->addSeparator();
	mViewMenu->addCheckBox( "Full Screen Mode" )
		->setShortcutText( "Alt+Return" )
		->setId( "fullscreen-mode" );
	mViewMenu->addSeparator();
	mViewMenu->add( "Split Left", findIcon( "split-horizontal" ), "Ctrl+Shift+J" );
	mViewMenu->add( "Split Right", findIcon( "split-horizontal" ), "Ctrl+Shift+L" );
	mViewMenu->add( "Split Top", findIcon( "split-vertical" ), "Ctrl+Shift+I" );
	mViewMenu->add( "Split Bottom", findIcon( "split-vertical" ), "Ctrl+Shift+K" );
	mViewMenu->addSeparator();
	mViewMenu->add( "Zoom In", findIcon( "zoom-in" ), "Ctrl++" );
	mViewMenu->add( "Zoom Out", findIcon( "zoom-out" ), "Ctrl+-" );
	mViewMenu->add( "Zoom Reset", findIcon( "zoom-reset" ), "Ctrl+0" );
	mViewMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getText() == "Show Line Numbers" ) {
			mConfig.editor.showLineNumbers = item->asType<UIMenuCheckBox>()->isActive();
			forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowLineNumber( mConfig.editor.showLineNumbers );
			} );
		} else if ( item->getText() == "Show White Space" ) {
			mConfig.editor.showWhiteSpaces = item->asType<UIMenuCheckBox>()->isActive();
			forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowWhitespaces( mConfig.editor.showWhiteSpaces );
			} );
		} else if ( item->getText() == "Highlight Matching Bracket" ) {
			mConfig.editor.highlightMatchingBracket = item->asType<UIMenuCheckBox>()->isActive();
			forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightMatchingBracket( mConfig.editor.highlightMatchingBracket );
			} );
		} else if ( item->getText() == "Highlight Current Line" ) {
			mConfig.editor.highlightCurrentLine = item->asType<UIMenuCheckBox>()->isActive();
			forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightCurrentLine( mConfig.editor.highlightCurrentLine );
			} );
		} else if ( item->getText() == "Enable Horizontal ScrollBar" ) {
			mConfig.editor.horizontalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHorizontalScrollBarEnabled( mConfig.editor.horizontalScrollbar );
			} );
		} else if ( item->getText() == "Editor Font Size" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set the editor font size:" );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, true );
			msgBox->getTextInput()->setText( String::format(
				mConfig.editor.fontSize == (int)mConfig.editor.fontSize ? "%2.f" : "%2.1f",
				mConfig.editor.fontSize ) );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				Float val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) ) {
					mConfig.editor.fontSize = val;
					forEachEditor( [val]( UICodeEditor* editor ) { editor->setFontSize( val ); } );
				}
			} );
			msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
				if ( mCurEditor )
					mCurEditor->setFocus();
			} );
		} else if ( item->getText() == "UI Font Size" ) {
			UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::INPUT,
													  "Set the UI font size (requires restart):" );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, true );
			msgBox->getTextInput()->setText(
				String::format( mConfig.ui.fontSize == (int)mConfig.ui.fontSize ? "%2.f" : "%2.1f",
								mConfig.ui.fontSize ) );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				Float val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) ) {
					mConfig.ui.fontSize = val;
					msgBox->closeWindow();
				}
			} );
			msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
				if ( mCurEditor )
					mCurEditor->setFocus();
			} );
		} else if ( item->getText() == "Line Breaking Column" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set Line Breaking Column:\n"
														"Set 0 to disable it.\n" );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText(
				String::toString( mConfig.editor.lineBreakingColumn ) );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mConfig.editor.lineBreakingColumn = val;
					forEachEditor(
						[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
					msgBox->closeWindow();
				}
			} );
			msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
				if ( mCurEditor )
					mCurEditor->setFocus();
			} );
		} else if ( "Zoom In" == item->getText() ) {
			zoomIn();
		} else if ( "Zoom Out" == item->getText() ) {
			zoomOut();
		} else if ( "Zoom Reset" == item->getText() ) {
			zoomReset();
		} else if ( "Full Screen Mode" == item->getText() ) {
			runCommand( "fullscreen-toggle" );
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mViewMenu;
}

Drawable* App::findIcon( const std::string& name ) {
	return mUISceneNode->findIcon( name );
}

UIMenu* App::createEditMenu() {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( "Undo", findIcon( "undo" ), "Ctrl+Z" );
	menu->add( "Redo", findIcon( "redo" ), "Ctrl+Shift+Z" );
	menu->addSeparator();
	menu->add( "Cut", findIcon( "cut" ), "Ctrl+X" );
	menu->add( "Copy", findIcon( "copy" ), "Ctrl+C" );
	menu->add( "Paste", findIcon( "paste" ), "Ctrl+V" );
	menu->addSeparator();
	menu->add( "Select All", findIcon( "select-all" ), "Ctrl+A" );
	menu->addSeparator();
	menu->add( "Find/Replace", findIcon( "find-replace" ), "Ctrl+F" );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
		String::replaceAll( text, " ", "-" );
		String::replaceAll( text, "/", "-" );
		runCommand( text );
	} );
	return menu;
}

UIMenu* App::createDocumentMenu() {
	mDocMenu = UIPopUpMenu::New();

	mDocMenu->addCheckBox( "Auto Detect Indent Type", mConfig.editor.autoDetectIndentType )
		->setId( "auto_indent" );

	UIPopUpMenu* tabTypeMenu = UIPopUpMenu::New();
	tabTypeMenu->addRadioButton( "Tabs" )->setId( "tabs" );
	tabTypeMenu->addRadioButton( "Spaces" )->setId( "spaces" );
	mDocMenu->addSubMenu( "Indentation Type", nullptr, tabTypeMenu )->setId( "indent_type" );
	tabTypeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		if ( mCurEditor ) {
			TextDocument::IndentType indentType = text == "tabs"
													  ? TextDocument::IndentType::IndentTabs
													  : TextDocument::IndentType::IndentSpaces;
			mCurEditor->getDocument().setIndentType( indentType );
			mConfig.editor.indentSpaces = indentType == TextDocument::IndentType::IndentSpaces;
		}
	} );

	UIPopUpMenu* indentWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		indentWidthMenu
			->addRadioButton( String::toString( w ),
							  mCurEditor && mCurEditor->getDocument().getIndentWidth() == w )
			->setId( String::format( "indent_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( "Indent Width", nullptr, indentWidthMenu )->setId( "indent_width" );
	indentWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mCurEditor ) {
			int width = event->getNode()->getData();
			mCurEditor->getDocument().setIndentWidth( width );
			mConfig.editor.indentWidth = width;
		}
	} );

	UIPopUpMenu* tabWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		tabWidthMenu
			->addRadioButton( String::toString( w ), mCurEditor && mCurEditor->getTabWidth() == w )
			->setId( String::format( "tab_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( "Tab Width", nullptr, tabWidthMenu )->setId( "tab_width" );
	tabWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mCurEditor ) {
			int width = event->getNode()->getData();
			mCurEditor->setTabWidth( width );
			mConfig.editor.tabWidth = width;
		}
	} );

	UIPopUpMenu* lineEndingsMenu = UIPopUpMenu::New();
	lineEndingsMenu->addRadioButton( "Windows (CR/LF)", mConfig.editor.windowsLineEndings )
		->setId( "windows" );
	lineEndingsMenu->addRadioButton( "Unix (LF)", !mConfig.editor.windowsLineEndings )
		->setId( "unix" );
	mDocMenu->addSubMenu( "Line Endings", nullptr, lineEndingsMenu )->setId( "line_endings" );
	lineEndingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		bool winLe = event->getNode()->asType<UIRadioButton>()->getId() == "windows";
		if ( mCurEditor ) {
			mConfig.editor.windowsLineEndings = winLe;
			mCurEditor->getDocument().setLineEnding( winLe ? TextDocument::LineEnding::CRLF
														   : TextDocument::LineEnding::LF );
		}
	} );

	mDocMenu->addSeparator();

	mDocMenu->addCheckBox( "Read Only" )->setId( "read_only" );

	mDocMenu->addCheckBox( "Trim Trailing Whitespaces", mConfig.editor.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" );

	mDocMenu->addCheckBox( "Force New Line at End of File", mConfig.editor.forceNewLineAtEndOfFile )
		->setId( "force_nl" );

	mDocMenu->addCheckBox( "Write Unicode BOM", mConfig.editor.writeUnicodeBOM )
		->setId( "write_bom" );

	mDocMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !mCurEditor || event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();
		TextDocument& doc = mCurEditor->getDocument();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "auto_indent" == id ) {
				doc.setAutoDetectIndentType( item->isActive() );
				mConfig.editor.autoDetectIndentType = item->isActive();
			} else if ( "trim_whitespaces" == id ) {
				doc.setTrimTrailingWhitespaces( item->isActive() );
				mConfig.editor.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				doc.setForceNewLineAtEndOfFile( item->isActive() );
				mConfig.editor.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				doc.setBOM( item->isActive() );
				mConfig.editor.writeUnicodeBOM = item->isActive();
			} else if ( "read_only" == id ) {
				mCurEditor->setLocked( item->isActive() );
			}
		}
	} );
	return mDocMenu;
}

void App::updateDocumentMenu() {
	if ( !mCurEditor )
		return;

	const TextDocument& doc = mCurEditor->getDocument();

	mDocMenu->find( "auto_indent" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getAutoDetectIndentType() );

	mDocMenu->find( "indent_type" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( doc.getIndentType() == TextDocument::IndentType::IndentTabs ? "tabs" : "spaces" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "indent_width" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "indent_width_%d", doc.getIndentWidth() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "tab_width" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "tab_width_%d", mCurEditor->getTabWidth() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "trim_whitespaces" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getTrimTrailingWhitespaces() );

	mDocMenu->find( "force_nl" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getForceNewLineAtEndOfFile() );

	mDocMenu->find( "write_bom" )->asType<UIMenuCheckBox>()->setActive( doc.getBOM() );

	mDocMenu->find( "line_endings" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( mConfig.editor.windowsLineEndings ? "windows" : "unix" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "read_only" )->asType<UIMenuCheckBox>()->setActive( mCurEditor->isLocked() );
}

void App::createSettingsMenu() {
	mSettingsMenu = UIPopUpMenu::New();
	mSettingsMenu->add( "New", findIcon( "document-new" ), "Ctrl+T" );
	mSettingsMenu->add( "Open...", findIcon( "document-open" ), "Ctrl+O" );
	mSettingsMenu->addSubMenu( "Recent Files", findIcon( "document-recent" ), UIPopUpMenu::New() );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Save", findIcon( "document-save" ), "Ctrl+S" );
	mSettingsMenu->add( "Save as...", findIcon( "document-save-as" ) );
	mSettingsMenu->addSeparator();
	mSettingsMenu->addSubMenu( "Filetype", nullptr, createFiletypeMenu() );
	mSettingsMenu->addSubMenu( "Color Scheme", nullptr, createColorSchemeMenu() );
	mSettingsMenu->addSubMenu( "Document", nullptr, createDocumentMenu() );
	mSettingsMenu->addSubMenu( "Edit", nullptr, createEditMenu() );
	mSettingsMenu->addSubMenu( "View", nullptr, createViewMenu() );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Close", findIcon( "document-close" ), "Ctrl+W" );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Quit", findIcon( "quit" ), "Ctrl+Q" );
	mSettingsButton = mUISceneNode->find<UITextView>( "settings" );
	mSettingsButton->addEventListener( Event::MouseClick, [&]( const Event* ) {
		Vector2f pos( mSettingsButton->getPixelsPosition() );
		mSettingsButton->nodeToWorldTranslation( pos );
		UIMenu::findBestMenuPos( pos, mSettingsMenu );
		mSettingsMenu->setPixelsPosition( pos );
		mSettingsMenu->show();
	} );
	mSettingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& name = event->getNode()->asType<UIMenuItem>()->getText();
		if ( name == "New" ) {
			runCommand( "create-new" );
		} else if ( name == "Open..." ) {
			runCommand( "open-file" );
		} else if ( name == "Save" ) {
			runCommand( "save-doc" );
		} else if ( name == "Save as..." ) {
			runCommand( "save-as-doc" );
		} else if ( name == "Close" ) {
			runCommand( "close-doc" );
		} else if ( name == "Quit" ) {
			runCommand( "close-app" );
		}
	} );
	updateRecentFiles();
}

void App::setColorScheme( const std::string& name ) {
	if ( name != mCurrentColorScheme ) {
		mCurrentColorScheme = name;
		applyColorScheme( mColorSchemes[mCurrentColorScheme] );
	}
}

void App::updateColorSchemeMenu() {
	for ( size_t i = 0; i < mColorSchemeMenu->getCount(); i++ ) {
		UIMenuRadioButton* menuItem = mColorSchemeMenu->getItem( i )->asType<UIMenuRadioButton>();
		menuItem->setActive( mCurrentColorScheme == menuItem->getText() );
	}
}

UIMenu* App::createColorSchemeMenu() {
	mColorSchemeMenu = UIPopUpMenu::New();
	for ( auto& colorScheme : mColorSchemes ) {
		mColorSchemeMenu->addRadioButton( colorScheme.first,
										  mCurrentColorScheme == colorScheme.first );
	}
	mColorSchemeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		setColorScheme( name );
	} );
	return mColorSchemeMenu;
}

UIMenu* App::createFiletypeMenu() {
	auto* dM = SyntaxDefinitionManager::instance();
	mFiletypeMenu = UIPopUpMenu::New();
	auto names = dM->getLanguageNames();
	for ( auto& name : names ) {
		mFiletypeMenu->addRadioButton(
			name, mCurEditor && mCurEditor->getSyntaxDefinition().getLanguageName() == name );
	}
	mFiletypeMenu->addEventListener( Event::OnItemClicked, [&, dM]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		if ( mCurEditor ) {
			mCurEditor->setSyntaxDefinition( dM->getStyleByLanguageName( name ) );
			updateCurrentFiletype();
		}
	} );
	return mFiletypeMenu;
}

void App::updateCurrentFiletype() {
	if ( !mCurEditor )
		return;
	std::string curLang( mCurEditor->getSyntaxDefinition().getLanguageName() );
	for ( size_t i = 0; i < mFiletypeMenu->getCount(); i++ ) {
		UIMenuRadioButton* menuItem = mFiletypeMenu->getItem( i )->asType<UIMenuRadioButton>();
		std::string itemLang( menuItem->getText() );
		menuItem->setActive( curLang == itemLang );
	}
}

void App::updateEditorState() {
	if ( mCurEditor ) {
		updateEditorTitle( mCurEditor );
		updateCurrentFiletype();
		updateDocumentMenu();
	}
}

void App::init( const std::string& file, const Float& pidelDensity ) {
	loadConfig();

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	mConfig.window.pixelDensity =
		pidelDensity > 0 ? pidelDensity
						 : ( mConfig.window.pixelDensity > 0 ? mConfig.window.pixelDensity
															 : currentDisplay->getPixelDensity() );

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	std::string resPath( Sys::getProcessPath() );

	mWindow = Engine::instance()->createWindow(
		WindowSettings( mConfig.window.size.getWidth(), mConfig.window.size.getHeight(),
						mWindowTitle, WindowStyle::Default, WindowBackend::Default, 32,
						resPath + "assets/icon/ee.png", 1 ),
		ContextSettings( true ) );

	if ( mWindow->isOpen() ) {
		PixelDensity::setPixelDensity( mConfig.window.pixelDensity );

		if ( mConfig.window.maximized )
			mWindow->maximize();

		mWindow->setCloseRequestCallback(
			[&]( EE::Window::Window* win ) -> bool { return onCloseRequestCallback( win ); } );

		mWindow->getInput()->pushCallback( [&]( InputEvent* event ) {
			if ( event->Type == InputEvent::FileDropped ) {
				onFileDropped( event->file.file );
			} else if ( event->Type == InputEvent::TextDropped ) {
				onTextDropped( event->textdrop.text );
			}
		} );

		PixelDensity::setPixelDensity( eemax( mWindow->getScale(), mConfig.window.pixelDensity ) );

		mUISceneNode = UISceneNode::New();

		FontTrueType* font =
			FontTrueType::New( "NotoSans-Regular", resPath + "assets/fonts/NotoSans-Regular.ttf" );

		FontTrueType* fontMono =
			FontTrueType::New( "monospace", resPath + "assets/fonts/DejaVuSansMono.ttf" );
		fontMono->setBoldAdvanceSameAsRegular( true );

		FontTrueType* iconFont =
			FontTrueType::New( "icon", resPath + "assets/fonts/remixicon.ttf" );

		SceneManager::instance()->add( mUISceneNode );

		UITheme* theme =
			UITheme::load( "uitheme", "uitheme", "", font, resPath + "assets/ui/breeze.css" );
		theme->setDefaultFontSize( mConfig.ui.fontSize );
		mUISceneNode->setStyleSheet( theme->getStyleSheet() );
		mUISceneNode
			->getUIThemeManager()
			//->setDefaultEffectsEnabled( true )
			->setDefaultTheme( theme )
			->setDefaultFont( font )
			->setDefaultFontSize( mConfig.ui.fontSize )
			->add( theme );

		auto colorSchemes =
			SyntaxColorScheme::loadFromFile( resPath + "assets/colorschemes/colorschemes.conf" );
		if ( !colorSchemes.empty() ) {
			for ( auto& colorScheme : colorSchemes )
				mColorSchemes[colorScheme.getName()] = colorScheme;
			mCurrentColorScheme =
				mColorSchemes.find( mConfig.editor.colorScheme ) != mColorSchemes.end()
					? mConfig.editor.colorScheme
					: colorSchemes[0].getName();
		} else {
			mColorSchemes["default"] = SyntaxColorScheme::getDefault();
			mCurrentColorScheme = "default";
		}

		mUISceneNode->getRoot()->addClass( "appbackground" );

		const std::string baseUI = R"xml(
		<style>
		TextInput#search_find,
		TextInput#search_replace {
			padding-top: 0;
			padding-bottom: 0;
		}
		#search_bar {
			padding-left: 4dp;
			padding-right: 4dp;
			padding-bottom: 3dp;
		}
		.close_button {
			width: 12dp;
			height: 12dp;
			border-radius: 6dp;
			background-color: var(--icon-back-hover);
			foreground-image: poly(line, var(--icon-line-hover), "0dp 0dp, 6dp 6dp"), poly(line, var(--icon-line-hover), "6dp 0dp, 0dp 6dp");
			foreground-position: 3dp 3dp, 3dp 3dp;
			transition: all 0.15s;
		}
		.close_button:hover {
			background-color: var(--icon-back-alert);
		}
		#settings {
			color: #eff0f188;
			font-family: icon;
			font-size: 16dp;
			margin-top: 6dp;
			margin-right: 22dp;
			transition: all 0.15s;
		}
		#settings:hover {
			color: var(--primary);
		}
		</style>
		<RelativeLayout layout_width="match_parent" layout_height="match_parent">
		<vbox layout_width="match_parent" layout_height="match_parent">
			<vbox id="code_container" layout_width="match_parent" layout_height="0" layout_weight="1">
			</vbox>
			<searchbar id="search_bar" layout_width="match_parent" layout_height="wrap_content" margin-bottom="2dp">
				<vbox layout_width="wrap_content" layout_height="wrap_content" margin-right="4dp">
					<TextView layout_width="wrap_content" layout_height="18dp" text="Find:"  margin-bottom="2dp" />
					<TextView layout_width="wrap_content" layout_height="18dp" text="Replace with:" />
				</vbox>
				<vbox layout_width="0" layout_weight="1" layout_height="wrap_content" margin-right="4dp">
					<TextInput id="search_find" layout_width="match_parent" layout_height="18dp" padding="0" margin-bottom="2dp" />
					<TextInput id="search_replace" layout_width="match_parent" layout_height="18dp" padding="0" />
				</vbox>
				<vbox layout_width="wrap_content" layout_height="wrap_content">
					<hbox layout_width="wrap_content" layout_height="wrap_content" margin-bottom="2dp">
						<PushButton id="find_prev" layout_width="wrap_content" layout_height="18dp" text="Previous" margin-right="4dp" />
						<PushButton id="find_next" layout_width="wrap_content" layout_height="18dp" text="Next" margin-right="4dp" />
						<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="true" />
						<RelativeLayout layout_width="0" layout_weight="1" layout_height="18dp">
							<Widget id="searchbar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center_vertical|right" />
						</RelativeLayout>
					</hbox>
					<hbox layout_width="wrap_content" layout_height="wrap_content">
						<PushButton id="replace" layout_width="wrap_content" layout_height="18dp" text="Replace" margin-right="4dp" />
						<PushButton id="replace_find" layout_width="wrap_content" layout_height="18dp" text="Replace & Find" margin-right="4dp" />
						<PushButton id="replace_all" layout_width="wrap_content" layout_height="18dp" text="Replace All" />
					</hbox>
				</vbox>
			</searchbar>
		</vbox>
		<TextView id="settings" layout_width="wrap_content" layout_height="wrap_content" text="&#xf0e9;" layout_gravity="top|right" />
		</RelativeLayout>
		)xml";

		UIWidgetCreator::registerWidget( "searchbar", [] { return UISearchBar::New(); } );
		mUISceneNode->loadLayoutFromString( baseUI );
		mUISceneNode->bind( "code_container", mBaseLayout );
		mUISceneNode->bind( "search_bar", mSearchBarLayout );
		mSearchBarLayout->setVisible( false )->setEnabled( false );
		UIIconTheme* iconTheme = UIIconTheme::New( "remixicon" );
		auto addIcon = [iconTheme, iconFont]( const std::string& name, const Uint32& codePoint,
											  const Uint32& size ) {
			iconTheme->add( name,
							iconFont->getGlyphDrawable( codePoint, PixelDensity::dpToPx( size ) ) );
		};
		addIcon( "go-up", 0xea78, 16 );
		addIcon( "ok", 0xeb7a, 16 );
		addIcon( "cancel", 0xeb98, 16 );
		addIcon( "document-new", 0xecc3, 12 );
		addIcon( "document-open", 0xed70, 12 );
		addIcon( "document-save", 0xf0b3, 12 );
		addIcon( "document-save-as", 0xf0b3, 12 );
		addIcon( "document-close", 0xeb99, 12 );
		addIcon( "quit", 0xeb97, 12 );
		addIcon( "undo", 0xea58, 12 );
		addIcon( "redo", 0xea5a, 12 );
		addIcon( "redo", 0xea5a, 12 );
		addIcon( "cut", 0xf0c1, 12 );
		addIcon( "copy", 0xecd5, 12 );
		addIcon( "paste", 0xeb91, 12 );
		addIcon( "split-horizontal", 0xf17a, 12 );
		addIcon( "split-vertical", 0xf17b, 12 );
		addIcon( "find-replace", 0xed2b, 12 );
		addIcon( "folder", 0xed54, 12 );
		addIcon( "folder-add", 0xed5a, 12 );
		addIcon( "file", 0xecc3, 12 );
		addIcon( "file-code", 0xecd1, 12 );
		addIcon( "file-edit", 0xecdb, 12 );
		addIcon( "font-size", 0xed8d, 12 );
		addIcon( "color-picker", 0xf13d, 16 );
		addIcon( "zoom-in", 0xf2db, 12 );
		addIcon( "zoom-out", 0xf2dd, 12 );
		addIcon( "zoom-reset", 0xeb47, 12 );
		addIcon( "fullscreen", 0xed9c, 12 );

		mUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );
		initSearchBar();

		createSettingsMenu();
		createEditorWithTabWidget( mBaseLayout );

		if ( !file.empty() ) {
			loadFileFromPath( file );
		}

		mConsole = eeNew( Console, ( fontMono, true, true, 1024 * 1000, 0, mWindow ) );

		mWindow->runMainLoop( &appLoop );
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	args::ArgumentParser parser( "ecode" );
	args::HelpFlag help( parser, "help", "Display this help menu", {'h', "help"} );
	args::Positional<std::string> file( parser, "file", "The file path" );
	args::ValueFlag<Float> pixelDenstiyConf(
		parser, "pixel-density", "Set default application pixel density", {'d', "pixel-density"} );

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
	appInstance->init( file.Get(), pixelDenstiyConf ? pixelDenstiyConf.Get() : 0.f );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
