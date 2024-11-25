#include "projectdirectorytree.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <limits>

namespace ecode {

#define PRJ_ALLOWED_PATH ".ecode/.prjallowed"

ProjectDirectoryTree::ProjectDirectoryTree(
	const std::string& path, std::shared_ptr<ThreadPool> threadPool, PluginManager* pluginManager,
	std::function<void( const std::string& )> loadFileFromPathOrFocusFn ) :
	mPath( path ),
	mPool( threadPool ),
	mRunning( false ),
	mIsReady( false ),
	mIgnoreHidden( true ),
	mClosing( false ),
	mIgnoreMatcher( path ),
	mPluginManager( pluginManager ),
	mLoadFileFromPathOrFocusFn( std::move( loadFileFromPathOrFocusFn ) ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

ProjectDirectoryTree::~ProjectDirectoryTree() {
	mClosing = true;
	if ( mPluginManager )
		mPluginManager->unsubscribeMessages( "ProjectDirectoryTree" );
	Lock rl( mMatchingMutex );
	if ( mRunning ) {
		mRunning = false;
		Lock l( mFilesMutex );
	}
	{
		Lock l( mDoneMutex );
	}
}

void ProjectDirectoryTree::scan( const ProjectDirectoryTree::ScanCompleteEvent& scanComplete,
								 const std::vector<std::string>& acceptedPatterns,
								 const bool& ignoreHidden ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
	mPool->run(
		[this, acceptedPatterns = std::move( acceptedPatterns ), ignoreHidden] {
#endif
			Lock l( mFilesMutex );
			mRunning = true;
			mIgnoreHidden = ignoreHidden;
			mDirectories.push_back( mPath );

			if ( !mAllowedMatcher && FileSystem::fileExists( mPath + PRJ_ALLOWED_PATH ) )
				mAllowedMatcher =
					std::make_unique<GitIgnoreMatcher>( mPath, PRJ_ALLOWED_PATH, false );

			if ( !acceptedPatterns.empty() ) {
				std::vector<std::string> files;
				std::vector<std::string> names;
				mAcceptedPatterns.clear();
				mAcceptedPatterns.reserve( acceptedPatterns.size() );
				for ( const auto& strPattern : acceptedPatterns )
					mAcceptedPatterns.emplace_back( std::string{ strPattern } );
				std::set<std::string> info;
				getDirectoryFiles( files, names, mPath, info, false, mIgnoreMatcher,
								   mAllowedMatcher.get() );
				size_t namesCount = names.size();
				bool found;
				for ( size_t i = 0; i < namesCount; i++ ) {
					found = false;
					for ( const auto& pattern : mAcceptedPatterns ) {
						if ( pattern.matches( names[i] ) ) {
							found = true;
							break;
						}
					}
					if ( found ) {
						mFiles.emplace_back( std::move( files[i] ) );
						mNames.emplace_back( std::move( names[i] ) );
					}
				}
			} else {
				std::set<std::string> info;
				getDirectoryFiles( mFiles, mNames, mPath, info, ignoreHidden, mIgnoreMatcher,
								   mAllowedMatcher.get() );
			}
			mIsReady = true;
			if ( mPluginManager ) {
				mPluginManager->subscribeMessages(
					"ProjectDirectoryTree",
					[this]( const PluginMessage& msg ) -> PluginRequestHandle {
						return processMessage( msg );
					} );
			}
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN && !defined( __EMSCRIPTEN_PTHREADS__ )
			if ( scanComplete )
				scanComplete( *this );
#endif
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		},
		[scanComplete, this]( const auto& ) {
			if ( !mClosing && scanComplete ) {
				Lock l( mDoneMutex );
				scanComplete( *this );
			}
			mRunning = false;
		} );
#endif
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::fuzzyMatchTree( const std::vector<std::string>& matches, const size_t& max,
									  const std::string& basePath ) const {
	Lock rl( mMatchingMutex );
	std::multimap<int, int, std::greater<int>> matchesMap;
	std::vector<std::string> files;
	std::vector<std::string> names;
	for ( const auto& match : matches ) {
		for ( size_t i = 0; i < mNames.size(); i++ ) {
			int matchName = String::fuzzyMatch( mNames[i], match );
			int matchPath = String::fuzzyMatch( mFiles[i], match );
			matchesMap.insert( { std::max( matchName, matchPath ), i } );
		}
	}
	for ( auto& res : matchesMap ) {
		if ( names.size() < max ) {
			names.emplace_back( mNames[res.second] );
			files.emplace_back( mFiles[res.second] );
		} else {
			break;
		}
	}
	auto model = std::make_shared<FileListModel>( std::move( files ), std::move( names ) );
	model->setBasePath( basePath );
	return model;
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::fuzzyMatchTree( const std::string& match, const size_t& max,
									  const std::string& basePath ) const {
	Lock rl( mMatchingMutex );
	std::multimap<int, int, std::greater<int>> matchesMap;
	std::vector<std::string> files;
	std::vector<std::string> names;
	for ( size_t i = 0; i < mNames.size(); i++ ) {
		int matchName = String::fuzzyMatch( mNames[i], match );
		int matchPath = String::fuzzyMatch( mFiles[i], match );
		matchesMap.insert( { std::max( matchName, matchPath ), i } );
	}
	for ( auto& res : matchesMap ) {
		if ( names.size() < max ) {
			names.emplace_back( mNames[res.second] );
			files.emplace_back( mFiles[res.second] );
		} else {
			break;
		}
	}
	auto model = std::make_shared<FileListModel>( std::move( files ), std::move( names ) );
	model->setBasePath( basePath );
	return model;
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::matchTree( const std::string& match, const size_t& max,
								 const std::string& basePath ) const {
	Lock rl( mMatchingMutex );
	std::vector<std::string> files;
	std::vector<std::string> names;
	std::string lowerMatch( String::toLower( match ) );
	for ( size_t i = 0; i < mNames.size(); i++ ) {
		if ( String::toLower( mNames[i] ).find( lowerMatch ) != std::string::npos ) {
			names.emplace_back( mNames[i] );
			files.emplace_back( mFiles[i] );
			if ( max == names.size() )
				break;
		}
	}
	auto model = std::make_shared<FileListModel>( std::move( files ), std::move( names ) );
	model->setBasePath( basePath );
	return model;
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::globMatchTree( const std::string& match, const size_t& max,
									 const std::string& basePath ) const {
	Lock rl( mMatchingMutex );
	std::vector<std::string> files;
	std::vector<std::string> names;
	for ( size_t i = 0; i < mNames.size(); i++ ) {
		std::string_view file( mFiles[i] );
		if ( !match.empty() && !basePath.empty() && file.size() >= basePath.size() &&
			 String::startsWith( file, std::string_view{ basePath } ) ) {
			file = file.substr( basePath.size() );
		}

		if ( match.empty() || String::globMatch( file, match ) ) {
			names.emplace_back( mNames[i] );
			files.emplace_back( mFiles[i] );
			if ( max == names.size() )
				break;
		}
	}
	auto model = std::make_shared<FileListModel>( std::move( files ), std::move( names ) );
	model->setBasePath( basePath );
	return model;
}

void ProjectDirectoryTree::asyncMatchTree( MatchType type, const std::string& match,
										   const size_t& max, MatchResultCb res,
										   const std::string& basePath ) const {
	mPool->run( [this, match, max, res, basePath, type]() {
		std::shared_ptr<FileListModel> result;
		switch ( type ) {
			case MatchType::Substring:
				result = matchTree( match, max, basePath );
				break;
			case MatchType::Fuzzy:
				result = fuzzyMatchTree( match, max, basePath );
				break;
			case MatchType::Glob:
				result = globMatchTree( match, max, basePath );
				break;
		}
		res( result );
	} );
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::asModel( const size_t& max, const std::vector<CommandInfo>& prependCommands,
							   const std::string& basePath,
							   const std::vector<std::string>& skipExtensions ) const {
	size_t namesSize = mNames.size();
	size_t rmax = eemin( namesSize, max );
	std::vector<std::string> files;
	std::vector<std::string> names;
	files.reserve( rmax + prependCommands.size() );
	names.reserve( rmax + prependCommands.size() );
	for ( size_t i = 0; i < namesSize; i++ ) {
		if ( skipExtensions.empty() ||
			 std::find( skipExtensions.begin(), skipExtensions.end(),
						FileSystem::fileExtension( mFiles[i] ) ) == skipExtensions.end() ) {
			files.emplace_back( mFiles[i] );
			names.emplace_back( mNames[i] );
			if ( files.size() >= rmax )
				break;
		}
	}
	if ( !prependCommands.empty() ) {
		int count = 0;
		for ( const auto& cmd : prependCommands ) {
			names.insert( names.begin() + count, cmd.name );
			files.insert( files.begin() + count, cmd.desc );
			count++;
		}
	}
	auto model = std::make_shared<FileListModel>( std::move( files ), std::move( names ) );
	model->setBasePath( basePath );

	if ( !prependCommands.empty() ) {
		for ( size_t i = 0; i < prependCommands.size(); ++i )
			model->setIcon( i, prependCommands[i].icon );
	}

	return model;
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::emptyModel( const std::vector<CommandInfo>& prependCommands,
								  const std::string& basePath ) {
	std::vector<std::string> files;
	std::vector<std::string> names;
	if ( !prependCommands.empty() ) {
		files.reserve( prependCommands.size() );
		names.reserve( prependCommands.size() );
		int count = 0;
		for ( const auto& cmd : prependCommands ) {
			names.insert( names.begin() + count, cmd.name );
			files.insert( files.begin() + count, cmd.desc );
			count++;
		}
	}
	auto model = std::make_shared<FileListModel>( std::move( files ), std::move( names ) );
	model->setBasePath( basePath );

	if ( !prependCommands.empty() ) {
		for ( size_t i = 0; i < prependCommands.size(); ++i )
			model->setIcon( i, prependCommands[i].icon );
	}

	return model;
}

size_t ProjectDirectoryTree::getFilesCount() const {
	Lock l( mFilesMutex );
	return mFiles.size();
}

const std::vector<std::string>& ProjectDirectoryTree::getFiles() const {
	Lock l( mFilesMutex );
	return mFiles;
}

const std::vector<std::string>& ProjectDirectoryTree::getDirectories() const {
	return mDirectories;
}

bool ProjectDirectoryTree::isFileInTree( const std::string& filePath ) const {
	return std::find( mFiles.begin(), mFiles.end(), filePath ) != mFiles.end();
}

bool ProjectDirectoryTree::isDirInTree( const std::string& dirTree ) const {
	std::string dir( FileSystem::fileRemoveFileName( dirTree ) );
	FileSystem::dirAddSlashAtEnd( dir );
	return std::find( mDirectories.begin(), mDirectories.end(), dir ) != mDirectories.end();
}

void ProjectDirectoryTree::getDirectoryFiles(
	std::vector<std::string>& files, std::vector<std::string>& names, std::string directory,
	std::set<std::string> currentDirs, const bool& ignoreHidden,
	IgnoreMatcherManager& ignoreMatcher, GitIgnoreMatcher* allowedMatcher ) {
	if ( !mRunning )
		return;
	currentDirs.insert( directory );
	std::vector<std::string> pathFiles =
		FileSystem::filesGetInPath( directory, false, false, false );
	for ( auto& file : pathFiles ) {
		std::string fullpath( directory + file );
		if ( ignoreMatcher.foundMatch() && ignoreMatcher.match( directory, file ) ) {
			if ( !allowedMatcher )
				continue;
			std::string localPath;
			if ( String::startsWith( directory, allowedMatcher->getPath() ) )
				localPath = directory.substr( allowedMatcher->getPath().size() );
			if ( !allowedMatcher->match( localPath + file ) )
				continue;
		}
		if ( FileSystem::isDirectory( fullpath ) ) {
			fullpath += FileSystem::getOSSlash();
			FileInfo dirInfo( fullpath, true );
			if ( dirInfo.isLink() ) {
				fullpath = dirInfo.linksTo();
				FileSystem::dirAddSlashAtEnd( fullpath );
				if ( currentDirs.find( fullpath ) == currentDirs.end() )
					continue;
				if ( std::find( mDirectories.begin(), mDirectories.end(), fullpath ) !=
					 mDirectories.end() )
					continue;
				mDirectories.push_back( fullpath );
			} else {
				mDirectories.push_back( fullpath );
			}
			IgnoreMatcherManager dirMatcher( fullpath );
			IgnoreMatcher* childMatch = nullptr;
			if ( dirMatcher.foundMatch() ) {
				childMatch = dirMatcher.popMatcher( 0 );
				ignoreMatcher.addChild( childMatch );
			}
			getDirectoryFiles( files, names, fullpath, currentDirs, ignoreHidden, ignoreMatcher,
							   allowedMatcher );
			if ( childMatch ) {
				ignoreMatcher.removeChild( childMatch );
				eeSAFE_DELETE( childMatch );
			}
		} else {
			files.emplace_back( fullpath );
			names.emplace_back( file );
		}
	}
}

void ProjectDirectoryTree::onChange( const ProjectDirectoryTree::Action& action,
									 const FileInfo& file, const std::string& oldFilename ) {
	if ( !file.isDirectory() && !isDirInTree( file.getFilepath() ) )
		return;
	switch ( action ) {
		case ProjectDirectoryTree::Action::Add:
			addFile( file );
			break;
		case ProjectDirectoryTree::Action::Delete:
			removeFile( file );
			break;
		case ProjectDirectoryTree::Action::Moved:
			moveFile( file, oldFilename );
			break;
		case ProjectDirectoryTree::Action::Modified:
			break;
	}
}

void ProjectDirectoryTree::resetPluginManager() {
	mPluginManager = nullptr;
}

void ProjectDirectoryTree::tryAddFile( const FileInfo& file ) {
	if ( mIgnoreHidden && file.isHidden() )
		return;
	IgnoreMatcherManager matcher( getIgnoreMatcherFromPath( file.getFilepath() ) );
	if ( !matcher.foundMatch() || !matcher.match( file ) ) {
		bool foundPattern = mAcceptedPatterns.empty();
		for ( auto& pattern : mAcceptedPatterns ) {
			if ( pattern.matches( file.getFilepath() ) ) {
				foundPattern = true;
				break;
			}
		}
		if ( foundPattern ) {
			Lock l( mFilesMutex );
			auto exists =
				std::find( mFiles.begin(), mFiles.end(), file.getFilepath() ) != mFiles.end();
			if ( !exists ) {
				mFiles.emplace_back( file.getFilepath() );
				mNames.emplace_back( file.getFileName() );
			}
		}
	}
}

void ProjectDirectoryTree::addFile( const FileInfo& file ) {
	if ( file.isDirectory() ) {
		if ( !String::startsWith( file.getFilepath(), mPath ) || isDirInTree( file.getFilepath() ) )
			return;
		if ( mIgnoreHidden && file.isHidden() )
			return;
		Lock l( mFilesMutex );
		std::vector<std::string> files;
		std::vector<std::string> names;
		std::vector<LuaPattern> patterns;
		std::set<std::string> info;
		if ( !mAcceptedPatterns.empty() ) {
			getDirectoryFiles( files, names, mPath, info, false, mIgnoreMatcher,
							   mAllowedMatcher.get() );
			size_t namesCount = names.size();
			bool found;
			for ( size_t i = 0; i < namesCount; i++ ) {
				found = false;
				for ( auto& pattern : patterns ) {
					if ( pattern.matches( names[i] ) ) {
						found = true;
						break;
					}
				}
				if ( found ) {
					mFiles.emplace_back( std::move( files[i] ) );
					mNames.emplace_back( std::move( names[i] ) );
				}
			}
		} else {
			getDirectoryFiles( mFiles, mNames, mPath, info, false, mIgnoreMatcher,
							   mAllowedMatcher.get() );
		}
	} else {
		tryAddFile( file );
	}
}

void ProjectDirectoryTree::moveFile( const FileInfo& file, const std::string& oldFilename ) {
	Lock l( mFilesMutex );
	if ( file.isDirectory() ) {
		std::string dir( file.getDirectoryPath() );
		FileSystem::dirRemoveSlashAtEnd( dir );
		std::string parentDir( FileSystem::fileRemoveFileName( dir ) );
		FileSystem::dirAddSlashAtEnd( parentDir );
		std::string oldDir( parentDir + oldFilename );
		FileSystem::dirAddSlashAtEnd( dir );
		FileSystem::dirAddSlashAtEnd( oldDir );
		std::vector<std::string> files;
		std::vector<std::string> names;
		for ( size_t i = 0; i < mFiles.size(); i++ ) {
			if ( !String::startsWith( mFiles[i], oldDir ) ) {
				files.emplace_back( mFiles[i] );
				names.emplace_back( mNames[i] );
			} else {
				std::string newDir( dir + mFiles[i].substr( oldDir.size() ) );
				files.emplace_back( newDir );
				names.emplace_back( FileSystem::fileNameFromPath( newDir ) );
			}
		}
		mFiles = files;
		mNames = names;
		auto wasDirIt = std::find( mDirectories.begin(), mDirectories.end(), oldDir );
		if ( wasDirIt != mDirectories.end() )
			mDirectories.erase( wasDirIt );
		mDirectories.emplace_back( std::move( dir ) );
	} else {
		std::string dir( file.getDirectoryPath() );
		FileSystem::dirAddSlashAtEnd( dir );
		size_t index = findFileIndex( dir + oldFilename );
		if ( index != std::string::npos ) {
			IgnoreMatcherManager matcher( getIgnoreMatcherFromPath( file.getFilepath() ) );
			if ( !( mIgnoreHidden && file.isHidden() ) &&
				 ( !matcher.foundMatch() || !matcher.match( file ) ) ) {
				mFiles[index] = file.getFilepath();
				mNames[index] = file.getFileName();
			} else {
				mFiles.erase( mFiles.begin() + index );
				mNames.erase( mNames.begin() + index );
			}
		} else {
			tryAddFile( file );
		}
	}
}

void ProjectDirectoryTree::removeFile( const FileInfo& file ) {
	Lock l( mFilesMutex );
	std::string removedDir( file.getFilepath() );
	FileSystem::dirAddSlashAtEnd( removedDir );
	auto wasDirIt = std::find( mDirectories.begin(), mDirectories.end(), removedDir );
	if ( wasDirIt != mDirectories.end() ) {
		std::vector<std::string> files;
		std::vector<std::string> names;
		for ( size_t i = 0; i < mFiles.size(); i++ ) {
			if ( !String::startsWith( mFiles[i], removedDir ) ) {
				files.emplace_back( mFiles[i] );
				names.emplace_back( mNames[i] );
			}
		}
		mFiles = files;
		mNames = names;
		mDirectories.erase( wasDirIt );
	} else {
		size_t index = findFileIndex( file.getFilepath() );
		if ( index != std::string::npos ) {
			mFiles.erase( mFiles.begin() + index );
			mNames.erase( mNames.begin() + index );
		}
	}
}

IgnoreMatcherManager ProjectDirectoryTree::getIgnoreMatcherFromPath( const std::string& path ) {
	std::string dir( FileSystem::fileRemoveFileName( path ) );
	FileSystem::dirAddSlashAtEnd( dir );
	IgnoreMatcherManager curMatcher( dir );
	IgnoreMatcherManager matcher( curMatcher.findRepositoryRootPath() );
	if ( !matcher.foundMatch() ) {
		if ( curMatcher.foundMatch() )
			return curMatcher;
		return matcher;
	}
	std::string prevDir( dir );
	if ( prevDir != matcher.getRootPath() && prevDir.size() > matcher.getRootPath().size() ) {
		std::string tmpdir( FileSystem::removeLastFolderFromPath( dir ) );
		std::string ltmpdir;
		while ( ltmpdir != tmpdir && tmpdir != matcher.getRootPath() ) {
			IgnoreMatcherManager tmpMatcher( tmpdir );
			if ( tmpMatcher.foundMatch() )
				matcher.addChild( tmpMatcher.popMatcher( 0 ) );
			ltmpdir = tmpdir;
			tmpdir = FileSystem::removeLastFolderFromPath( tmpdir );
		}
	}
	return matcher;
}

size_t ProjectDirectoryTree::findFileIndex( const std::string& path ) {
	for ( size_t i = 0; i < mFiles.size(); i++ ) {
		if ( mFiles[i] == path )
			return i;
	}
	return std::string::npos;
}

PluginRequestHandle ProjectDirectoryTree::processMessage( const PluginMessage& msg ) {
	if ( msg.type != PluginMessageType::FindAndOpenClosestURI || !msg.isRequest() || !msg.isJSON() )
		return {};

	const nlohmann::json& json = msg.asJSON();
	if ( !json.contains( "uri" ) )
		return {};

	nlohmann::json juris = json.at( "uri" );
	std::vector<std::string> expectedNames;
	std::vector<std::string> tentativePaths;
	for ( const auto& turi : juris ) {
		std::string path( URI( turi.get<std::string>() ).getFSPath() );
		tentativePaths.emplace_back( path );
		expectedNames.emplace_back( FileSystem::fileNameFromPath( path ) );
	}

	auto model = fuzzyMatchTree( expectedNames, 10 );

	size_t rowCount = model->rowCount( {} );
	if ( rowCount == 0 )
		return {};

	std::map<int, std::string, std::greater<int>> matchesMap;

	for ( size_t i = 0; i < rowCount; ++i ) {
		Variant dataName = model->data( model->index( i, 0 ) );
		Variant dataPath = model->data( model->index( i, 1 ) );
		if ( dataName.isString() && dataPath.isString() ) {
			std::string fileName( dataName.toString() );
			std::string filePath( dataPath.toString() );
			if ( std::find( expectedNames.begin(), expectedNames.end(), fileName ) !=
				 expectedNames.end() ) {
				std::string closestDataPath;
				int max{ std::numeric_limits<int>::min() };
				for ( const auto& paths : tentativePaths ) {
					int res = String::fuzzyMatch( filePath.c_str(), paths.c_str(), true );
					if ( res > max ) {
						closestDataPath = filePath;
						max = res;
					}
				}
				if ( !closestDataPath.empty() )
					matchesMap[max] = closestDataPath;
			}
		}
	}

	if ( !matchesMap.empty() ) {
		if ( mPluginManager && mLoadFileFromPathOrFocusFn ) {
			std::string filePath( matchesMap.begin()->second );
			mPluginManager->getUISceneNode()->runOnMainThread(
				[this, filePath]() { mLoadFileFromPathOrFocusFn( filePath ); } );
		}
	}

	return PluginRequestHandle::broadcast();
}

} // namespace ecode
