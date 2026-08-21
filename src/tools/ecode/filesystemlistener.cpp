#include "filesystemlistener.hpp"
#include <eepp/core/small_vector.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/time.hpp>
#include <eepp/window/engine.hpp>

namespace ecode {

static thread_local const void* currentFileSystemListenerCallback{ nullptr };

class FileSystemListenerCallbackGuard {
  public:
	FileSystemListenerCallbackGuard( FileSystemListenerCallbackState& state,
									 const void* listener ) :
		mState( state ), mPreviousCallback( currentFileSystemListenerCallback ) {
		currentFileSystemListenerCallback = listener;
	}

	~FileSystemListenerCallbackGuard() {
		currentFileSystemListenerCallback = mPreviousCallback;
		mState.endCallback();
	}

  private:
	FileSystemListenerCallbackState& mState;
	const void* mPreviousCallback;
};

std::string getFileSystemEventTypeName( FileSystemEventType action ) {
	switch ( action ) {
		case FileSystemEventType::Add:
			return "Add";
		case FileSystemEventType::Modified:
			return "Modified";
		case FileSystemEventType::Delete:
			return "Delete";
		case FileSystemEventType::Moved:
			return "Moved";
		default:
			return "Bad Action";
	}
}

FileSystemListener::FileSystemListener( UICodeEditorSplitter* splitter,
										std::shared_ptr<FileSystemModel> fileSystemModel,
										const std::vector<std::string>& ignoreFiles ) :
	mSplitter( splitter ), mFileSystemModel( fileSystemModel ), mIgnoredFiles( ignoreFiles ) {
	// Namespaced 64-bit tag: a stable class-name prefix plus an atomic instance
	// counter, which minimizes (but cannot mathematically eliminate) collision
	// with unrelated scene action tags.
	static std::atomic<Uint64> nextEventActionTag{ 0 };
	mEventActionTag =
		( static_cast<Uint64>( EE::String::hash( "ecode::FileSystemListener" ) ) << 32 ) |
		( ++nextEventActionTag & 0xFFFFFFFFULL );
	mLifetime = std::make_shared<std::atomic<bool>>( true );
}

FileSystemListener::~FileSystemListener() {
	// The lifetime token is complete only because destruction runs on the main
	// thread, where queued actions also execute: a runnable cannot be
	// interleaved between its token check and its use of `this`. Assert that
	// invariant; cross-thread destruction would need a synchronized control
	// block instead of an atomic flag.
	eeASSERT( !Engine::existsSingleton() || Engine::isMainThread() );
	*mLifetime = false;
	SmallVector<std::shared_ptr<Listener>, 8> listeners;
	{
		Lock l( mCbsMutex );
		for ( auto& listener : mCbs )
			listeners.emplace_back( std::move( listener.second ) );
		mCbs.clear();
	}
	for ( const auto& listener : listeners )
		listener->callbackState.removeAndWait( currentFileSystemListenerCallback ==
											   listener.get() );
	mPendingEvents.clear();
	if ( auto* scene = SceneManager::instance()->getUISceneNode() )
		scene->getActionManager()->removeActionsByTagFromTarget( scene, mEventActionTag );
}

static inline bool endsWithSlash( const std::string& dir ) {
	return !dir.empty() && ( dir.back() == '\\' || dir.back() == '/' );
}

void FileSystemListener::handleFileAction( efsw::WatchID, const std::string& dir,
										   const std::string& filename, efsw::Action action,
										   const std::string& oldFilename ) {
	eeASSERT( !Engine::isMainThread() );
	FileEvent event( (FileSystemEventType)action, dir, filename, oldFilename );
	FileInfo file( ( endsWithSlash( dir ) ? dir : ( dir + FileSystem::getOSSlash() ) ) + filename,
				   false );
	std::shared_ptr<ProjectDirectoryTree> dirTree;
	{
		Lock l( mDirTreeMutex );
		dirTree = mDirTree;
	}
	if ( dirTree && action != efsw::Actions::Modified )
		dirTree->onChange( (ProjectDirectoryTree::Action)action, file, oldFilename );
	// Worker listeners receive the event early. The queued phase is independent:
	// it updates the model and documents and then dispatches main-thread listeners.
	dispatchCallbacks( ThreadAffinity::Worker, event, file, file.getFilepath(), true );
	enqueueFileAction( { std::move( event ), std::move( file ), action } );
}

void FileSystemListener::enqueueFileAction( PendingFileAction&& event ) {
	auto* scene = SceneManager::instance()->getUISceneNode();
	if ( !scene )
		return;

	const bool schedule = mPendingEvents.pushOrReplaceLast(
		std::move( event ), []( const PendingFileAction& previous, const PendingFileAction& next ) {
			return previous.action == efsw::Actions::Modified &&
				   next.action == efsw::Actions::Modified &&
				   previous.file.getFilepath() == next.file.getFilepath();
		} );
	if ( schedule ) {
		scene->runOnMainThread(
			[lifetime = mLifetime, this]() {
				if ( lifetime->load( std::memory_order_acquire ) )
					drainFileActions();
			},
			Time::Zero, mEventActionTag );
	}
}

void FileSystemListener::drainFileActions() {
	eeASSERT( Engine::isMainThread() );
	static constexpr std::size_t MaxEventsPerDrain = 64;
	SmallVector<PendingFileAction, MaxEventsPerDrain> events;
	mPendingEvents.popUpTo( events, MaxEventsPerDrain );

	for ( auto& event : events )
		processFileAction( std::move( event ) );

	if ( mPendingEvents.finishDrain() ) {
		if ( auto* scene = SceneManager::instance()->getUISceneNode() ) {
			scene->runOnMainThread(
				[lifetime = mLifetime, this]() {
					if ( lifetime->load( std::memory_order_acquire ) )
						drainFileActions();
				},
				Time::Zero, mEventActionTag );
		} else {
			mPendingEvents.clear();
		}
	}
}

void FileSystemListener::processFileAction( PendingFileAction&& pending ) {
	eeASSERT( Engine::isMainThread() );
	FileEvent& event = pending.event;
	const std::string& eventPath = pending.file.getFilepath();
	FileInfo file( pending.file );
	const efsw::Action action = pending.action;

	switch ( action ) {
		case efsw::Actions::Add:
		case efsw::Actions::Delete:
		case efsw::Actions::Moved: {
			if ( Log::instance() && Log::instance()->getLogLevelThreshold() == LogLevel::Debug ) {
				std::string txt =
					"DIR ( " + event.directory + " ) FILE ( " +
					( ( event.oldFilename.empty() ? ""
												  : "from file " + event.oldFilename + " to " ) +
					  event.filename ) +
					" ) has event " + getFileSystemEventTypeName( event.type );

				Log::debug( txt );
			}

			if ( mFileSystemModel )
				mFileSystemModel->handleFileEvent( event, file );

			if ( action == efsw::Actions::Moved ) {
				FileInfo oldFile( FileSystem::isRelativePath( event.oldFilename )
									  ? event.directory + event.oldFilename
									  : event.oldFilename );
				if ( file.isLink() )
					file = FileInfo( file.linksTo() );

				notifyMove( oldFile, file );

				if ( oldFile.isLink() ) {
					oldFile = FileInfo( oldFile.linksTo() );

					notifyMove( oldFile, file );
				}
			} else if ( action == efsw::Actions::Delete ) {
				notifyDelete( file );
			}

			if ( file.isLink() )
				file = FileInfo( file.linksTo() );

			if ( isFileOpen( file ) )
				notifyChange( file );

			dispatchCallbacks( ThreadAffinity::Main, event, file, eventPath );

			break;
		}
		case efsw::Actions::Modified: {
			if ( file.isLink() )
				file = FileInfo( file.linksTo() );
			if ( isFileOpen( file ) )
				notifyChange( file );

			dispatchCallbacks( ThreadAffinity::Main, event, file, eventPath );
		}
	}
}

void FileSystemListener::setDirTree( const std::shared_ptr<ProjectDirectoryTree>& dirTree ) {
	Lock l( mDirTreeMutex );
	mDirTree = dirTree;
}

Uint64 FileSystemListener::addListener( const FileEventFn& fn ) {
	return addListener( fn, {} );
}

Uint64 FileSystemListener::addListener( const FileEventFn& fn, ListenerOptions options ) {
	Lock l( mCbsMutex );
	Uint64 id = ++mLastId;
	mCbs[id] = std::make_shared<Listener>( fn, std::move( options ) );
	return id;
}

void FileSystemListener::dispatchCallbacks( ThreadAffinity affinity, const FileEvent& event,
											const FileInfo& file, const std::string& eventPath,
											bool resolveLinks ) {
	SmallVector<std::shared_ptr<Listener>, 8> callbacks;
	{
		Lock l( mCbsMutex );
		for ( const auto& entry : mCbs ) {
			const Listener& listener = *entry.second;
			if ( listener.options.affinity != affinity )
				continue;
			bool matches = listener.options.matches( event.type, eventPath );
			if ( !matches && event.type == FileSystemEventType::Moved &&
				 !event.oldFilename.empty() ) {
				if ( FileSystem::isRelativePath( event.oldFilename ) ) {
					matches = listener.options.matchesJoinedPath(
						event.type, event.directory,
						endsWithSlash( event.directory ) ? 0 : FileSystem::getOSSlash()[0],
						event.oldFilename );
				} else {
					matches = listener.options.matches( event.type, event.oldFilename );
				}
			}
			if ( matches )
				callbacks.emplace_back( entry.second );
		}
	}
	if ( callbacks.empty() )
		return;
	auto invokeCallback = [&]( const std::shared_ptr<Listener>& listener,
							   const FileInfo& callbackFile ) {
		if ( !listener->callbackState.beginCallback() )
			return;
		FileSystemListenerCallbackGuard guard( listener->callbackState, listener.get() );
		listener->callback( event, callbackFile );
	};
	if ( resolveLinks && file.isLink() ) {
		FileInfo resolvedFile( file.linksTo() );
		for ( const auto& listener : callbacks )
			invokeCallback( listener, resolvedFile );
		return;
	}
	for ( const auto& listener : callbacks )
		invokeCallback( listener, file );
}

bool FileSystemListener::removeListener( const Uint64& id ) {
	std::shared_ptr<Listener> listener;
	{
		Lock l( mCbsMutex );
		auto it = mCbs.find( id );
		if ( it == mCbs.end() )
			return false;
		listener = std::move( it->second );
		mCbs.erase( it );
	}
	listener->callbackState.removeAndWait( currentFileSystemListenerCallback == listener.get() );
	return true;
}

bool FileSystemListener::isFileOpen( const FileInfo& file ) {
	bool found = false;
	mSplitter->forEachDocStoppable( [&]( TextDocument& doc ) {
		if ( file.getFilepath() == doc.getFileInfo().getFilepath() ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found;
}

void FileSystemListener::notifyChange( const FileInfo& file ) {
	mSplitter->forEachDoc( [&]( TextDocument& doc ) {
		if ( !isIgnored( file.getFilepath() ) &&
			 file.getFilepath() == doc.getFileInfo().getFilepath() &&
			 file.getModificationTime() != doc.getFileInfo().getModificationTime() &&
			 !doc.isSaving() ) {
			MD5::Digest curHash = MD5::fromFile( file.getFilepath() ).digest;
			if ( curHash != doc.getHash() || doc.isDirty() ) {
				Log::notice( "Document: \"%s\" has changed on the file system:",
							 file.getFilepath().c_str() );
				Log::notice( "Modification time on file system: %u vs %u in memory",
							 file.getModificationTime(), doc.getFileInfo().getModificationTime() );
				Log::notice( "Hash on file system: %s vs %s in memory",
							 MD5::hexDigest( curHash ).c_str(),
							 MD5::hexDigest( doc.getHash() ).c_str() );
				doc.setDirtyOnFileSystem( true );
			}
		}
	} );
}

void FileSystemListener::notifyMove( const FileInfo& oldFile, const FileInfo& newFile ) {
	std::string oldPath( oldFile.getFilepath() );
	std::string newPath( newFile.getFilepath() );
	const bool directoryMoved = newFile.isDirectory();
	if ( directoryMoved ) {
		FileSystem::dirAddSlashAtEnd( oldPath );
		FileSystem::dirAddSlashAtEnd( newPath );
	}

	mSplitter->forEachDoc( [&]( TextDocument& doc ) {
		const std::string& documentPath = doc.getFileInfo().getFilepath();
		if ( oldPath == documentPath ) {
			doc.notifyDocumentMoved( newPath );
		} else if ( directoryMoved && String::startsWith( documentPath, oldPath ) ) {
			doc.notifyDocumentMoved( newPath + documentPath.substr( oldPath.size() ) );
		}
	} );
}

void FileSystemListener::notifyDelete( const FileInfo& file ) {
	mSplitter->forEachDoc( [&]( TextDocument& doc ) {
		if ( file.getFilepath() == doc.getFileInfo().getFilepath() )
			doc.setDirtyOnFileSystem( true );
	} );
}

bool FileSystemListener::isIgnored( const std::string& path ) {
	return std::find( mIgnoredFiles.begin(), mIgnoredFiles.end(), path ) != mIgnoredFiles.end();
}

} // namespace ecode
