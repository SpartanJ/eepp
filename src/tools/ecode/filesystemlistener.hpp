#ifndef ECODE_FILESYSTEMLISTENER_HPP
#define ECODE_FILESYSTEMLISTENER_HPP

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

	FileSystemListener( UICodeEditorSplitter* codeSplitter,
						std::shared_ptr<FileSystemModel> fileSystemModel,
						const std::vector<std::string>& ignoreFiles );

	virtual ~FileSystemListener() {}

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, std::string oldFilename );

	void setFileSystemModel( std::shared_ptr<FileSystemModel> model ) { mFileSystemModel = model; }

	void setDirTree( const std::shared_ptr<ProjectDirectoryTree>& dirTree );

	Uint64 addListener( const FileEventFn& fn );

	bool removeListener( const Uint64& id );

  protected:
	UICodeEditorSplitter* mSplitter;
	std::shared_ptr<FileSystemModel> mFileSystemModel;
	std::shared_ptr<ProjectDirectoryTree> mDirTree;
	std::atomic<Uint64> mLastId{ 0 };
	std::unordered_map<Uint64, FileEventFn> mCbs;
	std::vector<std::string> mIgnoredFiles;
	Mutex mCbsMutex;

	bool isFileOpen( const FileInfo& file );

	void notifyChange( const FileInfo& file );

	void notifyMove( const FileInfo& oldFile, const FileInfo& newFile );

	bool isIgnored( const std::string& path );
};

} // namespace ecode

#endif // ECODE_FILESYSTEMLISTENER_HPP
