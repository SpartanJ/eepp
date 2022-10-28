#include "lspplugin.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define LSP_THREADED 1
#else
#define LSP_THREADED 0
#endif

UICodeEditorPlugin* LSPPlugin::New( const PluginManager* pluginManager ) {
	return eeNew( LSPPlugin, ( pluginManager ) );
}

LSPPlugin::LSPPlugin( const PluginManager* pluginManager ) :
	mPool( pluginManager->getThreadPool() ) {
#if LSP_THREADED
	mPool->run( [&, pluginManager] { load( pluginManager ); }, [] {} );
#else
	load( pluginManager );
#endif
}

void LSPPlugin::load( const PluginManager* pluginManager ) {
	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/lsp.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "lsp.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n\"config\":{},\n\"lsp\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;
	for ( const auto& path : paths ) {
		try {
			loadLSPConfig( path );
		} catch ( json::exception& e ) {
			Log::error( "Parsing linter \"%s\" failed:\n%s", path.c_str(), e.what() );
		}
	}
	mReady = mClientManager.clientCount() > 0;
	if ( mReady )
		fireReadyCbs();
}

void LSPPlugin::loadLSPConfig( const std::string& path ) {

}

LSPPlugin::~LSPPlugin() {
	mClosing = true;
	Lock l( mDocMutex );
	for ( const auto& editor : mEditors ) {
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterPlugin( this );
	}
}

void LSPPlugin::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );
	std::vector<Uint32> listeners;
	listeners.push_back( editor->addEventListener( Event::OnDocumentLoaded, [&]( const Event* ) {

	} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentClosed, [&]( const Event* event ) {
			Lock l( mDocMutex );
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			TextDocument* doc = docEvent->getDoc();
			mDocs.erase( doc );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [&, editor]( const Event* ) {
			TextDocument* oldDoc = mEditorDocs[editor];
			TextDocument* newDoc = editor->getDocumentRef().get();
			Lock l( mDocMutex );
			mDocs.erase( oldDoc );
			mEditorDocs[editor] = newDoc;
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnCursorPosChange, [&, editor]( const Event* ) {} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentSyntaxDefinitionChange, [&]( const Event* ev ) {
			// const DocSyntaxDefEvent* event = static_cast<const DocSyntaxDefEvent*>( ev );
		} ) );

	mEditors.insert( { editor, listeners } );
	mDocs.insert( editor->getDocumentRef().get() );
	mEditorDocs[editor] = editor->getDocumentRef().get();
}

void LSPPlugin::onUnregister( UICodeEditor* editor ) {
	if ( mClosing )
		return;
	Lock l( mDocMutex );
	TextDocument* doc = mEditorDocs[editor];
	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	mEditors.erase( editor );
	mEditorDocs.erase( editor );
	for ( auto editor : mEditorDocs )
		if ( editor.second == doc )
			return;
	mDocs.erase( doc );
}

} // namespace ecode
