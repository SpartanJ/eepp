#ifndef EE_VFS_DETAIL_ARCHIVELOCATION_HPP
#define EE_VFS_DETAIL_ARCHIVELOCATION_HPP

#include <eepp/vfs/error.hpp>
#include <eepp/vfs/path.hpp>

namespace EE { namespace VFS { namespace detail {

/** Parsed archive identity and normalized path within it. */
class ArchiveLocation {
  public:
	URI archive;
	std::string member;

	/** Archive paths are untrusted. Reject names that could leave a copy destination or turn
	 * into native Windows paths/streams; do not alias unsafe entries to safe names. */
	static bool safeEntry( std::string_view name ) {
		if ( name.empty() || name.size() > 4096 || name.front() == '/' ||
			 name.find_first_of( std::string_view( "\\:\0", 3 ) ) != std::string_view::npos )
			return false;
		size_t start = 0;
		while ( start < name.size() ) {
			size_t end = name.find( '/', start );
			if ( end == std::string_view::npos )
				end = name.size();
			auto part = name.substr( start, end - start );
			// Windows trims trailing dots/spaces; an all-dot/space segment can become a parent.
			if ( part.empty() || part.find_first_not_of( ". " ) == std::string_view::npos )
				return false;
			start = end + 1;
		}
		return true;
	}

	static VFSResult<ArchiveLocation> parse( const URI& uri, std::string_view scheme ) {
		if ( uri.getScheme() != scheme || uri.getAuthority() != uri.getHost() ||
			 !uri.getFragment().empty() || !VFSPath::validURI( uri ) )
			return Err( VFSError( VFSErrorCode::InvalidURI, uri, "Invalid archive location" ) );
		if ( !uri.getRawQuery().empty() )
			return Err(
				VFSError( VFSErrorCode::Unsupported, uri, "Unsupported archive backing URI" ) );
		const auto& path = uri.getPath();
		if ( path.empty() || path.front() != '/' )
			return Err(
				VFSError( VFSErrorCode::InvalidURI, uri, "archive path must be absolute" ) );
		std::string backing;
		backing.reserve( path.size() );
		size_t boundary = std::string::npos;
		for ( size_t i = 0; i < path.size(); ++i ) {
			if ( path[i] == '!' ) {
				if ( i + 1 < path.size() && path[i + 1] == '!' ) {
					backing += '!';
					++i;
					continue;
				}
				if ( i + 1 == path.size() || path[i + 1] == '/' ) {
					boundary = i + 1;
					break;
				}
			}
			backing += path[i];
		}
		if ( boundary == std::string::npos || backing.empty() || backing.back() == '/' )
			return Err(
				VFSError( VFSErrorCode::InvalidURI, uri, "archive path needs archive!/member" ) );
		ArchiveLocation result;
		result.archive.setScheme( "file" );
		result.archive.setHost( uri.getHost() );
		result.archive.setRawPath( std::move( backing ) );
		URI member;
		member.setScheme( std::string( scheme ) );
		member.setRawPath( boundary == path.size() ? "/" : path.substr( boundary ) );
		member.normalize();
		result.member = member.getPath();
		return Ok( std::move( result ) );
	}
	static URI make( std::string_view scheme, const URI& archive, std::string_view member ) {
		if ( archive.getScheme() != "file" || !archive.getRawQuery().empty() ||
			 !archive.getFragment().empty() || archive.getAuthority() != archive.getHost() ||
			 archive.getPath().empty() || archive.getPath().front() != '/' ||
			 !VFSPath::validURI( archive ) || member.find( '\0' ) != std::string_view::npos )
			return {};
		URI uri;
		uri.setScheme( std::string( scheme ) );
		uri.setHost( archive.getHost() );
		std::string path;
		path.reserve( archive.getPath().size() + member.size() + 2 );
		// Escape literal bangs in the backing path, so even a directory named "packages!" is
		// unambiguous. Only the first unescaped "!/" separates the archive from its member.
		for ( char c : archive.getPath() ) {
			path += c;
			if ( c == '!' )
				path += '!';
		}
		URI entry;
		entry.setScheme( std::string( scheme ) );
		entry.setRawPath( member.empty() || member.front() != '/' ? "/" + std::string( member )
																  : std::string( member ) );
		entry.normalize();
		path += '!';
		path += entry.getPath();
		uri.setRawPath( std::move( path ) );
		return uri;
	}
};

}}} // namespace EE::VFS::detail

#endif
