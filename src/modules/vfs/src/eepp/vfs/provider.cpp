#include <eepp/vfs/provider.hpp>

#include <charconv>
#include <eepp/core/string.hpp>
#include <limits>

namespace EE { namespace VFS {

VFSTask<Ownership> VFSProvider::resolveOwnership( const URI& uri, std::string user,
												  std::string group, CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return readyError<Ownership>( cancelledError( uri ) );
	Ownership result;
	auto parse = []( const std::string& text, std::optional<Uint32>& id ) {
		if ( text.empty() )
			return true;
		Uint32 value;
		auto parsed = std::from_chars( text.data(), text.data() + text.size(), value );
		if ( parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() ||
			 value == std::numeric_limits<Uint32>::max() )
			return false;
		id = value;
		return true;
	};
	if ( !parse( user, result.uid ) || !parse( group, result.gid ) ) {
		return readyError<Ownership>( VFSErrorCode::InvalidOperation, uri,
									  "This provider requires numeric user and group IDs" );
	}
	return readyValue<Ownership>( std::move( result ) );
}

bool VFSProvider::relocateURI( URI& uri, const URI& from, const URI& to ) const {
	if ( !from.hasSameAuthority( to ) )
		return false;
	if ( !uri.isSameOrChildOf( from ) )
		return false;
	const auto base = String::rTrim( std::string_view( from.getPath() ), '/' );
	const auto destination = String::rTrim( std::string_view( to.getPath() ), '/' );
	std::string path;
	path.reserve( destination.size() + uri.getPath().size() - base.size() );
	path.append( destination );
	path.append( uri.getPath(), base.size(), std::string::npos );
	uri.setRawPath( std::move( path ) );
	return true;
}

}} // namespace EE::VFS
