#include <eepp/vfs/providers/localprovider.hpp>

#include <eepp/core/string.hpp>

namespace EE { namespace VFS {

LocalProvider::LocalProvider( std::shared_ptr<LocalIOScheduler> scheduler ) :
	mScheduler( std::move( scheduler ) ) {}

LocalProvider::~LocalProvider() = default;

VFSTask<bool> LocalProvider::sameFileSystem( const URI& from, const URI& to,
											 CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return readyError<bool>( cancelledError( from ) );
	if ( from.hasSameAuthority( to ) )
		return readyValue<bool>( true );
	auto source = toNativePath( from );
	if ( !source )
		return readyError<bool>( std::move( source ).error() );
	auto destination = toNativePath( to );
	if ( !destination )
		return readyError<bool>( std::move( destination ).error() );
	const std::string_view sourceHost =
		from.getHost() == "localhost" ? std::string_view{} : std::string_view( from.getHost() );
	const std::string_view destinationHost =
		to.getHost() == "localhost" ? std::string_view{} : std::string_view( to.getHost() );
	// Local paths share a namespace even across devices. Different UNC hosts may still be
	// aliases of one server; without resolving that identity, reject conservatively.
	if ( String::iequals( sourceHost, destinationHost ) )
		return readyValue<bool>( true );
	return readyError<bool>( VFSErrorCode::Unsupported, to,
							 "Cannot determine identity across UNC hosts" );
}

}} // namespace EE::VFS
