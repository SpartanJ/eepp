#include "plugin.hpp"
#include "pluginmanager.hpp"
#include <eepp/system/filesystem.hpp>

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

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [this, editor]( const Event* event ) {
			Lock l( mMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			mDocs.insert( docEvent->getDoc() );
			mEditorDocs[editor] = docEvent->getDoc();
			onDocumentLoaded( docEvent->getDoc() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentClosed, [this]( const Event* event ) {
			{
				Lock l( mMutex );
				const DocEvent* docEvent = static_cast<const DocEvent*>( event );
				TextDocument* doc = docEvent->getDoc();
				onDocumentClosed( doc );
				onUnregisterDocument( doc );
				mDocs.erase( doc );
			}
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [this, editor]( const Event* ) {
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
