#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/window/displaymanager.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Tools {

const std::map<KeyBindings::Shortcut, std::string> UICodeEditorSplitter::getDefaultKeybindings() {
	auto keybindings = UICodeEditor::getDefaultKeybindings();
	auto localKeybindings = getLocalDefaultKeybindings();
	localKeybindings.insert( keybindings.begin(), keybindings.end() );
	return localKeybindings;
}

const std::map<KeyBindings::Shortcut, std::string>
UICodeEditorSplitter::getLocalDefaultKeybindings() {
	return {
		{ { KEY_S, KeyMod::getDefaultModifier() }, "save-doc" },
		{ { KEY_L, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "lock-toggle" },
		{ { KEY_T, KeyMod::getDefaultModifier() }, "create-new" },
		{ { KEY_W, KeyMod::getDefaultModifier() }, "close-tab" },
		{ { KEY_TAB, KeyMod::getDefaultModifier() }, "next-tab" },
		{ { KEY_TAB, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "previous-tab" },
		{ { KEY_J, KEYMOD_LALT | KEYMOD_SHIFT }, "split-left" },
		{ { KEY_L, KEYMOD_LALT | KEYMOD_SHIFT }, "split-right" },
		{ { KEY_I, KEYMOD_LALT | KEYMOD_SHIFT }, "split-top" },
		{ { KEY_K, KEYMOD_LALT | KEYMOD_SHIFT }, "split-bottom" },
		{ { KEY_S, KEYMOD_LALT | KEYMOD_SHIFT }, "split-swap" },
		{ { KEY_J, KeyMod::getDefaultModifier() | KEYMOD_LALT }, "switch-to-previous-split" },
		{ { KEY_L, KeyMod::getDefaultModifier() | KEYMOD_LALT }, "switch-to-next-split" },
		{ { KEY_N, KeyMod::getDefaultModifier() | KEYMOD_LALT }, "switch-to-previous-colorscheme" },
		{ { KEY_M, KeyMod::getDefaultModifier() | KEYMOD_LALT }, "switch-to-next-colorscheme" },
		{ { KEY_1, KeyMod::getDefaultModifier() }, "switch-to-tab-1" },
		{ { KEY_2, KeyMod::getDefaultModifier() }, "switch-to-tab-2" },
		{ { KEY_3, KeyMod::getDefaultModifier() }, "switch-to-tab-3" },
		{ { KEY_4, KeyMod::getDefaultModifier() }, "switch-to-tab-4" },
		{ { KEY_5, KeyMod::getDefaultModifier() }, "switch-to-tab-5" },
		{ { KEY_6, KeyMod::getDefaultModifier() }, "switch-to-tab-6" },
		{ { KEY_7, KeyMod::getDefaultModifier() }, "switch-to-tab-7" },
		{ { KEY_8, KeyMod::getDefaultModifier() }, "switch-to-tab-8" },
		{ { KEY_9, KeyMod::getDefaultModifier() }, "switch-to-tab-9" },
		{ { KEY_0, KeyMod::getDefaultModifier() }, "switch-to-last-tab" },
		{ { KEY_1, KEYMOD_LALT }, "switch-to-tab-1" },
		{ { KEY_2, KEYMOD_LALT }, "switch-to-tab-2" },
		{ { KEY_3, KEYMOD_LALT }, "switch-to-tab-3" },
		{ { KEY_4, KEYMOD_LALT }, "switch-to-tab-4" },
		{ { KEY_5, KEYMOD_LALT }, "switch-to-tab-5" },
		{ { KEY_6, KEYMOD_LALT }, "switch-to-tab-6" },
		{ { KEY_7, KEYMOD_LALT }, "switch-to-tab-7" },
		{ { KEY_8, KEYMOD_LALT }, "switch-to-tab-8" },
		{ { KEY_9, KEYMOD_LALT }, "switch-to-tab-9" },
		{ { KEY_0, KEYMOD_LALT }, "switch-to-last-tab" },
	};
}

std::vector<std::string> UICodeEditorSplitter::getUnlockedCommands() {
	const std::vector<std::string> unlockedCmds{ "lock-toggle",
												 "create-new",
												 "close-tab",
												 "next-tab",
												 "previous-tab",
												 "split-left",
												 "split-right",
												 "split-top",
												 "split-bottom",
												 "split-swap",
												 "switch-to-previous-split",
												 "switch-to-next-split",
												 "switch-to-previous-colorscheme",
												 "switch-to-next-colorscheme" };
	return unlockedCmds;
}

UICodeEditorSplitter* UICodeEditorSplitter::New( UICodeEditorSplitter::Client* client,
												 UISceneNode* sceneNode,
												 const std::vector<SyntaxColorScheme>& colorSchemes,
												 const std::string& initColorScheme ) {
	return eeNew( UICodeEditorSplitter, ( client, sceneNode, colorSchemes, initColorScheme ) );
}

UICodeEditorSplitter::UICodeEditorSplitter( UICodeEditorSplitter::Client* client,
											UISceneNode* sceneNode,
											const std::vector<SyntaxColorScheme>& colorSchemes,
											const std::string& initColorScheme ) :
	mUISceneNode( sceneNode ), mClient( client ) {
	if ( !colorSchemes.empty() ) {
		for ( auto& colorScheme : colorSchemes )
			mColorSchemes.insert( std::make_pair( colorScheme.getName(), colorScheme ) );
		mCurrentColorScheme = mColorSchemes.find( initColorScheme ) != mColorSchemes.end()
								  ? initColorScheme
								  : colorSchemes[0].getName();
	} else {
		mColorSchemes["default"] = SyntaxColorScheme::getDefault();
		mCurrentColorScheme = "default";
	}
}

UICodeEditorSplitter::~UICodeEditorSplitter() {}

UITabWidget* UICodeEditorSplitter::tabWidgetFromEditor( UICodeEditor* editor ) const {
	if ( editor )
		return ( (UITab*)editor->getData() )->getTabWidget();
	return nullptr;
}

UITabWidget* UICodeEditorSplitter::tabWidgetFromWidget( UIWidget* widget ) const {
	if ( widget )
		return ( (UITab*)widget->getData() )->getTabWidget();
	return nullptr;
}

UISplitter* UICodeEditorSplitter::splitterFromEditor( UICodeEditor* editor ) const {
	if ( editor && editor->getParent()->getParent()->getParent()->isType( UI_TYPE_SPLITTER ) )
		return editor->getParent()->getParent()->getParent()->asType<UISplitter>();
	return nullptr;
}

UISplitter* UICodeEditorSplitter::splitterFromWidget( UIWidget* widget ) const {
	if ( widget && widget->getParent() && widget->getParent()->getParent() &&
		 widget->getParent()->getParent()->getParent() &&
		 widget->getParent()->getParent()->getParent()->isType( UI_TYPE_SPLITTER ) )
		return widget->getParent()->getParent()->getParent()->asType<UISplitter>();
	return nullptr;
}

UICodeEditor* UICodeEditorSplitter::createCodeEditor() {
	UICodeEditor* editor = UICodeEditor::NewOpt( true, true );
	TextDocument& doc = editor->getDocument();
	/* document commands */
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
			mClient->onDocumentStateChanged( mCurEditor, mCurEditor->getDocument() );
		}
	} );
	doc.setCommand( "unlock", [&] {
		if ( mCurEditor ) {
			mCurEditor->setLocked( false );
			mClient->onDocumentStateChanged( mCurEditor, mCurEditor->getDocument() );
		}
	} );
	doc.setCommand( "lock-toggle", [&] {
		if ( mCurEditor ) {
			mCurEditor->setLocked( !mCurEditor->isLocked() );
			mClient->onDocumentStateChanged( mCurEditor, mCurEditor->getDocument() );
		}
	} );
	editor->addUnlockedCommand( "copy" );
	editor->addUnlockedCommand( "select-all" );
	/* document commands */

	/* editor commands */
	doc.setCommand( "switch-to-previous-colorscheme", [&] {
		auto it = mColorSchemes.find( mCurrentColorScheme );
		setColorScheme( it == mColorSchemes.begin() ? mColorSchemes.rbegin()->first
													: ( --it )->first );
	} );

	doc.setCommand( "switch-to-next-colorscheme", [&] {
		auto it = mColorSchemes.find( mCurrentColorScheme );
		if ( ++it != mColorSchemes.end() )
			mCurrentColorScheme = it->first;
		else
			mCurrentColorScheme = mColorSchemes.begin()->first;
		applyColorScheme( mColorSchemes[mCurrentColorScheme] );
	} );
	doc.setCommand( "open-containing-folder", [&] {
		if ( mCurEditor )
			mCurEditor->openContainingFolder();
	} );
	doc.setCommand( "copy-file-path", [&] {
		if ( mCurEditor )
			mCurEditor->copyFilePath();
	} );
	/* editor commands */

	/* Splitter commands */
	registerSplitterCommands( doc );
	/* Splitter commands */

	editor->addEventListener( Event::OnFocus, [&]( const Event* event ) {
		setCurrentWidget( event->getNode()->asType<UICodeEditor>() );
	} );
	editor->addEventListener( Event::OnTextChanged, [&]( const Event* event ) {
		mClient->onDocumentModified( event->getNode()->asType<UICodeEditor>(),
									 event->getNode()->asType<UICodeEditor>()->getDocument() );
	} );
	editor->addEventListener( Event::OnSelectionChanged, [&]( const Event* event ) {
		mClient->onDocumentSelectionChange(
			event->getNode()->asType<UICodeEditor>(),
			event->getNode()->asType<UICodeEditor>()->getDocument() );
	} );
	editor->addEventListener( Event::OnCursorPosChange, [&]( const Event* event ) {
		mClient->onDocumentCursorPosChange(
			event->getNode()->asType<UICodeEditor>(),
			event->getNode()->asType<UICodeEditor>()->getDocument() );
	} );
	editor->addKeyBinds( getLocalDefaultKeybindings() );
	editor->addUnlockedCommands( getUnlockedCommands() );

	if ( nullptr == mCurEditor ) {
		mAboutToAddEditor = editor;
		setCurrentEditor( editor );
		mAboutToAddEditor = nullptr;
	}
	mClient->onCodeEditorCreated( editor, doc );

	return editor;
}

const std::string& UICodeEditorSplitter::getCurrentColorSchemeName() const {
	return mCurrentColorScheme;
}

const SyntaxColorScheme& UICodeEditorSplitter::getCurrentColorScheme() const {
	return mColorSchemes.at( mCurrentColorScheme );
}

void UICodeEditorSplitter::setColorScheme( const std::string& name ) {
	if ( name != mCurrentColorScheme ) {
		mCurrentColorScheme = name;
		applyColorScheme( mColorSchemes[mCurrentColorScheme] );
	}
}

const std::map<std::string, SyntaxColorScheme>& UICodeEditorSplitter::getColorSchemes() const {
	return mColorSchemes;
}

bool UICodeEditorSplitter::editorExists( UICodeEditor* editor ) {
	Lock l( mTabWidgetMutex );
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( editor == tabWidget->getTab( i )->getOwnedWidget() ) {
				return true;
			}
		}
	}
	return false;
}

bool UICodeEditorSplitter::loadDocument( std::shared_ptr<TextDocument> doc,
										 UICodeEditor* codeEditor ) {
	if ( nullptr == codeEditor )
		codeEditor = mCurEditor;

	if ( nullptr == codeEditor )
		return false;

	codeEditor->setColorScheme( mColorSchemes[mCurrentColorScheme] );
	codeEditor->setDocument( doc );

	return true;
}

std::pair<UITab*, UICodeEditor*>
UICodeEditorSplitter::loadDocumentInNewTab( std::shared_ptr<TextDocument> doc ) {
	auto d = createCodeEditorInTabWidget( tabWidgetFromWidget( mCurWidget ) );
	if ( d.first == nullptr || d.second == nullptr ) {
		if ( !mTabWidgets.empty() && mTabWidgets[0]->getTabCount() > 0 ) {
			d = createCodeEditorInTabWidget( mTabWidgets[0] );
		} else {
			Log::error( "Couldn't createCodeEditorInTabWidget in "
						"UICodeEditorSplitter::loadDocumentInNewTab" );
			return d;
		}
	}
	UITabWidget* tabWidget = d.first->getTabWidget();
	loadDocument( doc, d.second );
	tabWidget->setTabSelected( d.first );
	return d;
}

bool UICodeEditorSplitter::loadFileFromPath( const std::string& path, UICodeEditor* codeEditor ) {
	if ( FileSystem::isDirectory( path ) )
		return false;
	if ( nullptr == codeEditor )
		codeEditor = mCurEditor;
	codeEditor->setColorScheme( mColorSchemes[mCurrentColorScheme] );
	bool isUrl = String::startsWith( path, "https://" ) || String::startsWith( path, "http://" );
	bool ret = isUrl ? codeEditor->loadAsyncFromURL(
						   path, Http::Request::FieldTable(),
						   [&, codeEditor, path]( std::shared_ptr<TextDocument>, bool ) {
							   mClient->onDocumentLoaded( codeEditor, path );
						   } )
					 : codeEditor->loadFromFile( path ) == TextDocument::LoadStatus::Loaded;
	if ( ret && !isUrl )
		mClient->onDocumentLoaded( codeEditor, path );
	return ret;
}

void UICodeEditorSplitter::loadAsyncFileFromPath(
	const std::string& path, std::shared_ptr<ThreadPool> pool, UICodeEditor* codeEditor,
	std::function<void( UICodeEditor* codeEditor, const std::string& path )> onLoaded ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
	if ( FileSystem::isDirectory( path ) )
		return;
	if ( nullptr == codeEditor )
		codeEditor = mCurEditor;
	codeEditor->setColorScheme( mColorSchemes[mCurrentColorScheme] );
	bool isUrl = String::startsWith( path, "https://" ) || String::startsWith( path, "http://" );
	if ( isUrl ) {
		codeEditor->loadAsyncFromURL(
			path, Http::Request::FieldTable(),
			[&, codeEditor, path, onLoaded]( std::shared_ptr<TextDocument>, bool ) {
				mClient->onDocumentLoaded( codeEditor, path );
				if ( onLoaded )
					onLoaded( codeEditor, path );
			} );
	} else {
		codeEditor->loadAsyncFromFile(
			path, pool, [&, codeEditor, path, onLoaded]( std::shared_ptr<TextDocument>, bool ) {
				mClient->onDocumentLoaded( codeEditor, path );
				if ( onLoaded )
					onLoaded( codeEditor, path );
			} );
	}
#else
	loadFileFromPath( path, codeEditor );
	if ( nullptr == codeEditor )
		codeEditor = mCurEditor;
	if ( onLoaded )
		onLoaded( codeEditor, path );
#endif
}

std::pair<UITab*, UICodeEditor*>
UICodeEditorSplitter::loadFileFromPathInNewTab( const std::string& path ) {
	auto d = createCodeEditorInTabWidget( tabWidgetFromWidget( mCurWidget ) );
	if ( d.first == nullptr || d.second == nullptr ) {
		if ( !mTabWidgets.empty() && mTabWidgets[0]->getTabCount() > 0 ) {
			d = createCodeEditorInTabWidget( mTabWidgets[0] );
		} else {
			Log::error( "Couldn't createCodeEditorInTabWidget in "
						"UICodeEditorSplitter::loadFileFromPathInNewTab" );
			return d;
		}
	}
	UITabWidget* tabWidget = d.first->getTabWidget();
	loadFileFromPath( path, d.second );
	tabWidget->setTabSelected( d.first );
	return d;
}

void UICodeEditorSplitter::loadAsyncFileFromPathInNewTab(
	const std::string& path, std::shared_ptr<ThreadPool> pool,
	std::function<void( UICodeEditor*, const std::string& )> onLoaded, UITabWidget* tabWidget ) {
	auto d = createCodeEditorInTabWidget( tabWidget );
	if ( d.first == nullptr || d.second == nullptr ) {
		if ( !mTabWidgets.empty() && mTabWidgets[0]->getTabCount() > 0 ) {
			d = createCodeEditorInTabWidget( mTabWidgets[0] );
		} else {
			Log::error( "Couldn't createCodeEditorInTabWidget in "
						"UICodeEditorSplitter::loadAsyncFileFromPathInNewTab" );
			return;
		}
	}
	UITab* addedTab = d.first;
	loadAsyncFileFromPath( path, pool, d.second, onLoaded );
	tabWidget->setTabSelected( addedTab );
}

void UICodeEditorSplitter::loadAsyncFileFromPathInNewTab(
	const std::string& path, std::shared_ptr<ThreadPool> pool,
	std::function<void( UICodeEditor*, const std::string& )> onLoaded ) {
	auto d = createCodeEditorInTabWidget( tabWidgetFromWidget( mCurWidget ) );
	if ( d.first == nullptr || d.second == nullptr ) {
		if ( !mTabWidgets.empty() && mTabWidgets[0]->getTabCount() > 0 ) {
			d = createCodeEditorInTabWidget( mTabWidgets[0] );
		} else {
			Log::error( "Couldn't createCodeEditorInTabWidget in "
						"UICodeEditorSplitter::loadAsyncFileFromPathInNewTab" );
			return;
		}
	}
	UITabWidget* tabWidget = d.first->getTabWidget();
	UITab* addedTab = d.first;
	loadAsyncFileFromPath( path, pool, d.second, onLoaded );
	tabWidget->setTabSelected( addedTab );
}

void UICodeEditorSplitter::setCurrentEditor( UICodeEditor* editor ) {
	eeASSERT( checkEditorExists( editor ) );
	bool isNew = mCurEditor != editor;
	bool isNewW = mCurWidget != editor;
	mCurEditor = editor;
	mCurWidget = editor;
	if ( isNewW )
		mClient->onWidgetFocusChange( editor );
	if ( isNew )
		mClient->onCodeEditorFocusChange( editor );
	if ( editor )
		mClient->onDocumentStateChanged( editor, editor->getDocument() );
}

void UICodeEditorSplitter::setCurrentWidget( UIWidget* curWidget ) {
	if ( curWidget->isType( UI_TYPE_CODEEDITOR ) ) {
		setCurrentEditor( curWidget->asType<UICodeEditor>() );
		return;
	}
	bool isNewW = mCurWidget != curWidget;
	mCurWidget = curWidget;
	if ( isNewW )
		mClient->onWidgetFocusChange( curWidget );
}

std::pair<UITab*, UICodeEditor*>
UICodeEditorSplitter::createCodeEditorInTabWidget( UITabWidget* tabWidget ) {
	eeASSERT( curWidgetExists() );
	if ( nullptr == tabWidget )
		return std::make_pair( (UITab*)nullptr, (UICodeEditor*)nullptr );
	UICodeEditor* editor = createCodeEditor();
	mAboutToAddEditor = editor;
	editor->addEventListener( Event::OnDocumentChanged, [&]( const Event* event ) {
		mClient->onDocumentStateChanged( event->getNode()->asType<UICodeEditor>(),
										 event->getNode()->asType<UICodeEditor>()->getDocument() );
	} );
	UITab* tab = tabWidget->add( editor->getDocument().getFilename(), editor );
	editor->setData( (UintPtr)tab );
	DocEvent docEvent( editor, &editor->getDocument(), Event::OnEditorTabReady );
	editor->sendEvent( static_cast<const Event*>( &docEvent ) );
	mAboutToAddEditor = nullptr;
	mFirstCodeEditor = false;
	return std::make_pair( tab, editor );
}

std::pair<UITab*, UIWidget*>
UICodeEditorSplitter::createWidgetInTabWidget( UITabWidget* tabWidget, UIWidget* widget,
											   const std::string& tabName, bool focus ) {
	eeASSERT( curWidgetExists() );
	if ( nullptr == tabWidget )
		return std::make_pair( (UITab*)nullptr, (UIWidget*)nullptr );
	UITab* tab = tabWidget->add( tabName, widget );
	widget->setData( (UintPtr)tab );
	widget->addEventListener( Event::OnFocus, [&]( const Event* event ) {
		setCurrentWidget( event->getNode()->asType<UIWidget>() );
	} );
	widget->addEventListener( Event::OnTitleChange, [&]( const Event* event ) {
		const TextEvent* tevent = static_cast<const TextEvent*>( event );
		UIWidget* widget = event->getNode()->asType<UIWidget>();
		UITabWidget* tabWidget = tabWidgetFromWidget( widget );
		UITab* tab = tabWidget->getTabFromOwnedWidget( widget );
		if ( !tab )
			return;
		tab->setText( tevent->getText() );
	} );
	if ( focus )
		tabWidget->setTabSelected( tab );
	return std::make_pair( tab, widget );
}

void UICodeEditorSplitter::removeUnusedTab( UITabWidget* tabWidget, bool destroyOwnedNode,
											bool immediateClose ) {
	if ( tabWidget && tabWidget->getTabCount() >= 2 &&
		 tabWidget->getTab( 0 )->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) &&
		 tabWidget->getTab( 0 )
			 ->getOwnedWidget()
			 ->asType<UICodeEditor>()
			 ->getDocument()
			 .isEmpty() ) {
		tabWidget->removeTab( (Uint32)0, destroyOwnedNode, immediateClose );
	}
}

UITabWidget* UICodeEditorSplitter::createEditorWithTabWidget( Node* parent, bool openCurEditor ) {
	eeASSERT( curWidgetExists() );
	if ( nullptr == mBaseLayout )
		mBaseLayout = parent;
	UICodeEditor* prevCurEditor = mCurEditor;
	UITabWidget* tabWidget = UITabWidget::New();
	tabWidget->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	tabWidget->setParent( parent );
	tabWidget->setTabsClosable( true );
	tabWidget->setHideTabBarOnSingleTab( mHideTabBarOnSingleTab );
	tabWidget->setAllowRearrangeTabs( true );
	tabWidget->setAllowDragAndDropTabs( true );
	tabWidget->setAllowSwitchTabsInEmptySpaces( true );
	tabWidget->addEventListener( Event::OnTabSelected, [&]( const Event* event ) {
		UITabWidget* tabWidget = event->getNode()->asType<UITabWidget>();
		if ( tabWidget->getTabSelected()->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
			setCurrentEditor(
				tabWidget->getTabSelected()->getOwnedWidget()->asType<UICodeEditor>() );
		} else {
			setCurrentWidget( tabWidget->getTabSelected()->getOwnedWidget()->asType<UIWidget>() );
		}
	} );
	tabWidget->setTabTryCloseCallback( [&]( UITab* tab ) -> bool {
		if ( tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) ) {
			tryTabClose( tab->getOwnedWidget()->asType<UICodeEditor>() );
			return false;
		}
		return true;
	} );
	tabWidget->addEventListener( Event::OnTabClosed, [&]( const Event* event ) {
		onTabClosed( static_cast<const TabEvent*>( event ) );
	} );
	auto editorData = createCodeEditorInTabWidget( tabWidget );
	if ( editorData.first == nullptr || editorData.second == nullptr ) {
		if ( !mTabWidgets.empty() && mTabWidgets[0]->getTabCount() > 0 ) {
			editorData = createCodeEditorInTabWidget( mTabWidgets[0] );
		} else {
			Log::error( "Couldn't createCodeEditorInTabWidget in "
						"UICodeEditorSplitter::createEditorWithTabWidget" );
			return nullptr;
		}
	}
	mAboutToAddEditor = editorData.second;
	// Open same document in the new split
	if ( openCurEditor && prevCurEditor && prevCurEditor != editorData.second &&
		 !prevCurEditor->getDocument().isEmpty() )
		editorData.second->setDocument( prevCurEditor->getDocumentRef() );
	mAboutToAddEditor = nullptr;
	Lock l( mTabWidgetMutex );
	mTabWidgets.push_back( tabWidget );
	return tabWidget;
}

UITab* UICodeEditorSplitter::isDocumentOpen( std::string path, bool checkOnlyInCurrentTabWidget,
											 bool checkOpeningDocuments ) const {
	if ( String::startsWith( path, "file://" ) )
		path = path.substr( 7 );
	if ( checkOnlyInCurrentTabWidget ) {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( nullptr == tabWidget )
			return nullptr;
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			UITab* tab = tabWidget->getTab( i );
			if ( !tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) )
				continue;
			UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();

			if ( editor->getDocument().getFilePath() == path ||
				 ( checkOpeningDocuments && editor->getDocument().getLoadingFilePath() == path ) ) {
				return tab;
			}
		}
	} else {
		for ( auto tabWidget : mTabWidgets ) {
			for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
				UITab* tab = tabWidget->getTab( i );
				if ( !tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) )
					continue;
				UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();

				if ( editor->getDocument().getFilePath() == path ||
					 ( checkOpeningDocuments &&
					   editor->getDocument().getLoadingFilePath() == path ) ) {
					return tab;
				}
			}
		}
	}
	return nullptr;
}

UICodeEditor* UICodeEditorSplitter::editorFromTab( UITab* tab ) const {
	return tab->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR )
			   ? tab->getOwnedWidget()->asType<UICodeEditor>()
			   : nullptr;
}

UICodeEditor* UICodeEditorSplitter::findEditorFromPath( const std::string& path ) {
	UICodeEditor* editor = nullptr;
	forEachEditorStoppable( [&, path]( UICodeEditor* curEditor ) -> bool {
		if ( curEditor->getDocument().getFilePath() == path ) {
			editor = curEditor;
			return true;
		}
		return false;
	} );
	return editor;
}

void UICodeEditorSplitter::applyColorScheme( const SyntaxColorScheme& colorScheme ) {
	forEachEditor(
		[colorScheme]( UICodeEditor* editor ) { editor->setColorScheme( colorScheme ); } );
	mClient->onColorSchemeChanged( mCurrentColorScheme );
}

void UICodeEditorSplitter::forEachWidgetType( const UINodeType& nodeType,
											  std::function<void( UIWidget* )> run ) const {
	Node* node;
	UIWidget* widget;
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			node = tabWidget->getTab( i )->getOwnedWidget();
			if ( node && node->isWidget() ) {
				widget = node->asType<UIWidget>();
				if ( widget->isType( nodeType ) )
					run( widget );
			}
		}
	}
}

void UICodeEditorSplitter::forEachWidgetTypeStoppable(
	const UINodeType& nodeType, std::function<bool( UIWidget* )> run ) const {
	Node* node;
	UIWidget* widget;
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			node = tabWidget->getTab( i )->getOwnedWidget();
			if ( node && node->isWidget() ) {
				widget = node->asType<UIWidget>();
				if ( widget->isType( nodeType ) && run( widget ) )
					return;
			}
		}
	}
}

void UICodeEditorSplitter::forEachEditor( std::function<void( UICodeEditor* )> run ) const {
	for ( auto tabWidget : mTabWidgets )
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) )
				run( tabWidget->getTab( i )->getOwnedWidget()->asType<UICodeEditor>() );
		}
}

void UICodeEditorSplitter::forEachDoc( std::function<void( TextDocument& )> run ) const {
	std::unordered_set<TextDocument*> docs;
	forEachEditor( [&]( UICodeEditor* editor ) { docs.insert( editor->getDocumentRef().get() ); } );
	for ( auto doc : docs )
		run( *doc );
}

void UICodeEditorSplitter::forEachTabWidget( std::function<void( UITabWidget* )> run ) const {
	for ( auto widget : mTabWidgets )
		run( widget );
}

void UICodeEditorSplitter::forEachEditorStoppable(
	std::function<bool( UICodeEditor* )> run ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) &&
				 run( tabWidget->getTab( i )->getOwnedWidget()->asType<UICodeEditor>() ) ) {
				return;
			}
		}
	}
}

void UICodeEditorSplitter::forEachWidgetStoppable( std::function<bool( UIWidget* )> run ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isWidget() &&
				 run( tabWidget->getTab( i )->getOwnedWidget()->asType<UIWidget>() ) ) {
				return;
			}
		}
	}
}

void UICodeEditorSplitter::forEachWidget( std::function<void( UIWidget* )> run ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isWidget() )
				run( tabWidget->getTab( i )->getOwnedWidget()->asType<UIWidget>() );
		}
	}
}

void UICodeEditorSplitter::forEachDocStoppable( std::function<bool( TextDocument& )> run ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isType( UI_TYPE_CODEEDITOR ) &&
				 run( *tabWidget->getTab( i )
						   ->getOwnedWidget()
						   ->asType<UICodeEditor>()
						   ->getDocumentRef()
						   .get() ) ) {
				return;
			}
		}
	}
}

std::shared_ptr<TextDocument> UICodeEditorSplitter::findDocFromPath( const std::string& path ) {
	std::unordered_set<std::shared_ptr<TextDocument>> docs;
	forEachEditor( [&]( UICodeEditor* editor ) { docs.insert( editor->getDocumentRef() ); } );
	for ( const auto& doc : docs ) {
		if ( doc->getFilePath() == path )
			return doc;
	}
	return {};
}

bool UICodeEditorSplitter::getHideTabBarOnSingleTab() const {
	return mHideTabBarOnSingleTab;
}

void UICodeEditorSplitter::setHideTabBarOnSingleTab( bool hideTabBarOnSingleTab ) {
	if ( hideTabBarOnSingleTab != mHideTabBarOnSingleTab ) {
		mHideTabBarOnSingleTab = hideTabBarOnSingleTab;

		for ( auto widget : mTabWidgets )
			widget->setHideTabBarOnSingleTab( hideTabBarOnSingleTab );
	}
}

const std::vector<UITabWidget*>& UICodeEditorSplitter::getTabWidgets() const {
	return mTabWidgets;
}

Node* UICodeEditorSplitter::getBaseLayout() const {
	return mBaseLayout;
}

UIWidget* UICodeEditorSplitter::getCurWidget() const {
	return mCurWidget;
}

std::vector<UICodeEditor*> UICodeEditorSplitter::getAllEditors() {
	std::vector<UICodeEditor*> editors;
	forEachEditor( [&]( UICodeEditor* editor ) { editors.push_back( editor ); } );
	return editors;
}

bool UICodeEditorSplitter::isAnyEditorDirty() {
	bool any = false;
	forEachEditorStoppable( [&any]( UICodeEditor* editor ) {
		if ( editor->isDirty() ) {
			any = true;
			return true;
		}
		return false;
	} );
	return any;
}

void UICodeEditorSplitter::zoomIn() {
	forEachEditor( []( UICodeEditor* editor ) { editor->fontSizeGrow(); } );
}

void UICodeEditorSplitter::zoomOut() {
	forEachEditor( []( UICodeEditor* editor ) { editor->fontSizeShrink(); } );
}

void UICodeEditorSplitter::zoomReset() {
	forEachEditor( []( UICodeEditor* editor ) { editor->fontSizeReset(); } );
}

bool UICodeEditorSplitter::tryTabClose( UIWidget* widget ) {
	if ( !widget )
		return false;

	if ( widget->isType( UI_TYPE_CODEEDITOR ) ) {
		UICodeEditor* editor = widget->asType<UICodeEditor>();
		if ( nullptr != editor && editor->isDirty() &&
			 countEditorsOpeningDoc( editor->getDocument() ) == 1 ) {
			if ( nullptr != mTryCloseMsgBox )
				return false;
			mTryCloseMsgBox = UIMessageBox::New(
				UIMessageBox::OK_CANCEL,
				widget->getUISceneNode()->getTranslatorString(
					"@string/confirm_close_tab",
					"Do you really want to close this tab?\nAll changes will be lost." ) );
			mTryCloseMsgBox->addEventListener(
				Event::OnConfirm, [&, editor]( const Event* ) { closeTab( editor ); } );
			mTryCloseMsgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
				mTryCloseMsgBox = nullptr;
				if ( mCurEditor )
					mCurEditor->setFocus();
			} );
			mTryCloseMsgBox->setTitle( widget->getUISceneNode()->getTranslatorString(
				"@string/ask_close_tab", "Close Tab?" ) );
			mTryCloseMsgBox->center();
			mTryCloseMsgBox->show();
			return false;
		} else {
			closeTab( editor );
			return true;
		}
	} else {
		closeTab( widget );
		return true;
	}
}

void UICodeEditorSplitter::closeTab( UIWidget* widget ) {
	if ( widget ) {
		if ( widget->isType( UI_TYPE_CODEEDITOR ) ) {
			UICodeEditor* editor = widget->asType<UICodeEditor>();
			UITabWidget* tabWidget = tabWidgetFromEditor( editor );
			if ( tabWidget ) {
				if ( !( editor->getDocument().isEmpty() &&
						!tabWidget->getParent()->isType( UI_TYPE_SPLITTER ) &&
						tabWidget->getTabCount() == 1 ) ||
					 !editor->getDocument().isUntitledEmpty() ) {
					tabWidget->removeTab( (UITab*)editor->getData() );
				} else {
					return;
				}
			}
		} else {
			UITabWidget* tabWidget = tabWidgetFromWidget( widget );
			tabWidget->removeTab( (UITab*)widget->getData() );
		}
		if ( mCurEditor == widget )
			mCurEditor = nullptr;
		if ( mCurWidget == widget )
			mCurWidget = nullptr;
	}
}

bool UICodeEditorSplitter::curEditorExistsAndFocused() const {
	return mCurEditor && mCurEditor == mCurWidget;
}

UISplitter* UICodeEditorSplitter::split( const SplitDirection& direction, UIWidget* widget,
										 bool openCurEditor ) {
	if ( !widget )
		return nullptr;
	UIOrientation orientation =
		direction == SplitDirection::Left || direction == SplitDirection::Right
			? UIOrientation::Horizontal
			: UIOrientation::Vertical;
	UITabWidget* tabWidget = tabWidgetFromWidget( widget );
	if ( !tabWidget )
		return nullptr;
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
			return nullptr;
		}
	}

	UISplitter* splitter = UISplitter::New();
	splitter->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	splitter->setOrientation( orientation );
	tabWidget->detach();
	splitter->setParent( parent );
	tabWidget->setParent( splitter );
	createEditorWithTabWidget( splitter, openCurEditor );
	if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
		splitter->swap();

	if ( parentSplitter ) {
		if ( wasFirst && parentSplitter->getFirstWidget() != splitter ) {
			parentSplitter->swap();
		} else if ( !wasFirst && parentSplitter->getLastWidget() != splitter ) {
			parentSplitter->swap();
		}
	}

	return splitter;
}

void UICodeEditorSplitter::switchToTab( Int32 index ) {
	UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
	if ( tabWidget ) {
		tabWidget->setTabSelected( eeclamp<Int32>( index, 0, tabWidget->getTabCount() - 1 ) );
	}
}

UITabWidget* UICodeEditorSplitter::findPreviousSplit( UIWidget* widget ) {
	if ( !widget )
		return nullptr;
	UISplitter* splitter = splitterFromWidget( widget );
	if ( !splitter )
		return nullptr;
	UITabWidget* tabWidget = tabWidgetFromWidget( widget );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.rbegin(), mTabWidgets.rend(), tabWidget );
		if ( it != mTabWidgets.rend() && ++it != mTabWidgets.rend() ) {
			return *it;
		}
	}
	return nullptr;
}

void UICodeEditorSplitter::switchPreviousSplit( UIWidget* widget ) {
	UITabWidget* tabWidget = findPreviousSplit( widget );
	if ( tabWidget && tabWidget->getTabSelected() &&
		 tabWidget->getTabSelected()->getOwnedWidget() ) {
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} else {
		tabWidget = findNextSplit( widget );
		if ( tabWidget && tabWidget->getTabSelected() &&
			 tabWidget->getTabSelected()->getOwnedWidget() ) {
			tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
		}
	}
}

UITabWidget* UICodeEditorSplitter::findNextSplit( UIWidget* widget ) {
	if ( !widget )
		return nullptr;
	UISplitter* splitter = splitterFromWidget( widget );
	if ( !splitter )
		return nullptr;
	UITabWidget* tabWidget = tabWidgetFromWidget( widget );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
		if ( it != mTabWidgets.end() && ++it != mTabWidgets.end() ) {
			return *it;
		}
	}
	return nullptr;
}

void UICodeEditorSplitter::switchNextSplit( UIWidget* widget ) {
	UITabWidget* tabWidget = findNextSplit( widget );
	if ( tabWidget && tabWidget->getTabSelected() &&
		 tabWidget->getTabSelected()->getOwnedWidget() ) {
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} else {
		tabWidget = findPreviousSplit( widget );
		if ( tabWidget && tabWidget->getTabSelected() &&
			 tabWidget->getTabSelected()->getOwnedWidget() ) {
			tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
		}
	}
}

void UICodeEditorSplitter::focusSomeEditor( Node* searchFrom ) {
	UICodeEditor* editor =
		searchFrom && searchFrom->isType( UI_TYPE_CODEEDITOR )
			? searchFrom->findByType<UICodeEditor>( UI_TYPE_CODEEDITOR )
			: ( mBaseLayout ? mBaseLayout->findByType<UICodeEditor>( UI_TYPE_CODEEDITOR )
							: nullptr );

	UITabWidget* tabW = nullptr;
	if ( editor && ( tabW = tabWidgetFromEditor( editor ) ) && !tabW->isClosing() &&
		 tabW->getTabCount() > 1 ) {
		if ( tabW && tabW->getTabSelected()->getOwnedWidget() != editor ) {
			tabW->setTabSelected( tabW->getTabSelected() );
			return;
		} else if ( tabW ) {
			for ( size_t i = 0; i < tabW->getTabCount(); ++i ) {
				if ( tabW->getTab( i )->getOwnedWidget() != editor ) {
					tabW->setTabSelected( i );
					return;
				}
			}
		}
	}

	for ( auto widget : mTabWidgets ) {
		if ( !widget->isClosing() && widget->getTabCount() > 0 ) {
			if ( widget->getTabSelected() != nullptr ) {
				widget->setTabSelected( widget->getTabSelected() );
			} else {
				widget->setTabSelected( (Uint32)0 );
			}
			return;
		}
	}
}

void UICodeEditorSplitter::closeTabWidgets( UISplitter* splitter ) {
	Node* node = splitter->getFirstChild();
	while ( node ) {
		if ( node->isType( UI_TYPE_TABWIDGET ) ) {
			auto it =
				std::find( mTabWidgets.begin(), mTabWidgets.end(), node->asType<UITabWidget>() );
			if ( it != mTabWidgets.end() ) {
				Lock l( mTabWidgetMutex );
				mTabWidgets.erase( it );
			}
		} else if ( node->isType( UI_TYPE_SPLITTER ) ) {
			closeTabWidgets( node->asType<UISplitter>() );
		}
		node = node->getNextNode();
	}
}

bool UICodeEditorSplitter::checkEditorExists( UICodeEditor* checkEditor ) const {
	bool found = false;
	forEachEditorStoppable( [&found, checkEditor]( UICodeEditor* editor ) {
		if ( editor == checkEditor ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found || checkEditor == nullptr || mAboutToAddEditor == checkEditor || mFirstCodeEditor;
}

bool UICodeEditorSplitter::curEditorExists() const {
	bool found = false;
	forEachEditorStoppable( [&]( UICodeEditor* editor ) {
		if ( editor == mCurEditor ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found || mCurEditor == nullptr || mAboutToAddEditor == mCurEditor || mFirstCodeEditor;
}

bool UICodeEditorSplitter::curWidgetExists() const {
	bool found = false;
	forEachWidgetStoppable( [&]( UIWidget* widget ) {
		if ( widget == mCurWidget ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found || mCurWidget == nullptr;
}

bool UICodeEditorSplitter::isCurEditor( UICodeEditor* editor ) {
	return mCurEditor == editor;
}

UICodeEditor* UICodeEditorSplitter::getSomeEditor() {
	UICodeEditor* ed = nullptr;
	forEachEditorStoppable( [&]( UICodeEditor* editor ) {
		ed = editor;
		return true;
	} );
	return ed;
}

size_t UICodeEditorSplitter::countEditorsOpeningDoc( const TextDocument& doc ) const {
	size_t count = 0;
	const TextDocument* docPtr = &doc;
	forEachEditor( [&count, docPtr]( UICodeEditor* editor ) {
		const TextDocument* editorDocPtr = &editor->getDocument();
		if ( editorDocPtr == docPtr )
			++count;
	} );
	return count;
}

UISceneNode* UICodeEditorSplitter::getUISceneNode() const {
	return mUISceneNode;
}

UICodeEditor* UICodeEditorSplitter::getCurEditor() const {
	eeASSERT( curEditorExists() );
	return mCurEditor;
}

bool UICodeEditorSplitter::curEditorIsNotNull() const {
	return mCurEditor != nullptr;
}

void UICodeEditorSplitter::addRemainingTabWidgets( Node* widget ) {
	if ( widget->isType( UI_TYPE_TABWIDGET ) ) {
		if ( std::find( mTabWidgets.begin(), mTabWidgets.end(), widget->asType<UITabWidget>() ) ==
			 mTabWidgets.end() ) {
			Lock l( mTabWidgetMutex );
			mTabWidgets.push_back( widget->asType<UITabWidget>() );
		}
	} else if ( widget->isType( UI_TYPE_SPLITTER ) ) {
		UISplitter* splitter = widget->asType<UISplitter>();
		addRemainingTabWidgets( splitter->getFirstWidget() );
		addRemainingTabWidgets( splitter->getLastWidget() );
	}
}

void UICodeEditorSplitter::closeSplitter( UISplitter* splitter ) {
	splitter->setParent( mUISceneNode->getRoot() );
	splitter->setVisible( false );
	splitter->setEnabled( false );
	splitter->close();
	closeTabWidgets( splitter );
}

void UICodeEditorSplitter::onTabClosed( const TabEvent* tabEvent ) {
	UIWidget* widget = tabEvent->getTab()->getOwnedWidget()->asType<UIWidget>();
	UITabWidget* tabWidget = tabEvent->getTab()->getTabWidget();
	if ( tabWidget->getTabCount() == 0 ) {
		UISplitter* splitter = splitterFromWidget( widget );
		if ( splitter ) {
			if ( splitter->isFull() ) {
				tabWidget->close();
				auto itWidget = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
				if ( itWidget != mTabWidgets.end() ) {
					Lock l( mTabWidgetMutex );
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
					if ( remainingNode->isWidget() )
						remainingNode->asType<UIWidget>()->setLayoutSizePolicy(
							SizePolicy::MatchParent, SizePolicy::MatchParent );
					addRemainingTabWidgets( remainingNode );
					focusSomeEditor( nullptr );
				}

				eeASSERT( !mTabWidgets.empty() );
				eeASSERT( !( mTabWidgets.size() == 1 && mTabWidgets[0]->getTabCount() == 0 ) );
				return;
			}
		}
		mCurEditor = nullptr;
		mCurWidget = nullptr;
		auto d = createCodeEditorInTabWidget( tabWidget );
		if ( d.first == nullptr || d.second == nullptr ) {
			Log::error( "Couldn't createCodeEditorInTabWidget in "
						"UICodeEditorSplitter::onTabClosed" );
			return;
		}
		d.first->getTabWidget()->setTabSelected( d.first );
	} else {
		if ( tabWidget->getTabSelectedIndex() >= tabWidget->getTabCount() )
			tabWidget->setTabSelected(
				eemin( tabWidget->getTabCount() - 1, tabEvent->getTabIndex() ) );
	}
	if ( tabEvent->getTab()->getOwnedWidget() == mCurEditor )
		focusSomeEditor( nullptr );
	eeASSERT( curWidgetExists() );
}

}}} // namespace EE::UI::Tools
