#include "filesystemlistener.hpp"

FileSystemListener::FileSystemListener( UICodeEditorSplitter* splitter,
										std::shared_ptr<FileSystemModel> fileSystemModel ) :
	mSplitter( splitter ), mFileSystemModel( fileSystemModel ) {}

void FileSystemListener::handleFileAction( efsw::WatchID, const std::string& dir,
										   const std::string& filename, efsw::Action action,
										   std::string oldFilename ) {
	FileInfo file( dir + filename );

	switch ( action ) {
		case efsw::Actions::Add:
		case efsw::Actions::Delete:
		case efsw::Actions::Moved: {
			auto* node =
				mFileSystemModel.get()->getNodeFromPath( file.getDirectoryPath(), true, false );
			if ( node && ( !mFileSystemModel.get()->getDisplayConfig().ignoreHidden ||
						   !file.isHidden() ) ) {
				node->invalidate();
				mFileSystemModel.get()->invalidate();
			}

			if ( mDirTree )
				mDirTree.get()->onChange( (ProjectDirectoryTree::Action)action, file, oldFilename );
		}
		case efsw::Actions::Modified: {
			if ( file.isLink() )
				file = FileInfo( file.linksTo() );
			if ( isFileOpen( file ) )
				notifyChange( file );
		}
	}
}

void FileSystemListener::setDirTree( const std::shared_ptr<ProjectDirectoryTree>& dirTree ) {
	mDirTree = dirTree;
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
			 !doc.isSaving() )
			doc.setDirtyOnFileSystem( true );
	} );
}
