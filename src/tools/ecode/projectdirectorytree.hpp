#ifndef ECODE_PROJECTDIRECTORYTREE_HPP
#define ECODE_PROJECTDIRECTORYTREE_HPP

#include "ignorematcher.hpp"
#include "plugins/pluginmanager.hpp"
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <functional>
#include <memory>
#include <set>
#include <string>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace ecode {

class FileListModel : public Model {
  public:
	FileListModel( const std::vector<std::string>& files, const std::vector<std::string>& names ) :
		mFiles( files ), mNames( names ), mIcons( mNames.size(), nullptr ) {}

	virtual size_t rowCount( const ModelIndex& ) const { return mNames.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 2; }

	virtual std::string columnName( const size_t& index ) const {
		return index == 0 ? "Name" : "Path";
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( index.row() >= (Int64)mFiles.size() )
			return {};

		switch ( role ) {
			case ModelRole::Icon:
				return Variant( iconFor( index ) );
			case ModelRole::Display: {
				if ( !mBasePath.empty() && index.column() == 1 &&
					 mBasePath.size() < mFiles[index.row()].size() &&
					 String::startsWith( mFiles[index.row()], mBasePath ) )
					return Variant( mFiles[index.row()].substr( mBasePath.size() ) );
				return Variant( index.column() == 0 ? mNames[index.row()].c_str()
													: mFiles[index.row()].c_str() );
			}
			case ModelRole::Custom:
				return Variant( index.column() == 0 ? mNames[index.row()].c_str()
													: mFiles[index.row()].c_str() );
			default:
				break;
		}

		return {};
	}

	void setIcon( size_t idx, UIIcon* icon ) {
		eeASSERT( idx < mIcons.size() );
		mIcons[idx] = icon;
	}

	void setBasePath( const std::string& basePath ) { mBasePath = basePath; }

  protected:
	std::vector<std::string> mFiles;
	std::vector<std::string> mNames;
	std::string mBasePath;
	mutable std::vector<UIIcon*> mIcons;

	UIIcon* iconFor( const ModelIndex& index ) const {
		if ( index.column() == 0 ) {
			if ( mIcons[index.row()] )
				return mIcons[index.row()];
			auto* scene = SceneManager::instance()->getUISceneNode();
			auto* d = scene->findIcon(
				UIIconThemeManager::getIconNameFromFileName( mNames[index.row()], true ) );
			if ( !d )
				d = scene->findIcon( "file" );
			mIcons[index.row()] = d;
			return d;
		}
		return nullptr;
	}
};

class ProjectDirectoryTree {
  public:
	enum Action {
		/// Sent when a file is created or renamed
		Add = 1,
		/// Sent when a file is deleted or renamed
		Delete = 2,
		/// Sent when a file is modified
		Modified = 3,
		/// Sent when a file is moved
		Moved = 4
	};

	typedef std::function<void( ProjectDirectoryTree& dirTree )> ScanCompleteEvent;
	typedef std::function<void( std::shared_ptr<FileListModel> )> MatchResultCb;

	ProjectDirectoryTree(
		const std::string& path, std::shared_ptr<ThreadPool> threadPool,
		PluginManager* pluginManager = nullptr,
		std::function<void( const std::string& )> loadFileFromPathOrFocusFn = {} );

	~ProjectDirectoryTree();

	void scan( const ScanCompleteEvent& scanComplete,
			   const std::vector<std::string>& acceptedPatterns = {},
			   const bool& ignoreHidden = true );

	std::shared_ptr<FileListModel> fuzzyMatchTree( const std::vector<std::string>& matches,
												   const size_t& max,
												   const std::string& basePath = "" ) const;

	std::shared_ptr<FileListModel> fuzzyMatchTree( const std::string& match, const size_t& max,
												   const std::string& basePath = "" ) const;

	std::shared_ptr<FileListModel> matchTree( const std::string& match, const size_t& max,
											  const std::string& basePath = "" ) const;

	void asyncFuzzyMatchTree( const std::string& match, const size_t& max, MatchResultCb res,
							  const std::string& basePath = "" ) const;

	void asyncMatchTree( const std::string& match, const size_t& max, MatchResultCb res,
						 const std::string& basePath = "" ) const;

	struct CommandInfo {
		std::string name;
		std::string desc;
		UIIcon* icon{ nullptr };
	};

	std::shared_ptr<FileListModel> asModel( const size_t& max,
											const std::vector<CommandInfo>& prependCommands = {},
											const std::string& basePath = "" ) const;

	static std::shared_ptr<FileListModel>
	emptyModel( const std::vector<CommandInfo>& prependCommands = {},
				const std::string& basePath = "" );

	size_t getFilesCount() const;

	const std::vector<std::string>& getFiles() const;

	const std::vector<std::string>& getDirectories() const;

	bool isFileInTree( const std::string& filePath ) const;

	bool isDirInTree( const std::string& dirTree ) const;

	void onChange( const Action& action, const FileInfo& file, const std::string& oldFilename );

	const std::string& getPath() const { return mPath; }

	void resetPluginManager();

  protected:
	std::string mPath;
	std::shared_ptr<ThreadPool> mPool;
	std::vector<std::string> mFiles;
	std::vector<std::string> mNames;
	std::vector<std::string> mDirectories;
	std::vector<LuaPatternStorage> mAcceptedPatterns;
	std::unique_ptr<GitIgnoreMatcher> mAllowedMatcher;
	bool mRunning;
	bool mIsReady;
	bool mIgnoreHidden;
	bool mClosing;
	mutable Mutex mFilesMutex;
	mutable Mutex mMatchingMutex;
	Mutex mDoneMutex;
	IgnoreMatcherManager mIgnoreMatcher;
	PluginManager* mPluginManager{ nullptr };
	std::function<void( const std::string& )> mLoadFileFromPathOrFocusFn;

	void getDirectoryFiles( std::vector<std::string>& files, std::vector<std::string>& names,
							std::string directory, std::set<std::string> currentDirs,
							const bool& ignoreHidden, IgnoreMatcherManager& ignoreMatcher,
							GitIgnoreMatcher* allowedMatcher );

	void addFile( const FileInfo& file );

	void tryAddFile( const FileInfo& file );

	void moveFile( const FileInfo& file, const std::string& oldFilename );

	void removeFile( const FileInfo& file );

	IgnoreMatcherManager getIgnoreMatcherFromPath( const std::string& path );

	size_t findFileIndex( const std::string& path );

	PluginRequestHandle processMessage( const PluginMessage& msg );
};

} // namespace ecode

#endif // ECODE_PROJECTDIRECTORYTREE_HPP
