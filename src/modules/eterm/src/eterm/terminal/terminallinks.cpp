#include <eterm/terminal/terminallinks.hpp>

#include <cstdlib>
#include <eepp/core/string.hpp>
#include <eepp/core/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

namespace eterm { namespace Terminal {

/* Ghostty's default URL and path pattern (src/config/url.zig), assembled from the same pieces,
 * used under its license:
 *
 * MIT License
 *
 * Copyright (c) 2024 Mitchell Hashimoto, Ghostty contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
static std::string buildPattern() {
	const std::string urlSchemes =
		R"(https?://|mailto:|ftp://|file:|ssh:|git://|ssh://|tel:|magnet:|ipfs://|ipns://|gemini://|gopher://|news:)";
	const std::string ipv6UrlPattern = R"((?:\[[:0-9a-fA-F]+(?:[:0-9a-fA-F]*)+\](?::[0-9]+)?))";
	const std::string schemeUrlChars = R"([\w\-.~:/?#@!$&*+,;=%])";
	const std::string pathChars = R"([\w\-.~:\/?#@!$&*+;=%])";
	const std::string optionalBracketedWordSuffix = R"((?:[\(\[]\w*[\)\]])?)";
	const std::string noTrailingPunctuation = R"((?<![,.]))";
	const std::string noTrailingColon = R"((?<!:))";
	const std::string dottedPathLookahead = R"((?=[\w\-.~:\/?#@!$&*+;=%]*\.))";
	const std::string nonDottedPathLookahead = R"((?![\w\-.~:\/?#@!$&*+;=%]*\.))";
	const std::string dottedPathSpaceSegments =
		R"((?:(?<!:) (?!\w+:\/\/)(?!\.{0,2}\/)(?!~\/)[\w\-.~:\/?#@!$&*+;=%]*[\/.])*)";
	const std::string anyPathSpaceSegments =
		R"((?:(?<!:) (?!\w+:\/\/)(?!\.{0,2}\/)(?!~\/)[\w\-.~:\/?#@!$&*+;=%]+)*)";

	// Branch 1: URLs with explicit schemes (http, mailto, ftp, etc.).
	const std::string schemeUrlBranch = "(?:" + urlSchemes + ")" + "(?:" + ipv6UrlPattern + "|" +
										schemeUrlChars + "+" + optionalBracketedWordSuffix + ")+" +
										noTrailingPunctuation;

	// Branch 2: absolute and dot-relative paths (/, ./, ../, ~/, $VAR/).
	const std::string rootedOrRelativePathPrefix =
		R"((?:\.\.\/|\.\/|(?<!\w)~\/|(?:[\w][\w\-.]*\/)*(?<!\w)\$[A-Za-z_]\w*\/|\.[\w][\w\-.]*\/|(?<![\w~\/])\/(?!\/)))";
	const std::string rootedOrRelativePathBranch =
		rootedOrRelativePathPrefix + "(?:" + dottedPathLookahead + pathChars + "+" +
		dottedPathSpaceSegments + noTrailingColon + "|" + nonDottedPathLookahead + pathChars + "+" +
		anyPathSpaceSegments + noTrailingColon + ")";

	// Branch 3: bare relative paths such as src/config/url.zig.
	// Ghostty writes (?<!\$\d*), which PCRE2 rejects; (?<!\w) already excludes a preceding digit,
	// so only a directly preceding $ remains to rule out.
	const std::string bareRelativePathPrefix = R"((?<!\$)(?<!\w)[\w][\w\-.]*\/)";
	const std::string bareRelativePathBranch =
		dottedPathLookahead + bareRelativePathPrefix + pathChars + "+" + noTrailingColon;

	// eterm's addition, branch 4: Windows drive paths (C:/x, C:\x). Branch 2 would match only
	// what follows the drive, and nothing for backslashes. The branch mirrors branch 2, with
	// backslash as a separator and a drive as one more token that starts a new path.
	const std::string windowsPathChars = R"([\w\-.~:\/\\?#@!$&*+;=%])";
	const std::string windowsPathClass = R"([\w\-.~:\/\\?#@!$&*+;=%]*)";
	const std::string windowsNewToken =
		R"((?!\w+:\/\/)(?!\.{0,2}[\/\\])(?!~[\/\\])(?![A-Za-z]:[\/\\]))";
	const std::string windowsDottedSpaceSegments =
		"(?:(?<!:) " + windowsNewToken + windowsPathClass + R"([\/\\.])*)";
	const std::string windowsAnySpaceSegments =
		"(?:(?<!:) " + windowsNewToken + windowsPathChars + "+)*";
	const std::string windowsPathBranch =
		R"((?<![\w:\/\\])[A-Za-z]:[\/\\])" + std::string( "(?:" ) + "(?=" + windowsPathClass +
		R"(\.))" + windowsPathChars + "+" + windowsDottedSpaceSegments + noTrailingColon + "|" +
		"(?!" + windowsPathClass + R"(\.))" + windowsPathChars + "+" + windowsAnySpaceSegments +
		noTrailingColon + ")";

	return schemeUrlBranch + "|" + windowsPathBranch + "|" + rootedOrRelativePathBranch + "|" +
		   bareRelativePathBranch;
}

std::string_view TerminalLinks::pattern() {
	static const std::string pattern = buildPattern();
	return pattern;
}

std::optional<TerminalLinks::Match> TerminalLinks::findAt( std::string_view text,
														   std::size_t offset ) {
	if ( offset >= text.size() )
		return std::nullopt;
	// One instance per call: RegEx keeps match state, and the compiled pattern is cached.
	// Unicode \w, as in Ghostty (Oniguruma).
	RegEx regex( pattern(), RegEx::Utf | RegEx::Ucp );
	if ( !regex.isValid() )
		return std::nullopt;
	PatternMatcher::Range ranges[10];
	int start = 0;
	while ( start < static_cast<int>( text.size() ) &&
			regex.matches( text.data(), start, ranges, text.size() ) ) {
		const auto& range = ranges[0];
		if ( range.start < 0 || range.end <= range.start )
			break;
		if ( static_cast<std::size_t>( range.start ) > offset )
			break;
		if ( offset < static_cast<std::size_t>( range.end ) )
			return Match{ static_cast<std::size_t>( range.start ),
						  static_cast<std::size_t>( range.end ) };
		start = range.end;
	}
	return std::nullopt;
}

static bool isSchemeUrl( std::string_view link ) {
	std::size_t i = 0;
	while ( i < link.size() &&
			( ( link[i] >= 'a' && link[i] <= 'z' ) || ( link[i] >= 'A' && link[i] <= 'Z' ) ) )
		++i;
	return i > 1 && i < link.size() && link[i] == ':';
}

/* C:/x or C:\x. */
static bool isDrivePath( std::string_view link ) {
	return link.size() >= 3 &&
		   ( ( link[0] >= 'a' && link[0] <= 'z' ) || ( link[0] >= 'A' && link[0] <= 'Z' ) ) &&
		   link[1] == ':' && ( link[2] == '/' || link[2] == '\\' );
}

static bool exists( const std::string& path ) {
	return FileSystem::fileExists( path ) || FileSystem::isDirectory( path );
}

/* Drops a trailing :line or :line:column, as compilers and grep print. */
static std::string_view withoutLocation( std::string_view path ) {
	for ( int part = 0; part < 2; ++part ) {
		const auto colon = path.find_last_of( ':' );
		if ( colon == std::string_view::npos || colon + 1 == path.size() )
			break;
		bool digits = true;
		for ( std::size_t i = colon + 1; i < path.size() && digits; ++i )
			digits = path[i] >= '0' && path[i] <= '9';
		if ( !digits )
			break;
		path = path.substr( 0, colon );
	}
	return path;
}

std::string TerminalLinks::resolve( std::string_view link, const std::string& workingDirectory,
									std::string* resolvedPath ) {
	if ( link.empty() )
		return {};
	if ( isSchemeUrl( link ) )
		return std::string( link );

	std::string path;
	if ( link.size() >= 2 && link[0] == '~' && link[1] == '/' ) {
		path = Sys::getUserDirectory();
		FileSystem::dirRemoveSlashAtEnd( path );
		path += link.substr( 1 );
	} else if ( link[0] == '$' ) {
		const auto slash = link.find( '/' );
		if ( slash == std::string_view::npos )
			return {};
		const std::string name( link.substr( 1, slash - 1 ) );
		const char* value = std::getenv( name.c_str() );
		if ( !value || !*value )
			return {};
		path = value;
		FileSystem::dirRemoveSlashAtEnd( path );
		path += link.substr( slash );
	} else if ( link[0] == '/' || isDrivePath( link ) ) {
		path = link;
	} else {
		if ( workingDirectory.empty() )
			return {};
		path = workingDirectory;
		FileSystem::dirAddSlashAtEnd( path );
		path += link;
	}

	bool found = exists( path );
	// A sentence can end with a path and a period. Prefer a real dot-ending filename,
	// otherwise retry without the period before stripping a compiler's :line:column.
	if ( !found && path.back() == '.' ) {
		path.pop_back();
		found = exists( path );
	}
	if ( !found ) {
		const auto trimmed = withoutLocation( path );
		if ( trimmed.size() == path.size() )
			return {};
		path.resize( trimmed.size() );
		if ( !exists( path ) )
			return {};
	}
	if ( resolvedPath )
		*resolvedPath = path;
	std::string uri( "file://" );
	if ( isDrivePath( path ) ) {
		// file:///C:/x: a drive path needs a slash before it and forward separators.
		uri += '/';
		std::string slashes( path );
		String::replaceAll( slashes, "\\", "/" );
		URI::encode( slashes, "?#", uri );
		return uri;
	}
	URI::encode( path, "?#", uri ); // A path keeps its slashes; ? and # would end it.
	return uri;
}

}} // namespace eterm::Terminal
