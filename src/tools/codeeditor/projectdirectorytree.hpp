#ifndef EE_TOOLS_PROJECTDIRECTORYTREE_HPP
#define EE_TOOLS_PROJECTDIRECTORYTREE_HPP

#include <eepp/system/luapattern.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/models/model.hpp>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

using namespace EE;
using namespace EE::System;
using namespace EE::UI::Models;

class FileListModel : public Model {
  public:
	FileListModel( const std::vector<std::string>& files, const std::vector<std::string>& names ) :
		mFiles( files ), mNames( names ) {}

	virtual size_t rowCount( const ModelIndex& ) const { return mNames.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 2; }

	virtual std::string columnName( const size_t& index ) const {
		return index == 0 ? "Name" : "Path";
	}

	virtual Variant data( const ModelIndex& index, Role role = Role::Display ) const {
		if ( role == Role::Display ) {
			return Variant( index.column() == 0 ? mNames[index.row()].c_str()
												: mFiles[index.row()].c_str() );
		}
		return {};
	}

	virtual void update() { onModelUpdate(); }

  protected:
	std::vector<std::string> mFiles;
	std::vector<std::string> mNames;
};

class ProjectDirectoryTree {
  public:
	typedef std::function<void( ProjectDirectoryTree& dirTree )> ScanCompleteEvent;

	ProjectDirectoryTree( const std::string& path, std::shared_ptr<ThreadPool> threadPool );

	void scan( const ScanCompleteEvent& scanComplete,
			   const std::vector<std::string>& acceptedPattern = {},
			   const bool& ignoreHidden = true );

	std::shared_ptr<FileListModel> fuzzyMatchTree( const std::string& match,
												   const size_t& max ) const;

	std::shared_ptr<FileListModel> matchTree( const std::string& match, const size_t& max ) const;

	std::shared_ptr<FileListModel> asModel( const size_t& max ) const;

	size_t getFilesCount() const;

  protected:
	std::string mPath;
	std::shared_ptr<ThreadPool> mPool;
	std::vector<std::string> mFiles;
	std::vector<std::string> mNames;
	bool mIsReady;
	Mutex mFilesMutex;

	void getDirectoryFiles( std::vector<std::string>& files, std::vector<std::string>& names,
							std::string directory, std::set<std::string> currentDirs,
							const bool& ignoreHidden );
};

#endif // EE_TOOLS_PROJECTDIRECTORYTREE_HPP
