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
		{ { KEY_W, KeyMod::getDefaultModifier() }, "close-doc" },
		{ { KEY_TAB, KeyMod::getDefaultModifier() }, "next-doc" },
		{ { KEY_TAB, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "previous-doc" },
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

UITabWidget* UICodeEditorSplitter::tabWidgetFromEditor( UICodeEditor* editor ) {
	if ( editor )
		return ( (UITab*)editor->getData() )->getTabWidget();
	return nullptr;
}

UISplitter* UICodeEditorSplitter::splitterFromEditor( UICodeEditor* editor ) {
	if ( editor && editor->getParent()->getParent()->getParent()->isType( UI_TYPE_SPLITTER ) )
		return editor->getParent()->getParent()->getParent()->asType<UISplitter>();
	return nullptr;
}

UICodeEditor* UICodeEditorSplitter::createCodeEditor() {
	UICodeEditor* editor = UICodeEditor::NewOpt( false, true );
	TextDocument& doc = editor->getDocument();
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
	doc.setCommand( "switch-to-first-tab", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() ) {
			switchToTab( 0 );
		}
	} );
	doc.setCommand( "switch-to-last-tab", [&] {
		UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
		if ( tabWidget && tabWidget->getTabCount() ) {
			switchToTab( tabWidget->getTabCount() - 1 );
		}
	} );
	doc.setCommand( "split-right", [&] { splitEditor( SplitDirection::Right, mCurEditor ); } );
	doc.setCommand( "split-bottom", [&] { splitEditor( SplitDirection::Bottom, mCurEditor ); } );
	doc.setCommand( "split-left", [&] { splitEditor( SplitDirection::Left, mCurEditor ); } );
	doc.setCommand( "split-top", [&] { splitEditor( SplitDirection::Top, mCurEditor ); } );
	doc.setCommand( "split-swap", [&] {
		if ( UISplitter* splitter = splitterFromEditor( mCurEditor ) )
			splitter->swap();
	} );
	doc.setCommand( "open-containing-folder", [&] {
		if ( mCurEditor )
			mCurEditor->openContainingFolder();
	} );
	doc.setCommand( "copy-file-path", [&] {
		if ( mCurEditor )
			mCurEditor->copyFilePath();
	} );
	editor->addEventListener( Event::OnFocus, [&]( const Event* event ) {
		setCurrentEditor( event->getNode()->asType<UICodeEditor>() );
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
	editor->addUnlockedCommands( { "lock-toggle", "create-new", "close-doc", "next-doc",
								   "previous-doc", "split-left", "split-right", "split-top",
								   "split-bottom", "split-swap", "switch-to-previous-split",
								   "switch-to-next-split", "switch-to-previous-colorscheme",
								   "switch-to-next-colorscheme" } );

	if ( nullptr == mCurEditor )
		setCurrentEditor( editor );

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

bool UICodeEditorSplitter::editorExists( UICodeEditor* editor ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( editor == tabWidget->getTab( i )->getOwnedWidget() ) {
				return true;
			}
		}
	}
	return false;
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

void UICodeEditorSplitter::loadFileFromPathInNewTab( const std::string& path ) {
	auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
	UITabWidget* tabWidget = d.first->getTabWidget();
	UITab* addedTab = d.first;
	loadFileFromPath( path, d.second );
	tabWidget->setTabSelected( addedTab );
}

void UICodeEditorSplitter::loadAsyncFileFromPathInNewTab(
	const std::string& path, std::shared_ptr<ThreadPool> pool,
	std::function<void( UICodeEditor*, const std::string& )> onLoaded, UITabWidget* tabWidget ) {
	auto d = createCodeEditorInTabWidget( tabWidget );
	UITab* addedTab = d.first;
	loadAsyncFileFromPath( path, pool, d.second, onLoaded );
	tabWidget->setTabSelected( addedTab );
}

void UICodeEditorSplitter::loadAsyncFileFromPathInNewTab(
	const std::string& path, std::shared_ptr<ThreadPool> pool,
	std::function<void( UICodeEditor*, const std::string& )> onLoaded ) {
	auto d = createCodeEditorInTabWidget( tabWidgetFromEditor( mCurEditor ) );
	UITabWidget* tabWidget = d.first->getTabWidget();
	UITab* addedTab = d.first;
	loadAsyncFileFromPath( path, pool, d.second, onLoaded );
	tabWidget->setTabSelected( addedTab );
}

void UICodeEditorSplitter::setCurrentEditor( UICodeEditor* editor ) {
	bool isNew = mCurEditor != editor;
	mCurEditor = editor;
	if ( isNew )
		mClient->onCodeEditorFocusChange( editor );
	if ( editor )
		mClient->onDocumentStateChanged( editor, editor->getDocument() );
}

std::pair<UITab*, UICodeEditor*>
UICodeEditorSplitter::createCodeEditorInTabWidget( UITabWidget* tabWidget ) {
	if ( nullptr == tabWidget )
		return std::make_pair( (UITab*)nullptr, (UICodeEditor*)nullptr );
	UICodeEditor* editor = createCodeEditor();
	editor->addEventListener( Event::OnDocumentChanged, [&]( const Event* event ) {
		mClient->onDocumentStateChanged( event->getNode()->asType<UICodeEditor>(),
										 event->getNode()->asType<UICodeEditor>()->getDocument() );
	} );
	UITab* tab = tabWidget->add( editor->getDocument().getFilename(), editor );
	editor->setData( (UintPtr)tab );
	DocEvent docEvent( editor, &editor->getDocument(), Event::OnEditorTabReady );
	editor->sendEvent( static_cast<const Event*>( &docEvent ) );
	return std::make_pair( tab, editor );
}

void UICodeEditorSplitter::removeUnusedTab( UITabWidget* tabWidget ) {
	if ( tabWidget && tabWidget->getTabCount() >= 2 &&
		 tabWidget->getTab( 0 )
			 ->getOwnedWidget()
			 ->asType<UICodeEditor>()
			 ->getDocument()
			 .isEmpty() ) {
		tabWidget->removeTab( (Uint32)0, true, true );
	}
}

UITabWidget* UICodeEditorSplitter::createEditorWithTabWidget( Node* parent, bool openCurEditor ) {
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
	if ( openCurEditor && prevCurEditor && prevCurEditor != editorData.second &&
		 !prevCurEditor->getDocument().isEmpty() )
		editorData.second->setDocument( prevCurEditor->getDocumentRef() );
	mTabWidgets.push_back( tabWidget );
	return tabWidget;
}

UITab* UICodeEditorSplitter::isDocumentOpen( const std::string& path ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			UITab* tab = tabWidget->getTab( i );
			UICodeEditor* editor = tab->getOwnedWidget()->asType<UICodeEditor>();

			if ( editor->getDocument().getFilePath() == path ) {
				return tab;
			}
		}
	}
	return nullptr;
}

void UICodeEditorSplitter::applyColorScheme( const SyntaxColorScheme& colorScheme ) {
	forEachEditor(
		[colorScheme]( UICodeEditor* editor ) { editor->setColorScheme( colorScheme ); } );
	mClient->onColorSchemeChanged( mCurrentColorScheme );
}

void UICodeEditorSplitter::forEachEditor( std::function<void( UICodeEditor* )> run ) {
	for ( auto tabWidget : mTabWidgets )
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ )
			run( tabWidget->getTab( i )->getOwnedWidget()->asType<UICodeEditor>() );
}

void UICodeEditorSplitter::forEachDoc( std::function<void( TextDocument& )> run ) {
	std::unordered_set<TextDocument*> docs;
	forEachEditor( [&]( UICodeEditor* editor ) { docs.insert( editor->getDocumentRef().get() ); } );
	for ( auto doc : docs )
		run( *doc );
}

void UICodeEditorSplitter::forEachTabWidget( std::function<void( UITabWidget* )> run ) {
	for ( auto widget : mTabWidgets )
		run( widget );
}

void UICodeEditorSplitter::forEachEditorStoppable( std::function<bool( UICodeEditor* )> run ) {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( run( tabWidget->getTab( i )->getOwnedWidget()->asType<UICodeEditor>() ) ) {
				return;
			}
		}
	}
}

void UICodeEditorSplitter::forEachDocStoppable( std::function<bool( TextDocument& )> run ) {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( run( *tabWidget->getTab( i )
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

bool UICodeEditorSplitter::tryTabClose( UICodeEditor* editor ) {
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

void UICodeEditorSplitter::closeEditorTab( UICodeEditor* editor ) {
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

UISplitter* UICodeEditorSplitter::splitEditor( const SplitDirection& direction,
											   UICodeEditor* editor, bool openCurEditor ) {
	if ( !editor )
		return nullptr;
	UIOrientation orientation =
		direction == SplitDirection::Left || direction == SplitDirection::Right
			? UIOrientation::Horizontal
			: UIOrientation::Vertical;
	UITabWidget* tabWidget = tabWidgetFromEditor( editor );
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
	UITabWidget* tabWidget = tabWidgetFromEditor( mCurEditor );
	if ( tabWidget ) {
		tabWidget->setTabSelected( eeclamp<Int32>( index, 0, tabWidget->getTabCount() - 1 ) );
	}
}

UITabWidget* UICodeEditorSplitter::findPreviousSplit( UICodeEditor* editor ) {
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

void UICodeEditorSplitter::switchPreviousSplit( UICodeEditor* editor ) {
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

UITabWidget* UICodeEditorSplitter::findNextSplit( UICodeEditor* editor ) {
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

void UICodeEditorSplitter::switchNextSplit( UICodeEditor* editor ) {
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

void UICodeEditorSplitter::focusSomeEditor( Node* searchFrom ) {
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

void UICodeEditorSplitter::closeTabWidgets( UISplitter* splitter ) {
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

UICodeEditor* UICodeEditorSplitter::getCurEditor() const {
	return mCurEditor;
}

void UICodeEditorSplitter::addRemainingTabWidgets( Node* widget ) {
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

void UICodeEditorSplitter::closeSplitter( UISplitter* splitter ) {
	splitter->setParent( mUISceneNode->getRoot() );
	splitter->setVisible( false );
	splitter->setEnabled( false );
	splitter->close();
	closeTabWidgets( splitter );
}

void UICodeEditorSplitter::onTabClosed( const TabEvent* tabEvent ) {
	UICodeEditor* editor = mCurEditor;
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
					if ( remainingNode->isWidget() )
						remainingNode->asType<UIWidget>()->setLayoutSizePolicy(
							SizePolicy::MatchParent, SizePolicy::MatchParent );
					addRemainingTabWidgets( remainingNode );
					focusSomeEditor( nullptr );
				}
				if ( tabEvent->getTab()->getOwnedWidget() == mCurEditor )
					setCurrentEditor( nullptr );
				return;
			}
		}
		auto d = createCodeEditorInTabWidget( tabWidget );
		d.first->getTabWidget()->setTabSelected( d.first );
	} else {
		if ( tabWidget->getTabSelectedIndex() >= tabWidget->getTabCount() )
			tabWidget->setTabSelected(
				eemin( tabWidget->getTabCount() - 1, tabEvent->getTabIndex() ) );
	}
	if ( tabEvent->getTab()->getOwnedWidget() == mCurEditor )
		setCurrentEditor( nullptr );
}

}}} // namespace EE::UI::Tools
