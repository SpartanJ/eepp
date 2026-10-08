#ifndef ECODE_FILESYSTEMLISTENER_HPP
#define ECODE_FILESYSTEMLISTENER_HPP

#include "boundedeventqueue.hpp"
#include "filesystemlistenercallbackstate.hpp"
#include "filesystemlisteneroptions.hpp"
#include "projectdirectorytree.hpp"
#include <atomic>
#include <eepp/system/fileinfo.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <efsw/efsw.hpp>
#include <unordered_map>

using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::UI::Tools;

namespace ecode {

class FileSystemListener : public efsw::FileWatchListener {
  public:
	typedef std::function<void( const FileEvent&, const FileInfo& )> FileEventFn;
	using ThreadAffinity = FileEventThreadAffinity;
	using ListenerOptions = FileSystemListenerOptions;
	static constexpr FileEventTypeMask eventTypeMask( FileSystemEventType type ) {
		return fileEventTypeMask( type );
	}

	FileSystemListener( UICodeEditorSplitter* codeSplitter,
						std::shared_ptr<FileSystemModel> fileSystemModel,
						const std::vector<std::string>& ignoreFiles );

	virtual ~FileSystemListener();

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, const std::string& oldFilename );

	void setFileSystemModel( std::shared_ptr<FileSystemModel> model ) { mFileSystemModel = model; }

	void setDirTree( const std::shared_ptr<ProjectDirectoryTree>& dirTree );

	Uint64 addListener( const FileEventFn& fn );

	Uint64 addListener( const FileEventFn& fn, ListenerOptions options );

	bool removeListener( const Uint64& id );

  protected:
	struct PendingFileAction {
		FileEvent event;
		FileInfo file;
		efsw::Action action;
	};
	struct Listener {
		FileEventFn callback;
		ListenerOptions options;
		FileSystemListenerCallbackState callbackState;

		Listener( FileEventFn callback, ListenerOptions options ) :
			callback( std::move( callback ) ), options( std::move( options ) ) {}
	};

	UICodeEditorSplitter* mSplitter;
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	std::shared_ptr<ProjectDirectoryTree> mDirTree;
	std::atomic<Uint64> mLastId{ 0 };
	std::unordered_map<Uint64, std::shared_ptr<Listener>> mCbs;
	std::vector<std::string> mIgnoredFiles;
	Mutex mCbsMutex;
	Mutex mDirTreeMutex;
	BoundedEventQueue<PendingFileAction> mPendingEvents;
	// Tag of the queued main-thread file-event actions; the destructor cancels
	// them and marks the lifetime token dead so a queued runnable can never
	// dereference the destroyed listener.
	Uint64 mEventActionTag{ 0 };
	std::shared_ptr<std::atomic<bool>> mLifetime;

	void enqueueFileAction( PendingFileAction&& event );

	void drainFileActions();

	void processFileAction( PendingFileAction&& event );

	void dispatchCallbacks( ThreadAffinity affinity, const FileEvent& event, const FileInfo& file,
							const std::string& eventPath, bool resolveLinks = false );

	bool isFileOpen( const FileInfo& file );

	void notifyChange( const FileInfo& file );

	void notifyMove( const FileInfo& oldFile, const FileInfo& newFile );

	void notifyDelete( const FileInfo& file );

	bool isIgnored( const std::string& path );
};

} // namespace ecode

#endif // ECODE_FILESYSTEMLISTENER_HPP
