#include <eepp/system/packfileindex.hpp>
#include <eepp/vfs/detail/memoryhandles.hpp>
#include <eepp/vfs/providers/resourceprovider.hpp>

namespace EE { namespace VFS {

using System::PackFileIndex;

namespace {

constexpr ProviderCapabilities
	ResourceCapabilities( static_cast<Uint32>( VFSCapability::Read ) |
						  static_cast<Uint32>( VFSCapability::List ) |
						  static_cast<Uint32>( VFSCapability::Seek ) |
						  static_cast<Uint32>( VFSCapability::RandomAccess ) |
						  static_cast<Uint32>( VFSCapability::CaseSensitive ) );

VFSError readOnly( const URI& uri ) {
	return VFSError( VFSErrorCode::ReadOnly, uri, "res:// is read-only" );
}

FileInfo makeInfo( std::string name, bool directory, Uint64 generation ) {
	FileInfo info;
	info.name = std::move( name );
	info.type = directory ? FileType::Directory : FileType::Regular;
	info.hidden = !info.name.empty() && info.name[0] == '.';
	info.setPermissions( directory ? 0555 : 0444 );
	info.revision = FileRevision::fromValues( { generation } );
	return info;
}

/** Normalized res:// URI for a pack path: res:///a/b. */
URI resourceURI( const std::string& packPath ) {
	URI uri;
	uri.setScheme( "res" );
	uri.setRawPath( "/" + packPath );
	return uri;
}

} // namespace

ResourceProvider::ResourceProvider( std::shared_ptr<System::ThreadPool> pool ) :
	mPool( std::move( pool ) ) {}

ProviderCapabilities ResourceProvider::capabilities( const URI& ) const {
	return ResourceCapabilities;
}

std::string ResourceProvider::toPackPath( const URI& uri ) {
	// Authority-style URIs ("res://fonts/a.ttf") are accepted as a convenience: the host is the
	// first path segment.
	std::string path( uri.getHost() );
	const std::string& uriPath = uri.getPath();
	size_t start = 0;
	while ( start < uriPath.size() && uriPath[start] == '/' )
		++start;
	if ( !path.empty() && start < uriPath.size() )
		path += '/';
	path.append( uriPath, start );
	while ( !path.empty() && path.back() == '/' )
		path.pop_back();
	return path;
}

VFSTask<FileInfo> ResourceProvider::stat( const URI& uri, const StatOptions&,
										  CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return readyError<FileInfo>( cancelledError( uri ) );
	// Index lookups are in-memory: answer synchronously.
	auto* index = PackFileIndex::instance();
	std::string path = toPackPath( uri );
	Uint64 generation = index->getGeneration();
	size_t slash = path.find_last_of( '/' );
	std::string name = path.empty()
						   ? std::string( "/" )
						   : ( slash == std::string::npos ? path : path.substr( slash + 1 ) );
	if ( path.empty() || index->directoryExists( path ) )
		return readyValue<FileInfo>( makeInfo( std::move( name ), true, generation ) );
	if ( index->fileExists( path ) )
		return readyValue<FileInfo>( makeInfo( std::move( name ), false, generation ) );
	return readyError<FileInfo>( VFSErrorCode::NotFound, uri );
}

VFSTask<DirectoryHandle> ResourceProvider::openDirectory( const URI& uri,
														  const DirectoryOpenOptions&,
														  CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return readyError<DirectoryHandle>( cancelledError( uri ) );
	auto* index = PackFileIndex::instance();
	std::string path = toPackPath( uri );
	if ( !path.empty() && !index->directoryExists( path ) ) {
		return readyError<DirectoryHandle>(
			index->fileExists( path ) ? VFSErrorCode::NotDirectory : VFSErrorCode::NotFound, uri );
	}
	Uint64 generation = index->getGeneration();
	auto listing = index->listDirectory( path );
	std::vector<FileInfo> entries;
	entries.reserve( listing.size() );
	for ( auto& entry : listing )
		entries.push_back( makeInfo( std::move( entry.name ), entry.isDirectory, generation ) );
	return readyValue<DirectoryHandle>(
		uri, std::make_shared<detail::VectorDirectoryImpl>( std::move( entries ) ) );
}

VFSTask<FileHandle> ResourceProvider::open( const URI& uri, const OpenOptions& options,
											CancellationToken token ) {
	if ( hasFlag( options.flags, OpenFlags::Write ) ||
		 hasFlag( options.flags, OpenFlags::Create ) ||
		 hasFlag( options.flags, OpenFlags::Truncate ) )
		return readyError<FileHandle>( readOnly( uri ) );
	if ( token.isCancellationRequested() )
		return readyError<FileHandle>( cancelledError( uri ) );
	std::string path = toPackPath( uri );
	auto* index = PackFileIndex::instance();
	if ( path.empty() || index->directoryExists( path ) )
		return readyError<FileHandle>( VFSErrorCode::IsDirectory, uri );
	// Decompression may be slow: do it on the I/O pool.
	return mPool->submit(
		[uri, path = std::move( path ), token = std::move( token )]() -> VFSResult<FileHandle> {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( uri ) );
			auto* index = PackFileIndex::instance();
			Uint64 generation = index->getGeneration();
			auto data = std::make_shared<std::vector<Uint8>>();
			if ( !index->extractFileToMemory( path, *data ) )
				return Err( VFSError( VFSErrorCode::NotFound, uri ) );
			size_t slash = path.find_last_of( '/' );
			FileInfo info = makeInfo( slash == std::string::npos ? path : path.substr( slash + 1 ),
									  false, generation );
			info.setSize( data->size() );
			return Ok( FileHandle(
				uri, std::make_shared<detail::BufferFileImpl>(
						 std::move( data ), uri, std::move( info ), ResourceCapabilities ) ) );
		} );
}

VFSTask<void> ResourceProvider::mkdir( const URI& uri, const MkDirOptions&, CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<void> ResourceProvider::remove( const URI& uri, const RemoveOptions&, CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<void> ResourceProvider::rename( const URI& from, const URI&, const RenameOptions&,
										CancellationToken ) {
	return readyError<void>( readOnly( from ) );
}

VFSTask<URI> ResourceProvider::canonicalize( const URI& uri, CancellationToken ) {
	// Packs have no aliases: the canonical form is the normalized path.
	return readyValue<URI>( resourceURI( toPackPath( uri ) ) );
}

}} // namespace EE::VFS
