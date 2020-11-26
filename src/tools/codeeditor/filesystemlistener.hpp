#ifndef FILESYSTEMLISTENER_HPP
#define FILESYSTEMLISTENER_HPP

#include <eepp/system/fileinfo.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <efsw/efsw.hpp>

using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;

class FileSystemListener : public efsw::FileWatchListener {
  public:
	FileSystemListener( UICodeEditorSplitter* codeSplitter );

	virtual ~FileSystemListener() {}

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, std::string );

  protected:
	UICodeEditorSplitter* mSplitter;

	bool isFileOpen( const FileInfo& file );

	void notifyChange( const FileInfo& file );
};

#endif // FILESYSTEMLISTENER_HPP
