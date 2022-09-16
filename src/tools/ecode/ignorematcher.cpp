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

bool gitignore_glob_match( const std::string& text, const std::string& glob,
						   bool caseInsensitive = false ) {
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

IgnoreMatcher::IgnoreMatcher( const std::string& rootPath ) : mPath( rootPath ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

IgnoreMatcher::~IgnoreMatcher() {}

GitIgnoreMatcher::GitIgnoreMatcher( const std::string& rootPath ) : IgnoreMatcher( rootPath ) {
	if ( canMatch() )
		parse();
}

bool GitIgnoreMatcher::canMatch() {
	return FileSystem::fileExists( mPath + ".gitignore" );
}

bool GitIgnoreMatcher::parse() {
	std::string patternFile;
	FileSystem::fileGet( mPath + ".gitignore", patternFile );
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
	mPatterns.emplace_back( std::make_pair( "/.git", false ) ); // Also ignore the .git folder
	return !mPatterns.empty();
}

bool GitIgnoreMatcher::match( const std::string& value ) const {
	bool match = false;
	for ( size_t i = 0; i < mPatterns.size(); i++ ) {
		auto& pattern = mPatterns[i];
		match = gitignore_glob_match( value, pattern.first );
		if ( ( match && !pattern.second ) || ( !match && pattern.second ) ) {
			if ( !pattern.second && i + 1 < mPatterns.size() && mPatterns[i + 1].second ) {
				for ( size_t n = i + 1; n < mPatterns.size(); n++ ) {
					// Check if there's a positive negate after the match
					if ( mPatterns[n].second ) {
						if ( gitignore_glob_match( value, mPatterns[n].first ) )
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

IgnoreMatcherManager::IgnoreMatcherManager( IgnoreMatcherManager&& ignoreMatcher ) :
	mMatchers( ignoreMatcher.mMatchers ) {
	ignoreMatcher.mMatchers.clear();
}

IgnoreMatcherManager::IgnoreMatcherManager( std::string rootPath ) {
	FileSystem::dirAddSlashAtEnd( rootPath );
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

bool IgnoreMatcherManager::match( const std::string& value ) const {
	eeASSERT( foundMatch() );
	for ( const auto& matcher : mMatchers )
		if ( matcher->match( value ) )
			return true;
	return false;
}

const std::string& IgnoreMatcherManager::getPath() const {
	eeASSERT( foundMatch() );
	return mMatchers[mMatchers.size() - 1]->getPath();
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

} // namespace ecode
