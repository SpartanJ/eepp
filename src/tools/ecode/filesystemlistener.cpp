#include "filesystemlistener.hpp"
#include <eepp/system/md5.hpp>

namespace ecode {

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
										std::shared_ptr<FileSystemModel> fileSystemModel ) :
	mSplitter( splitter ), mFileSystemModel( fileSystemModel ) {}

static inline bool endsWithSlash( const std::string& dir ) {
	return !dir.empty() && ( dir.back() == '\\' || dir.back() == '/' );
}

void FileSystemListener::handleFileAction( efsw::WatchID, const std::string& dir,
										   const std::string& filename, efsw::Action action,
										   std::string oldFilename ) {
	FileInfo file( ( endsWithSlash( dir ) ? dir : ( dir + FileSystem::getOSSlash() ) ) + filename );

	switch ( action ) {
		case efsw::Actions::Add:
		case efsw::Actions::Delete:
		case efsw::Actions::Moved: {
			FileEvent event( (FileSystemEventType)action, dir, filename, oldFilename );

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
				mFileSystemModel.get()->handleFileEvent( event );

			if ( mDirTree )
				mDirTree.get()->onChange( (ProjectDirectoryTree::Action)action, file, oldFilename );

			if ( action == efsw::Actions::Moved ) {
				FileInfo oldFile( FileSystem::isRelativePath( oldFilename ) ? dir + oldFilename
																			: oldFilename );
				if ( file.isLink() )
					file = FileInfo( file.linksTo() );

				if ( isFileOpen( oldFile ) )
					notifyMove( oldFile, file );

				if ( oldFile.isLink() ) {
					oldFile = FileInfo( oldFile.linksTo() );

					if ( isFileOpen( oldFile ) )
						notifyMove( oldFile, file );
				}
			}

			if ( file.isLink() )
				file = FileInfo( file.linksTo() );

			if ( isFileOpen( file ) )
				notifyChange( file );

			Lock l( mCbsMutex );
			if ( !mCbs.empty() ) {
				auto cbs = mCbs;
				for ( const auto& cb : cbs )
					cb.second( event, file );
			}

			break;
		}
		case efsw::Actions::Modified: {
			if ( file.isLink() )
				file = FileInfo( file.linksTo() );
			if ( isFileOpen( file ) )
				notifyChange( file );

			Lock l( mCbsMutex );
			if ( !mCbs.empty() ) {
				auto cbs = mCbs;
				FileEvent event( (FileSystemEventType)action, dir, filename, oldFilename );
				for ( const auto& cb : cbs )
					cb.second( event, file );
			}
		}
	}
}

void FileSystemListener::setDirTree( const std::shared_ptr<ProjectDirectoryTree>& dirTree ) {
	mDirTree = dirTree;
}

Uint64 FileSystemListener::addListener( const FileEventFn& fn ) {
	Lock l( mCbsMutex );
	Uint64 id = ++mLastId;
	mCbs[id] = fn;
	return id;
}

bool FileSystemListener::removeListener( const Uint64& id ) {
	Lock l( mCbsMutex );
	auto it = mCbs.find( id );
	if ( it != mCbs.end() ) {
		mCbs.erase( it );
		return true;
	}
	return false;
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
		if ( file.getFilepath() == doc.getFileInfo().getFilepath() &&
			 file.getModificationTime() != doc.getFileInfo().getModificationTime() &&
			 !doc.isSaving() ) {
			MD5::Digest curHash = MD5::fromFile( file.getFilepath() ).digest;
			if ( curHash != doc.getHash() )
				doc.setDirtyOnFileSystem( true );
		}
	} );
}

void FileSystemListener::notifyMove( const FileInfo& oldFile, const FileInfo& newFile ) {
	mSplitter->forEachDoc( [&]( TextDocument& doc ) {
		if ( oldFile.getFilepath() == doc.getFileInfo().getFilepath() )
			doc.notifyDocumentMoved( newFile.getFilepath() );
	} );
}

} // namespace ecode
