#include "projectdirectorytree.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>

ProjectDirectoryTree::ProjectDirectoryTree( const std::string& path,
											std::shared_ptr<ThreadPool> threadPool ) :
	mPath( path ), mPool( threadPool ), mIsReady( false ), mIgnoreMatcher( path ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

void ProjectDirectoryTree::scan( const ProjectDirectoryTree::ScanCompleteEvent& scanComplete,
								 const std::vector<std::string>& acceptedPatterns,
								 const bool& ignoreHidden ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
	mPool->run(
		[&, acceptedPatterns, ignoreHidden] {
#endif
			Lock l( mFilesMutex );
			mIgnoreHidden = ignoreHidden;
			mDirectories.push_back( mPath );
			if ( !acceptedPatterns.empty() ) {
				std::vector<std::string> files;
				std::vector<std::string> names;
				std::vector<LuaPattern> patterns;
				for ( auto& strPattern : acceptedPatterns )
					patterns.emplace_back( LuaPattern( strPattern ) );
				mAcceptedPatterns = patterns;
				std::set<std::string> info;
				getDirectoryFiles( files, names, mPath, info, ignoreHidden, mIgnoreMatcher );
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
				getDirectoryFiles( mFiles, mNames, mPath, info, ignoreHidden, mIgnoreMatcher );
			}
			mIsReady = true;
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN && !defined( __EMSCRIPTEN_PTHREADS__ )
			if ( scanComplete )
				scanComplete( *this );
#endif
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		},
		[scanComplete, this] {
			if ( scanComplete )
				scanComplete( *this );
		} );
#endif
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::fuzzyMatchTree( const std::string& match,
																	 const size_t& max ) const {
	std::multimap<int, int, std::greater<int>> matchesMap;
	std::vector<std::string> files;
	std::vector<std::string> names;
	for ( size_t i = 0; i < mNames.size(); i++ )
		matchesMap.insert( { String::fuzzyMatch( mNames[i], match ), i } );
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
	mPool->run( [&, match, max, res]() { res( fuzzyMatchTree( match, max ) ); }, []() {} );
}

void ProjectDirectoryTree::asyncMatchTree( const std::string& match, const size_t& max,
										   ProjectDirectoryTree::MatchResultCb res ) const {
	mPool->run( [&, match, max, res]() { res( matchTree( match, max ) ); }, []() {} );
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::asModel( const size_t& max ) const {
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
	return std::make_shared<FileListModel>( files, names );
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

void ProjectDirectoryTree::getDirectoryFiles( std::vector<std::string>& files,
											  std::vector<std::string>& names,
											  std::string directory,
											  std::set<std::string> currentDirs,
											  const bool& ignoreHidden,
											  const IgnoreMatcherManager& ignoreMatcher ) {
	currentDirs.insert( directory );
	std::string localDirPath( directory.substr(
		ignoreMatcher.foundMatch() ? ignoreMatcher.getPath().size() : mPath.size() ) );
	std::vector<std::string> pathFiles =
		FileSystem::filesGetInPath( directory, false, false, ignoreHidden );
	for ( auto& file : pathFiles ) {
		std::string fullpath( directory + file );
		std::string localpath( localDirPath + file );
		if ( ignoreMatcher.foundMatch() && ignoreMatcher.match( localpath ) )
			continue;
		if ( FileSystem::isDirectory( fullpath ) ) {
			fullpath += FileSystem::getOSSlash();
			FileInfo dirInfo( fullpath, true );
			if ( dirInfo.isLink() ) {
				fullpath = dirInfo.linksTo();
				FileSystem::dirAddSlashAtEnd( fullpath );
				if ( currentDirs.find( fullpath ) == currentDirs.end() )
					continue;
				mDirectories.push_back( fullpath );
			} else {
				mDirectories.push_back( fullpath );
			}
			IgnoreMatcherManager dirMatcher( fullpath );
			getDirectoryFiles( files, names, fullpath, currentDirs, ignoreHidden,
							   dirMatcher.foundMatch() ? dirMatcher : ignoreMatcher );
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
			getDirectoryFiles( files, names, mPath, info, mIgnoreHidden, mIgnoreMatcher );
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
			getDirectoryFiles( mFiles, mNames, mPath, info, mIgnoreHidden, mIgnoreMatcher );
		}
	} else {
		if ( file.isHidden() && mIgnoreHidden )
			return;

		IgnoreMatcherManager matcher( getIgnoreMatcherFromPath( file.getFilepath() ) );
		if ( !matcher.foundMatch() ||
			 ( matcher.foundMatch() && !matcher.match( file.getFilepath() ) ) ) {
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
	std::string ldir;
	FileSystem::dirAddSlashAtEnd( dir );
	IgnoreMatcherManager dirMatcher( dir );
	while ( !dirMatcher.foundMatch() ) {
		dirMatcher = IgnoreMatcherManager( dir );
		if ( !dirMatcher.foundMatch() ) {
			if ( dir.empty() || dir.find_first_of( "/\\" ) == std::string::npos || dir == mPath ||
				 dir == ldir )
				break;
			ldir = dir;
			FileSystem::dirRemoveSlashAtEnd( dir );
			dir = FileSystem::fileRemoveFileName( dir );
			FileSystem::dirAddSlashAtEnd( dir );
		}
	}
	return dirMatcher;
}

size_t ProjectDirectoryTree::findFileIndex( const std::string& path ) {
	for ( size_t i = 0; i < mFiles.size(); i++ ) {
		if ( mFiles[i] == path )
			return i;
	}
	return std::string::npos;
}
