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
}

void App::switchToTab( Int32 index ) {
	UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
	if ( tabWidget ) {
		tabWidget->setTabSelected( eeclamp<Int32>( index, 0, tabWidget->getTabCount() - 1 ) );
	}
}

UITabWidget* App::findPreviousSplit( UICodeEditor* editor ) {
	if ( !editor )
		return NULL;
	UISplitter* splitter = splitterFromEditor( editor );
	if ( !splitter )
		return NULL;
	UITabWidget* tabWidget = tabWidgetFromEditor( editor );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.rbegin(), mTabWidgets.rend(), tabWidget );
		if ( it != mTabWidgets.rend() && ++it != mTabWidgets.rend() ) {
			return *it;
		}
	}
	return NULL;
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
		return NULL;
	UISplitter* splitter = splitterFromEditor( editor );
	if ( !splitter )
		return NULL;
	UITabWidget* tabWidget = tabWidgetFromEditor( editor );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
		if ( it != mTabWidgets.end() && ++it != mTabWidgets.end() ) {
			return *it;
		}
	}
	return NULL;
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
}

UICodeEditor* App::createCodeEditor() {
	UICodeEditor* codeEditor = UICodeEditor::New();
	codeEditor->setFontSize( 11 );
	codeEditor->setColorScheme( mColorSchemes[mCurrentColorScheme] );
	codeEditor->getDocument().setCommand( "switch-to-previous-colorscheme", [&] {
		auto it = mColorSchemes.find( mCurrentColorScheme );
		auto prev = std::prev( it, 1 );
		if ( prev != mColorSchemes.end() )
			mCurrentColorScheme = prev->first;
		else
			mCurrentColorScheme = mColorSchemes.rbegin()->first;
		applyColorScheme( mColorSchemes[mCurrentColorScheme] );
	} );
	codeEditor->getDocument().setCommand( "switch-to-next-colorscheme", [&] {
		auto it = mColorSchemes.find( mCurrentColorScheme );
		if ( ++it != mColorSchemes.end() )
			mCurrentColorScheme = it->first;
		else
			mCurrentColorScheme = mColorSchemes.begin()->first;
		applyColorScheme( mColorSchemes[mCurrentColorScheme] );
	} );
	codeEditor->getDocument().setCommand( "switch-to-previous-split",
										  [&] { switchPreviousSplit( mCurEditor ); } );
	codeEditor->getDocument().setCommand( "switch-to-next-split",
										  [&] { switchNextSplit( mCurEditor ); } );
	codeEditor->getDocument().setCommand( "save-doc", [&, codeEditor] {
		if ( codeEditor->save() )
			updateEditorTitle( codeEditor );
	} );
	codeEditor->getDocument().setCommand( "find", [&] { showFindView(); } );
	codeEditor->getDocument().setCommand( "repeat-find", [&] {
		findNextText( "", mSearchBarLayout->find<UICheckBox>( "case_sensitive" )->isChecked() );
	} );
	codeEditor->getDocument().setCommand( "close-app", [&] { closeApp(); } );
	codeEditor->getDocument().setCommand( "fullscreen-toggle",
										  [&]() { mWindow->toggleFullscreen(); } );
	codeEditor->getDocument().setCommand( "open-file", [&] { openFileDialog(); } );
	codeEditor->getDocument().setCommand( "console-toggle", [&] { mConsole->toggle(); } );
	codeEditor->getDocument().setCommand( "close-doc", [&] { tryTabClose( mCurEditor ); } );
	codeEditor->getDocument().setCommand( "create-new", [&] {
		auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
		d.first->getTabWidget()->setTabSelected( d.first );
	} );
	codeEditor->getDocument().setCommand( "next-doc", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mCurEditor->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			switchToTab( ( tabIndex + 1 ) % tabWidget->getTabCount() );
		}
	} );
	codeEditor->getDocument().setCommand( "previous-doc", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() > 1 ) {
			UITab* tab = (UITab*)mCurEditor->getData();
			Uint32 tabIndex = tabWidget->getTabIndex( tab );
			Int32 newTabIndex = (Int32)tabIndex - 1;
			switchToTab( newTabIndex < 0 ? tabWidget->getTabCount() - newTabIndex : newTabIndex );
		}
	} );
	for ( int i = 1; i <= 10; i++ )
		codeEditor->getDocument().setCommand( String::format( "switch-to-tab-%d", i ),
											  [&, i] { switchToTab( i - 1 ); } );
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
		mCurEditor = NULL;
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
					focusSomeEditor( NULL );
				}
				return;
			}
		}
		auto d = createCodeEditorInTabWidget( tabWidget );
		d.first->getTabWidget()->setTabSelected( d.first );
	} else {
		tabWidget->setTabSelected( tabWidget->getTabCount() - 1 );
	}
}

std::pair<UITab*, UICodeEditor*> App::createCodeEditorInTabWidget( UITabWidget* tabWidget ) {
	if ( NULL == tabWidget )
		return std::make_pair( (UITab*)NULL, (UICodeEditor*)NULL );
	UICodeEditor* editor = createCodeEditor();
	UITab* tab = tabWidget->add( editor->getDocument().getFilename(), editor );
	editor->setData( (UintPtr)tab );
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
		mCurEditor = tabWidget->getTabSelected()->getOwnedWidget()->asType<UICodeEditor>();
		updateEditorTitle( mCurEditor );
	} );
	tabWidget->setTabTryCloseCallback( [&]( UITab* tab ) -> bool {
		tryTabClose( tab->getOwnedWidget()->asType<UICodeEditor>() );
		return false;
	} );
	tabWidget->addEventListener( Event::OnTabClosed, [&]( const Event* event ) {
		onTabClosed( static_cast<const TabEvent*>( event ) );
	} );
	createCodeEditorInTabWidget( tabWidget );
	mTabWidgets.push_back( tabWidget );
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

void App::initSearchBar() {
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mSearchBarLayout->execute( cmd );
		} );
	};
	auto addReturnListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::KeyDown, [this, cmd]( const Event* event ) {
			const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
			if ( keyEvent->getKeyCode() == KEY_RETURN )
				mSearchBarLayout->execute( cmd );
		} );
	};
	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	UITextInput* replaceInput = mSearchBarLayout->find<UITextInput>( "search_replace" );
	UICheckBox* caseSensitiveChk = mSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	findInput->getInputTextBuffer()->pushIgnoredChar( '\t' );
	replaceInput->getInputTextBuffer()->pushIgnoredChar( '\t' );
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
		findInput->getInputTextBuffer()->selectAll();
	}
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
	displayManager->disableBypassCompositor();

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

		auto colorSchemes =
			SyntaxColorScheme::loadFromFile( resPath + "assets/colorschemes/colorschemes.conf" );
		if ( !colorSchemes.empty() ) {
			mCurrentColorScheme = colorSchemes[0].getName();
			for ( auto& colorScheme : colorSchemes )
				mColorSchemes[colorScheme.getName()] = colorScheme;
		} else {
			mColorSchemes["default"] = SyntaxColorScheme::getDefault();
			mCurrentColorScheme = "default";
		}

		mUISceneNode->getRoot()->addClass( "appbackground" );

		std::string baseUI = R"xml(
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
		</style>
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
		)xml";

		UIWidgetCreator::registerWidget( "searchbar", [] { return UISearchBar::New(); } );
		mUISceneNode->loadLayoutFromString( baseUI );
		mUISceneNode->bind( "code_container", mBaseLayout );
		mUISceneNode->bind( "search_bar", mSearchBarLayout );
		mSearchBarLayout->setVisible( false )->setEnabled( false );
		initSearchBar();

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
