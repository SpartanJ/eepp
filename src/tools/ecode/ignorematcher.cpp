#include "ignorematcher.hpp"
#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>

namespace ecode {

// Author:      Robert A. van Engelen, engelen@genivia.com
// Date:        August 5, 2019
// License:     The Code Project Open License (CPOL)
//              https://www.codeproject.com/info/cpol10.aspx

#define DOTGLOB 1
#define PATHSEP '/'
#define CASE( c, caseInsensitive ) ( caseInsensitive ? std::tolower( c ) : ( c ) )

bool IgnoreMatcher::globMatch( const std::string& text, const std::string_view& glob,
							   bool caseInsensitive ) {
	size_t i = 0;
	size_t j = 0;
	size_t n = text.size();
	size_t m = glob.size();
	size_t text1_backup = std::string::npos;
	size_t glob1_backup = std::string::npos;
	size_t text2_backup = std::string::npos;
	size_t glob2_backup = std::string::npos;
	bool nodot = !DOTGLOB;
	// match pathname if glob contains a / otherwise match the basename
	if ( j + 1 < m && glob[j] == '/' ) {
		// if pathname starts with ./ then ignore these pairs
		while ( i + 1 < n && text[i] == '.' && text[i + 1] == PATHSEP )
			i += 2;
		// if pathname starts with a / then ignore it
		if ( i < n && text[i] == PATHSEP )
			i++;
		j++;
	} else if ( glob.find( '/' ) == std::string::npos ) {
		size_t sep = text.rfind( PATHSEP );
		if ( sep != std::string::npos )
			i = sep + 1;
	}
	while ( i < n ) {
		if ( j < m ) {
			switch ( glob[j] ) {
				case '*':
					// match anything except . after /
					if ( nodot && text[i] == '.' )
						break;
					if ( ++j < m && glob[j] == '*' ) {
						// trailing ** match everything after /
						if ( ++j >= m )
							return true;
						// ** followed by a / match zero or more directories
						if ( glob[j] != '/' )
							return false;
						// new **-loop, discard *-loop
						text1_backup = std::string::npos;
						glob1_backup = std::string::npos;
						text2_backup = i;
						glob2_backup = ++j;
						continue;
					}
					// trailing * matches everything except /
					text1_backup = i;
					glob1_backup = j;
					continue;
				case '?':
					// match anything except . after /
					if ( nodot && text[i] == '.' )
						break;
					// match any character except /
					if ( text[i] == PATHSEP )
						break;
					i++;
					j++;
					continue;
				case '[': {
					// match anything except . after /
					if ( nodot && text[i] == '.' )
						break;
					// match any character in [...] except /
					if ( text[i] == PATHSEP )
						break;
					int lastchr;
					bool matched = false;
					bool reverse = j + 1 < m && ( glob[j + 1] == '^' || glob[j + 1] == '!' );
					// inverted character class
					if ( reverse )
						j++;
					// match character class
					for ( lastchr = 256; ++j < m && glob[j] != ']';
						  lastchr = CASE( glob[j], caseInsensitive ) )
						if ( lastchr < 256 && glob[j] == '-' && j + 1 < m && glob[j + 1] != ']'
								 ? CASE( text[i], caseInsensitive ) <=
										   CASE( glob[++j], caseInsensitive ) &&
									   CASE( text[i], caseInsensitive ) >= lastchr
								 : CASE( text[i], caseInsensitive ) ==
									   CASE( glob[j], caseInsensitive ) )
							matched = true;
					if ( matched == reverse )
						break;
					i++;
					if ( j < m )
						j++;
					continue;
				}
				case '\\':
					// literal match \-escaped character
					if ( j + 1 < m )
						j++;
					// FALLTHROUGH
				default:
					// match the current non-NUL character
					if ( CASE( glob[j], caseInsensitive ) != CASE( text[i], caseInsensitive ) &&
						 !( glob[j] == '/' && text[i] == PATHSEP ) )
						break;
					// do not match a . with *, ? [] after /
					nodot = !DOTGLOB && glob[j] == '/';
					i++;
					j++;
					continue;
			}
		}
		if ( glob1_backup != std::string::npos && text[text1_backup] != PATHSEP ) {
			// *-loop: backtrack to the last * but do not jump over /
			i = ++text1_backup;
			j = glob1_backup;
			continue;
		}
		if ( glob2_backup != std::string::npos ) {
			// **-loop: backtrack to the last **
			i = ++text2_backup;
			j = glob2_backup;
			continue;
		}
		return false;
	}
	// ignore trailing stars
	while ( j < m && glob[j] == '*' )
		j++;
	// at end of text means success if nothing else is left to match
	return j >= m;
}

bool IgnoreMatcher::globMatch( const std::string& text, const std::vector<std::string>& globs,
							   bool caseInsensitive ) {
	for ( const auto& glob : globs ) {
		if ( globMatch( text, glob, caseInsensitive ) )
			return true;
	}
	return false;
}

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
		match = globMatch( value, pattern.first );
		if ( pattern.second )
			match = !match;
		if ( match && !pattern.second ) {
			if ( i + 1 < mPatterns.size() && mPatterns[i + 1].second ) {
				for ( size_t n = i + 1; n < mPatterns.size(); n++ ) {
					// Check if there's a positive negate after the match
					if ( mPatterns[n].second ) {
						if ( globMatch( value, mPatterns[n].first ) )
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
