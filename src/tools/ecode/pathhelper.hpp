#ifndef ECODE_PATHHELPER_HPP
#define ECODE_PATHHELPER_HPP

#include <algorithm>
#include <eepp/ui/doc/textposition.hpp>

using namespace EE;
using namespace EE::UI::Doc;

namespace ecode {

template <typename T> static bool pathHasPosition( const T& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	bool countedSep = std::count( path.begin(), path.end(), ':' ) > 1;
#else
	bool countedSep = std::count( path.begin(), path.end(), ':' ) > 0;
#endif
	if ( countedSep ) {
		auto seps = String::split( path, ':' );
		return !seps.empty() && String::isNumber( seps.back() );
	}
	return false;
}

template <typename T> static std::pair<T, TextPosition> getPathAndPosition( const T& path ) {
	if ( pathHasPosition( path ) ) {
		auto parts = String::split( path, ':' );
		if ( parts.size() >= 2 ) {
			Int64 line = 0;
			Int64 col = 0;
#if EE_PLATFORM == EE_PLATFORM_WIN
			size_t partCount = 4;
#else
			size_t partCount = 3;
#endif
			int linePos = parts.size() >= partCount ? parts.size() - 2 : parts.size() - 1;
			int colPos = parts.size() >= partCount ? parts.size() - 1 : -1;
			if ( String::fromString( line, parts[linePos] ) ) {
				if ( colPos > 0 )
					String::fromString( col, parts[colPos] );
			}
			std::string npath( parts[0] );
			if ( parts.size() >= 2 ) {
				for ( Int64 i = 1; i < linePos; i++ ) {
					npath += ":" + parts[i];
				}
			}
			return { npath, { eemax( (Int64)0, line - 1 ), col } };
		}
	}
	return { path, { 0, 0 } };
}

struct PathHelper {
	static bool isVideoExtension( std::string_view ext );

	static bool isDocumentExtension( std::string_view ext );

	static bool isOpenExternalExtension( std::string_view ext );

	static bool isCompressedArchiveExtension( std::string_view ext );
};

} // namespace ecode

#endif // ECODE_PATHHELPER_HPP
