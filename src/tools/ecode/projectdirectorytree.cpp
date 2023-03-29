#include "projectdirectorytree.hpp"
#include "ecode.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <limits>

namespace ecode {

#define PRJ_ALLOWED_PATH ".ecode/.prjallowed"

ProjectDirectoryTree::ProjectDirectoryTree( const std::string& path,
											std::shared_ptr<ThreadPool> threadPool, App* app ) :
	mPath( path ),
	mPool( threadPool ),
	mRunning( false ),
	mIsReady( false ),
	mIgnoreHidden( true ),
	mIgnoreMatcher( path ),
	mApp( app ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

ProjectDirectoryTree::~ProjectDirectoryTree() {
	if ( mApp->getPluginManager() )
		mApp->getPluginManager()->unsubscribeMessages( "ProjectDirectoryTree" );
	Lock rl( mMatchingMutex );
	if ( mRunning ) {
		mRunning = false;
		Lock l( mFilesMutex );
	}
}

void ProjectDirectoryTree::scan( const ProjectDirectoryTree::ScanCompleteEvent& scanComplete,
								 const std::vector<std::string>& acceptedPatterns,
								 const bool& ignoreHidden ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
	mPool->run(
		[&, acceptedPatterns, ignoreHidden] {
#endif
			Lock l( mFilesMutex );
			mRunning = true;
			mIgnoreHidden = ignoreHidden;
			mDirectories.push_back( mPath );

			if ( !mAllowedMatcher && FileSystem::fileExists( mPath + PRJ_ALLOWED_PATH ) )
				mAllowedMatcher = std::make_unique<GitIgnoreMatcher>( mPath, PRJ_ALLOWED_PATH );

			if ( !acceptedPatterns.empty() ) {
				std::vector<std::string> files;
				std::vector<std::string> names;
				std::vector<LuaPattern> patterns;
				for ( auto& strPattern : acceptedPatterns )
					patterns.emplace_back( LuaPattern( strPattern ) );
				mAcceptedPatterns = patterns;
				std::set<std::string> info;
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
				std::set<std::string> info;
				getDirectoryFiles( mFiles, mNames, mPath, info, ignoreHidden, mIgnoreMatcher,
								   mAllowedMatcher.get() );
			}
			mIsReady = true;
			mRunning = false;
			mApp->getPluginManager()->subscribeMessages(
				"ProjectDirectoryTree", [&]( const PluginMessage& msg ) -> PluginRequestHandle {
					return processMessage( msg );
				} );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN && !defined( __EMSCRIPTEN_PTHREADS__ )
			if ( scanComplete )
				scanComplete( *this );
#endif
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		},
		[scanComplete, this]( const auto& ) {
			if ( scanComplete )
				scanComplete( *this );
		} );
#endif
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::fuzzyMatchTree( const std::vector<std::string>& matches,
									  const size_t& max ) const {
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
		}
	}
	return std::make_shared<FileListModel>( files, names );
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::fuzzyMatchTree( const std::string& match,
																	 const size_t& max ) const {
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
		}
	}
	return std::make_shared<FileListModel>( files, names );
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::matchTree( const std::string& match,
																const size_t& max ) const {
	Lock rl( mMatchingMutex );
	std::vector<std::string> files;
	std::vector<std::string> names;
	std::string lowerMatch( String::toLower( match ) );
	for ( size_t i = 0; i < mNames.size(); i++ ) {
		if ( String::toLower( mNames[i] ).find( lowerMatch ) != std::string::npos ) {
			names.emplace_back( mNames[i] );
			files.emplace_back( mFiles[i] );
			if ( max == names.size() )
				return std::make_shared<FileListModel>( files, names );
		}
	}
	return std::make_shared<FileListModel>( files, names );
}

void ProjectDirectoryTree::asyncFuzzyMatchTree( const std::string& match, const size_t& max,
												ProjectDirectoryTree::MatchResultCb res ) const {
	mPool->run( [&, match, max, res]() { res( fuzzyMatchTree( match, max ) ); } );
}

void ProjectDirectoryTree::asyncMatchTree( const std::string& match, const size_t& max,
										   ProjectDirectoryTree::MatchResultCb res ) const {
	mPool->run( [&, match, max, res]() { res( matchTree( match, max ) ); } );
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::asModel( const size_t& max,
							   const std::vector<CommandInfo>& prependCommands ) const {
	if ( mNames.empty() )
		return std::make_shared<FileListModel>( std::vector<std::string>(),
												std::vector<std::string>() );
	size_t rmax = eemin( mNames.size(), max );
	std::vector<std::string> files( rmax );
	std::vector<std::string> names( rmax );
	for ( size_t i = 0; i < rmax; i++ ) {
		files[i] = mFiles[i];
		names[i] = mNames[i];
	}
	if ( !prependCommands.empty() ) {
		int count = 0;
		for ( const auto& cmd : prependCommands ) {
			names.insert( names.begin() + count, cmd.name );
			files.insert( files.begin() + count, cmd.desc );
			count++;
		}
	}
	auto model = std::make_shared<FileListModel>( files, names );

	if ( !prependCommands.empty() ) {
		for ( size_t i = 0; i < prependCommands.size(); ++i )
			model->setIcon( i, prependCommands[i].icon );
	}

	return model;
}

std::shared_ptr<FileListModel>
ProjectDirectoryTree::emptyModel( const std::vector<CommandInfo>& prependCommands ) {
	std::vector<std::string> files;
	std::vector<std::string> names;
	if ( !prependCommands.empty() ) {
		int count = 0;
		for ( const auto& cmd : prependCommands ) {
			names.insert( names.begin() + count, cmd.name );
			files.insert( files.begin() + count, cmd.desc );
			count++;
		}
	}
	auto model = std::make_shared<FileListModel>( files, names );

	if ( !prependCommands.empty() ) {
		for ( size_t i = 0; i < prependCommands.size(); ++i )
			model->setIcon( i, prependCommands[i].icon );
	}

	return model;
}

size_t ProjectDirectoryTree::getFilesCount() const {
	return mFiles.size();
}

const std::vector<std::string>& ProjectDirectoryTree::getFiles() const {
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

void ProjectDirectoryTree::tryAddFile( const FileInfo& file ) {
	IgnoreMatcherManager matcher( getIgnoreMatcherFromPath( file.getFilepath() ) );
	if ( !matcher.foundMatch() || !matcher.match( file.getDirectoryPath(), file.getFilepath() ) ) {
		bool foundPattern = mAcceptedPatterns.empty();
		for ( auto& pattern : mAcceptedPatterns ) {
			if ( pattern.matches( file.getFilepath() ) ) {
				foundPattern = true;
				break;
			}
		}
		if ( foundPattern ) {
			Lock l( mFilesMutex );
			mFiles.emplace_back( file.getFilepath() );
			mNames.emplace_back( file.getFileName() );
		}
	}
}

void ProjectDirectoryTree::addFile( const FileInfo& file ) {
	if ( file.isDirectory() ) {
		if ( !String::startsWith( file.getFilepath(), mPath ) || isDirInTree( file.getFilepath() ) )
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
			mFiles[index] = file.getFilepath();
			mNames[index] = file.getFileName();
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
		if ( dataName.is( Variant::Type::cstr ) && dataPath.is( Variant::Type::cstr ) ) {
			std::string fileName( dataName.asCStr() );
			std::string filePath( dataPath.asCStr() );
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
		std::string filePath( matchesMap.begin()->second );
		mApp->getUISceneNode()->runOnMainThread(
			[this, filePath]() { mApp->loadFileFromPathOrFocus( filePath ); } );
	}

	return PluginRequestHandle::broadcast();
}

} // namespace ecode
