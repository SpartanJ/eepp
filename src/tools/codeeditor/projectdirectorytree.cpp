#include "projectdirectorytree.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>

ProjectDirectoryTree::ProjectDirectoryTree( const std::string& path,
											std::shared_ptr<ThreadPool> threadPool ) :
	mPath( path ), mPool( threadPool ), mIsReady( false ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

void ProjectDirectoryTree::scan( const ProjectDirectoryTree::ScanCompleteEvent& scanComplete,
								 const std::vector<std::string>& acceptedPattern ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	mPool->run(
		[&, acceptedPattern] {
#endif
			Lock l( mFilesMutex );
			if ( !acceptedPattern.empty() ) {
				std::vector<std::string> files;
				std::vector<std::string> names;
				std::vector<LuaPattern> patterns;
				for ( auto& strPattern : acceptedPattern )
					patterns.emplace_back( LuaPattern( strPattern ) );
				std::set<std::string> info;
				getDirectoryFiles( files, names, mPath, info );
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
				getDirectoryFiles( mFiles, mNames, mPath, info );
			}
			mIsReady = true;
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
			if ( scanComplete )
				scanComplete( *this );
#endif
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
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
	int score;
	for ( size_t i = 0; i < mNames.size(); i++ ) {
		if ( ( score = String::fuzzyMatch( mNames[i], match ) ) > 0 ) {
			matchesMap.insert( {score, i} );
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

std::shared_ptr<FileListModel> ProjectDirectoryTree::asModel( const size_t& max ) const {
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

void ProjectDirectoryTree::getDirectoryFiles( std::vector<std::string>& files,
											  std::vector<std::string>& names,
											  std::string directory,
											  std::set<std::string> currentDirs ) {
	currentDirs.insert( directory );
	std::vector<std::string> pathFiles = FileSystem::filesGetInPath( directory );
	for ( auto& file : pathFiles ) {
		std::string fullpath( directory + file );
		if ( FileSystem::isDirectory( fullpath ) ) {
			fullpath += FileSystem::getOSSlash();
			FileInfo dirInfo( fullpath, true );
			if ( dirInfo.isLink() ) {
				fullpath = dirInfo.linksTo();
				FileSystem::dirAddSlashAtEnd( fullpath );
				if ( currentDirs.find( fullpath ) == currentDirs.end() )
					continue;
			}
			getDirectoryFiles( files, names, fullpath, currentDirs );
		} else {
			files.emplace_back( fullpath );
			names.emplace_back( file );
		}
	}
}
