#include "plugin.hpp"
#include "pluginmanager.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace ecode {

Plugin::Plugin( PluginManager* manager ) :
	mManager( manager ),
	mThreadPool( manager->getThreadPool() ),
	mReady( false ), // All plugins will start as "not ready" until proven the contrary
	mLoading( true ) // All plugins will start as "loading" until the load is complete, this is to
					 // avoid concurrency issues
{}

bool Plugin::isReady() const {
	return mReady;
}

bool Plugin::isLoading() const {
	return mLoading;
}

bool Plugin::isShuttingDown() const {
	return mShuttingDown;
}

bool Plugin::hasFileConfig() {
	return !mConfigPath.empty();
}

void Plugin::subscribeFileSystemListener() {
	mConfigFileInfo = FileInfo( mConfigPath );
	mManager->subscribeFileSystemListener( this );
}

void Plugin::unsubscribeFileSystemListener() {
	mManager->unsubscribeFileSystemListener( this );
}

std::string Plugin::getFileConfigPath() {
	return mConfigPath;
}

PluginManager* Plugin::getManager() const {
	return mManager;
}

PluginContextProvider* Plugin::getPluginContext() const {
	return mManager ? mManager->getPluginContext() : nullptr;
}

UISceneNode* Plugin::getUISceneNode() const {
	return mManager->getUISceneNode();
}

String Plugin::i18n( const std::string& key, const String& def ) const {
	return getManager()->getUISceneNode()->i18n( key, def );
}

UIIcon* Plugin::findIcon( const std::string& iconName ) {
	return getManager()->getUISceneNode()->findIcon( iconName );
}

Drawable* Plugin::iconDrawable( const std::string& iconName, Float dpSize ) {
	UIIcon* icon = findIcon( iconName );
	return icon ? icon->getSize( PixelDensity::dpToPx( dpSize ) ) : nullptr;
}

void Plugin::showMessage( LSPMessageType type, const std::string& message,
						  const std::string& title ) {
	if ( !mManager )
		return;
	LSPShowMessageParams msgReq{ type, message, { { title } } };
	mManager->sendBroadcast( PluginMessageType::ShowMessage, PluginMessageFormat::ShowMessage,
							 &msgReq );
}

void Plugin::waitUntilLoaded() {
	while ( mLoading )
		Sys::sleep( Milliseconds( 1 ) );
}

void Plugin::onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) {
	if ( ev.type != FileSystemEventType::Modified || mShuttingDown || isLoading() )
		return;

	if ( file.getFilepath() != mConfigPath ||
		 file.getModificationTime() == mConfigFileInfo.getModificationTime() )
		return;

	std::string fileContents;
	FileSystem::fileGet( file.getFilepath(), fileContents );
	if ( getConfigFileHash() != String::hash( fileContents ) ) {
		if ( mManager->isPluginReloadEnabled() && !isLoading() && isReady() ) {
			mConfigFileInfo = file;
			unsubscribeFileSystemListener();
			mManager->reload( getId() );
		} else {
			Log::debug( "Plugin %s: Configuration file has been modified: %s. But "
						"plugin reload is not enabled.",
						getTitle().c_str(), mConfigPath.c_str() );
		}
	} else {
		Log::debug( "Plugin %s: Configuration file has been modified: %s. But contents "
					"are the same.",
					getTitle().c_str(), mConfigPath.c_str() );
	}
}

void Plugin::setReady( Time loadTime ) {
	if ( mReady ) {
		if ( loadTime != Time::Zero ) {
			Log::info( "Plugin: %s (PID %u) loaded in %s", getTitle(), Sys::getProcessID(),
					   loadTime.toString() );
		} else {
			Log::info( "Plugin: %s (PID %u) loaded", getTitle(), Sys::getProcessID() );
		}
	}
}

bool Plugin::editorExists( UICodeEditor* editor ) {
	return mManager->getSplitter() && mManager->getSplitter()->editorExists( editor );
}

void Plugin::createListView( UICodeEditor* editor, std::shared_ptr<Model> model,
							 const ModelEventCallback& onModelEventCb,
							 const std::function<void( UIListView* )> onCreateCb ) {
	UICodeEditorSplitter* splitter = getManager()->getSplitter();
	if ( nullptr == splitter || !editorExists( editor ) )
		return;
	editor->runOnMainThread( [model, editor, splitter, onModelEventCb, onCreateCb] {
		auto lvs = editor->findAllByClass( "editor_listview" );
		for ( auto* ilv : lvs )
			ilv->close();

		UIListView* lv = UIListView::New();
		lv->setParent( editor );
		lv->addClass( "editor_listview" );
		auto pos =
			editor->getRelativeScreenPosition( editor->getDocumentRef()->getSelection().start() );
		lv->setPixelsPosition( { pos.x, pos.y + editor->getLineHeight() } );
		if ( !lv->getParent()->getLocalBounds().contains(
				 lv->getLocalBounds().setPosition( lv->getPixelsPosition() ) ) ) {
			lv->setPixelsPosition( { pos.x, pos.y - lv->getPixelsSize().getHeight() } );
		}
		lv->setVisible( true );
		lv->getVerticalScrollBar()->reloadStyle( true, true, true );
		lv->setAutoExpandOnSingleColumn( false );
		lv->setModel( model );
		Float height = std::min( lv->getContentSize().y, lv->getRowHeight() * 8 );
		Float colWidth = lv->getMaxColumnContentWidth( 0 ) + PixelDensity::dpToPx( 4 );
		bool needsVScroll = lv->getContentSize().y > lv->getRowHeight() * 8;
		Float width = colWidth + lv->getPixelsPadding().getWidth() +
					  ( needsVScroll ? lv->getVerticalScrollBar()->getPixelsSize().getWidth() : 0 );
		lv->setPixelsSize( { width, height } );
		lv->setColumnWidth( 0, colWidth );
		lv->setScrollMode( needsVScroll ? ScrollBarMode::Auto : ScrollBarMode::AlwaysOff,
						   ScrollBarMode::AlwaysOff );
		if ( onCreateCb )
			onCreateCb( lv );
		lv->setSelection( model->index( 0 ) );
		lv->setFocus();
		Uint32 focusCb = lv->getUISceneNode()->getUIEventDispatcher()->addFocusEventCallback(
			[lv]( const auto&, Node* focus, Node* ) {
				if ( !lv->inParentTreeOf( focus ) && !lv->isClosing() )
					lv->close();
			} );
		Uint32 cursorCb =
			editor->on( Event::OnCursorPosChange, [lv, editor, splitter]( const Event* ) {
				if ( !lv->isClosing() ) {
					lv->close();
					if ( splitter->editorExists( editor ) )
						editor->setFocus();
				}
			} );
		lv->on( Event::KeyDown, [lv, splitter, editor]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == EE::Window::KEY_ESCAPE && !lv->isClosing() )
				lv->close();
			if ( splitter->editorExists( editor ) )
				editor->setFocus();
		} );
		lv->on( Event::OnModelEvent, [onModelEventCb]( const Event* event ) {
			const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
			if ( onModelEventCb )
				onModelEventCb( modelEvent );
		} );
		lv->on( Event::OnClose, [lv, editor, cursorCb, focusCb]( const Event* ) {
			lv->getUISceneNode()->getUIEventDispatcher()->removeFocusEventCallback( focusCb );
			editor->removeEventListener( cursorCb );
		} );
	} );
}

PluginBase::~PluginBase() {
	mShuttingDown = true;
	unsubscribeFileSystemListener();

	for ( auto editor : mEditors ) {
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterPlugin( this );
	}
}

void PluginBase::onRegister( UICodeEditor* editor ) {
	Lock l( mMutex );

	std::vector<Uint32> listeners;

	listeners.push_back( editor->on( Event::OnDocumentLoaded, [this, editor]( const Event* event ) {
		Lock l( mMutex );
		const DocEvent* docEvent = static_cast<const DocEvent*>( event );
		mDocs.insert( docEvent->getDoc() );
		mEditorDocs[editor] = docEvent->getDoc();
		onDocumentLoaded( docEvent->getDoc() );
	} ) );

	listeners.push_back( editor->on( Event::OnDocumentClosed, [this]( const Event* event ) {
		{
			Lock l( mMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			TextDocument* doc = docEvent->getDoc();
			onDocumentClosed( doc );
			onUnregisterDocument( doc );
			mDocs.erase( doc );
		}
	} ) );

	listeners.push_back( editor->on( Event::OnDocumentChanged, [this, editor]( const Event* ) {
		TextDocument* oldDoc = mEditorDocs[editor];
		TextDocument* newDoc = editor->getDocumentRef().get();
		Lock l( mMutex );
		mDocs.erase( oldDoc );
		mDocs.insert( newDoc );
		mEditorDocs[editor] = newDoc;
		onDocumentChanged( editor, oldDoc );
	} ) );

	onRegisterListeners( editor, listeners );

	mEditors.insert( { editor, listeners } );
	if ( mDocs.count( editor->getDocumentRef().get() ) == 0 ) {
		mDocs.insert( editor->getDocumentRef().get() );
		onRegisterDocument( editor->getDocumentRef().get() );
	}
	mEditorDocs[editor] = editor->getDocumentRef().get();

	onRegisterEditor( editor );
}

void PluginBase::onUnregister( UICodeEditor* editor ) {
	onBeforeUnregister( editor );
	if ( mShuttingDown )
		return;
	Lock l( mMutex );
	TextDocument* doc = mEditorDocs[editor];
	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	onUnregisterEditor( editor );
	mEditors.erase( editor );
	mEditorDocs.erase( editor );
	for ( auto editorIt : mEditorDocs )
		if ( editorIt.second == doc )
			return;

	onUnregisterDocument( doc );

	mDocs.erase( doc );
}

void PluginBase::onBeforeUnregister( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings )
		editor->getKeyBindings().removeCommandKeybind( kb.first );
}

void PluginBase::onRegisterEditor( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings ) {
		if ( !kb.second.empty() )
			editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}
}

void PluginBase::onUnregisterDocument( TextDocument* doc ) {
	for ( auto& kb : mKeyBindings )
		doc->removeCommand( kb.first );
}

} // namespace ecode
