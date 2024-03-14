#include "ignorematcher.hpp"
#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>

namespace ecode {

IgnoreMatcher::IgnoreMatcher( const std::string& rootPath ) : mPath( rootPath ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

IgnoreMatcher::~IgnoreMatcher() {}

GitIgnoreMatcher::GitIgnoreMatcher( const std::string& rootPath,
									const std::string& ignoreFileName ) :
	IgnoreMatcher( rootPath ),
	mIgnoreFileName( ignoreFileName ),
	mIgnoreFilePath( mPath + mIgnoreFileName ) {
	if ( canMatch() ) {
		if ( parse() ) {
			mMatcherReady = true;
		}
	}
}

bool GitIgnoreMatcher::canMatch() {
	return FileSystem::fileExists( mIgnoreFilePath );
}

const std::string& GitIgnoreMatcher::getIgnoreFilePath() const {
	return mIgnoreFilePath;
}

bool GitIgnoreMatcher::parse() {
	std::string patternFile;
	FileSystem::fileGet( mPath + mIgnoreFileName, patternFile );
	if ( FileSystem::fileExists( mPath + ".git" ) ) // Also ignore the .git folder
		mPatterns.emplace_back( std::make_pair( "**/.git/**", false ) );
	std::vector<std::string> patterns = String::split( patternFile );
	for ( auto& pattern : patterns ) {
		bool negates = false;
		if ( pattern.empty() )
			continue;
		if ( pattern[0] == '#' )
			continue;
		pattern = String::rTrim( pattern, ' ' );
		pattern = String::rTrim( pattern, '\r' );
		if ( pattern[0] == '!' ) {
			negates = true;
			pattern = String::lTrim( pattern, '!' );
		}
		pattern = String::rTrim( pattern, '/' );
		mPatterns.emplace_back( std::make_pair( pattern, negates ) );
	}
	return !mPatterns.empty();
}

bool GitIgnoreMatcher::match( const std::string& value ) const {
	if ( mPatterns.empty() )
		return false;
	bool match = false;
	for ( size_t i = 0; i < mPatterns.size(); i++ ) {
		auto& pattern = mPatterns[i];
		match = String::globMatch( value, pattern.first );
		if ( pattern.second )
			match = !match;
		if ( match && !pattern.second ) {
			if ( i + 1 < mPatterns.size() && mPatterns[i + 1].second ) {
				for ( size_t n = i + 1; n < mPatterns.size(); n++ ) {
					// Check if there's a positive negate after the match
					if ( mPatterns[n].second ) {
						if ( String::globMatch( value, mPatterns[n].first ) )
							return false;
					} else {
						break;
					}
				}
			}
			return true;
		}
	}
	return false;
}

std::string GitIgnoreMatcher::findRepositoryRootPath() const {
	std::string rootPath( mPath );
	std::string lRootPath;
	FileSystem::dirAddSlashAtEnd( rootPath );
	while ( rootPath != lRootPath && !FileSystem::fileExists( rootPath + ".git" ) ) {
		lRootPath = rootPath;
		rootPath = FileSystem::removeLastFolderFromPath( rootPath );
	}
	return FileSystem::fileExists( rootPath + ".git" ) ? rootPath : "";
}

IgnoreMatcherManager::IgnoreMatcherManager( IgnoreMatcherManager&& ignoreMatcher ) :
	mMatchers( ignoreMatcher.mMatchers ) {
	ignoreMatcher.mMatchers.clear();
}

IgnoreMatcherManager::IgnoreMatcherManager( std::string rootPath ) {
	FileSystem::dirAddSlashAtEnd( rootPath );
	mRootPath = rootPath;
	GitIgnoreMatcher git( rootPath );
	if ( git.canMatch() )
		mMatchers.emplace_back( eeNew( GitIgnoreMatcher, ( rootPath ) ) );
}

IgnoreMatcherManager& IgnoreMatcherManager::operator=( IgnoreMatcherManager&& other ) {
	mMatchers = other.mMatchers;
	other.mMatchers.clear();
	return *this;
}

IgnoreMatcherManager::~IgnoreMatcherManager() {
	for ( size_t i = 0; i < mMatchers.size(); ++i )
		eeDelete( mMatchers[i] );
}

bool IgnoreMatcherManager::foundMatch() const {
	return !mMatchers.empty();
}

bool IgnoreMatcherManager::match( const FileInfo& file ) const {
	eeASSERT( foundMatch() );
	auto dirPath( file.getDirectoryPath() );
	std::string localPath;
	for ( const auto& matcher : mMatchers ) {
		localPath.clear();
		if ( String::startsWith( dirPath, matcher->getPath() ) )
			localPath = dirPath.substr( matcher->getPath().size() );
		if ( matcher->match( localPath + file.getFileName() ) )
			return true;
	}
	return false;
}

bool IgnoreMatcherManager::match( const std::string& dir, const std::string& value ) const {
	eeASSERT( foundMatch() );
	std::string localPath;
	for ( const auto& matcher : mMatchers ) {
		localPath.clear();
		if ( String::startsWith( dir, matcher->getPath() ) )
			localPath = dir.substr( matcher->getPath().size() );
		if ( matcher->match( localPath + value ) )
			return true;
	}
	return false;
}

std::string IgnoreMatcherManager::findRepositoryRootPath() const {
	if ( mMatchers.empty() ) {
		std::string rootPath( mRootPath );
		std::string lRootpath;
		while ( rootPath != lRootpath && !rootPath.empty() ) {
			GitIgnoreMatcher git( rootPath );
			if ( git.canMatch() )
				return git.findRepositoryRootPath();
			lRootpath = rootPath;
			rootPath = FileSystem::removeLastFolderFromPath( rootPath );
		}
		return "";
	}
	return mMatchers.front()->findRepositoryRootPath();
}

const std::string& IgnoreMatcherManager::getPath() const {
	eeASSERT( foundMatch() );
	return mMatchers[mMatchers.size() - 1]->getPath();
}

const std::string& IgnoreMatcherManager::getRootPath() const {
	return mRootPath;
}

void IgnoreMatcherManager::addChild( IgnoreMatcher* child ) {
	mMatchers.emplace_back( child );
}

void IgnoreMatcherManager::removeChild( IgnoreMatcher* child ) {
	auto it = std::find( mMatchers.begin(), mMatchers.end(), child );
	if ( it != mMatchers.end() )
		mMatchers.erase( it );
}

IgnoreMatcher* IgnoreMatcherManager::popMatcher( size_t index ) {
	IgnoreMatcher* matcher = mMatchers[index];
	mMatchers.erase( mMatchers.begin() + index );
	return matcher;
}

const std::vector<IgnoreMatcher*>& IgnoreMatcherManager::getMatchers() const {
	return mMatchers;
}

} // namespace ecode
