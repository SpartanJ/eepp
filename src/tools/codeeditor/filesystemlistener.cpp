#include "filesystemlistener.hpp"

FileSystemListener::FileSystemListener( UICodeEditorSplitter* splitter ) : mSplitter( splitter ) {}

void FileSystemListener::handleFileAction( efsw::WatchID, const std::string& dir,
										   const std::string& filename, efsw::Action action,
										   std::string ) {
	if ( action == efsw::Actions::Modified ) {
		FileInfo file( dir + filename );
		if ( file.isLink() )
			file = FileInfo( file.linksTo() );
		if ( isFileOpen( file ) )
			notifyChange( file );
	}
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
