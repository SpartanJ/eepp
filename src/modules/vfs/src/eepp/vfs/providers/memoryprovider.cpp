#include <cstring>
#include <eepp/core/string.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/detail/memoryhandles.hpp>
#include <eepp/vfs/providers/memoryprovider.hpp>

namespace EE { namespace VFS {

struct MemoryProvider::Node {
	std::mutex mutex; // guards data/revision of files shared with open handles
	std::vector<Uint8> data;
	Int64 modified{ 0 };
	Uint64 revision{ 0 };
	Uint32 permissions{ 0644 };
	bool directory{ false };
};

namespace {

constexpr ProviderCapabilities MemoryCapabilities(
	static_cast<Uint32>( VFSCapability::Read ) | static_cast<Uint32>( VFSCapability::Write ) |
	static_cast<Uint32>( VFSCapability::List ) | static_cast<Uint32>( VFSCapability::Create ) |
	static_cast<Uint32>( VFSCapability::Remove ) | static_cast<Uint32>( VFSCapability::Rename ) |
	static_cast<Uint32>( VFSCapability::Seek ) |
	static_cast<Uint32>( VFSCapability::RandomAccess ) |
	static_cast<Uint32>( VFSCapability::Truncate ) |
	static_cast<Uint32>( VFSCapability::Permissions ) |
	static_cast<Uint32>( VFSCapability::AtomicRename ) |
	static_cast<Uint32>( VFSCapability::AtomicReplace ) |
	static_cast<Uint32>( VFSCapability::Fsync ) |
	static_cast<Uint32>( VFSCapability::CaseSensitive ) );

std::atomic<Uint64> sRevisionCounter{ 0 };

Uint64 nextRevision() {
	return ++sRevisionCounter;
}

std::string parentKey( const std::string& key ) {
	size_t slash = key.find_last_of( '/' );
	size_t authorityEnd = key.find( '/' );
	if ( slash == std::string::npos || slash <= authorityEnd )
		return key.substr( 0, authorityEnd == std::string::npos ? key.size() : authorityEnd ) + "/";
	return key.substr( 0, slash );
}

bool isRootKey( const std::string& key ) {
	return !key.empty() && key.back() == '/';
}

/** Writable file handle sharing the node with the provider. */
class MemoryFileImpl : public FileHandleImpl {
  public:
	MemoryFileImpl( std::shared_ptr<MemoryProvider::Node> node, URI uri, bool readable,
					bool writable ) :
		mNode( std::move( node ) ),
		mUri( std::move( uri ) ),
		mReadable( readable ),
		mWritable( writable ) {}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		if ( token.isCancellationRequested() )
			return readyError<size_t>( cancelledError( mUri ) );
		if ( !mReadable )
			return readyError<size_t>( VFSErrorCode::PermissionDenied, mUri, "Not readable" );
		std::lock_guard<std::mutex> lock( mNode->mutex );
		if ( offset >= mNode->data.size() )
			return readyValue<size_t>( 0 );
		size_t count = std::min<size_t>( data.size(), mNode->data.size() - offset );
		std::memcpy( data.data(), mNode->data.data() + offset, count );
		return readyValue<size_t>( count );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token ) override {
		if ( token.isCancellationRequested() )
			return readyError<size_t>( cancelledError( mUri ) );
		if ( !mWritable )
			return readyError<size_t>( VFSErrorCode::PermissionDenied, mUri, "Not writable" );
		std::lock_guard<std::mutex> lock( mNode->mutex );
		if ( offset + data.size() > mNode->data.size() )
			mNode->data.resize( offset + data.size() );
		if ( !data.empty() )
			std::memcpy( mNode->data.data() + offset, data.data(), data.size() );
		touch();
		return readyValue<size_t>( data.size() );
	}

	VFSTask<void> truncate( Uint64 size ) override {
		if ( !mWritable )
			return readyError<void>( VFSErrorCode::PermissionDenied, mUri, "Not writable" );
		std::lock_guard<std::mutex> lock( mNode->mutex );
		mNode->data.resize( size );
		touch();
		return readyValue<void>();
	}

	VFSTask<void> flush() override { return readyValue<void>(); }

	VFSTask<FileInfo> stat() override {
		FileInfo info;
		info.name = mUri.getLastPathSegment();
		info.type = FileType::Regular;
		std::lock_guard<std::mutex> lock( mNode->mutex );
		info.setSize( mNode->data.size() );
		info.setModifiedTime( mNode->modified );
		info.setPermissions( mNode->permissions );
		info.revision = FileRevision::fromValues( { mNode->revision } );
		return readyValue<FileInfo>( std::move( info ) );
	}

	VFSTask<void> close() override { return readyValue<void>(); }

	ProviderCapabilities capabilities() const override { return MemoryCapabilities; }

  private:
	void touch() {
		mNode->modified = System::Sys::getUnixTimestamp();
		mNode->revision = nextRevision();
	}

	std::shared_ptr<MemoryProvider::Node> mNode;
	URI mUri;
	bool mReadable;
	bool mWritable;
};

} // namespace

MemoryProvider::MemoryProvider( std::string scheme, Executor* completionExecutor ) :
	mScheme( std::move( scheme ) ), mExecutor( completionExecutor ) {}

MemoryProvider::~MemoryProvider() = default;

ProviderCapabilities MemoryProvider::capabilities( const URI& ) const {
	return MemoryCapabilities;
}

template <typename T> VFSTask<T> MemoryProvider::complete( VFSResult<T>&& result ) {
	if ( !mExecutor )
		return makeReadyTask<VFSResult<T>>( std::move( result ) );
	TaskSource<VFSResult<T>> source;
	auto task = source.task();
	mExecutor->execute( [source = std::move( source ),
						 result = std::make_unique<VFSResult<T>>( std::move( result ) )]() mutable {
		source.resolve( std::move( *result ) );
	} );
	return task;
}

std::string MemoryProvider::key( const URI& uri ) const {
	// "authority/path" without trailing slash; the root of an authority is "authority/".
	std::string key( uri.getHost() );
	std::string_view path( uri.getPath() );
	while ( path.size() > 1 && path.back() == '/' )
		path.remove_suffix( 1 );
	if ( path.empty() || path == "/" ) {
		key += '/';
		return key;
	}
	if ( path.front() != '/' )
		key += '/';
	key.append( path );
	return key;
}

std::shared_ptr<MemoryProvider::Node> MemoryProvider::find( const std::string& key ) const {
	if ( isRootKey( key ) ) {
		static auto root = [] {
			auto node = std::make_shared<Node>();
			node->directory = true;
			node->permissions = 0755;
			return node;
		}();
		return root;
	}
	auto it = mNodes.find( key );
	return it != mNodes.end() ? it->second : nullptr;
}

FileInfo MemoryProvider::describe( const URI& uri, const std::string& key,
								   const Node& node ) const {
	FileInfo info;
	size_t slash = key.find_last_of( '/' );
	info.name = isRootKey( key ) ? std::string( "/" ) : key.substr( slash + 1 );
	info.hidden = !info.name.empty() && info.name[0] == '.';
	info.type = node.directory ? FileType::Directory : FileType::Regular;
	std::lock_guard<std::mutex> lock( const_cast<Node&>( node ).mutex );
	if ( !node.directory )
		info.setSize( node.data.size() );
	info.setModifiedTime( node.modified );
	info.setPermissions( node.permissions );
	info.revision = FileRevision::fromValues( { node.revision } );
	return info;
}

VFSResult<std::shared_ptr<MemoryProvider::Node>> MemoryProvider::createNode( const URI& uri,
																			 const std::string& key,
																			 bool directory,
																			 Uint32 permissions ) {
	if ( isRootKey( key ) )
		return Err( VFSError( VFSErrorCode::AlreadyExists, uri ) );
	if ( mNodes.count( key ) )
		return Err( VFSError( VFSErrorCode::AlreadyExists, uri ) );
	auto parent = find( parentKey( key ) );
	if ( !parent )
		return Err( VFSError( VFSErrorCode::NotFound, uri, "Parent does not exist" ) );
	if ( !parent->directory )
		return Err( VFSError( VFSErrorCode::NotDirectory, uri, "Parent is not a directory" ) );
	auto node = std::make_shared<Node>();
	node->directory = directory;
	node->permissions = permissions;
	node->modified = System::Sys::getUnixTimestamp();
	node->revision = nextRevision();
	mNodes[key] = node;
	return Ok( std::move( node ) );
}

VFSTask<FileInfo> MemoryProvider::stat( const URI& uri, const StatOptions&,
										CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return complete<FileInfo>( Err( cancelledError( uri ) ) );
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	std::string k = key( uri );
	auto node = find( k );
	if ( !node )
		return complete<FileInfo>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) );
	return complete<FileInfo>( Ok( describe( uri, k, *node ) ) );
}

VFSTask<DirectoryHandle> MemoryProvider::openDirectory( const URI& uri, const DirectoryOpenOptions&,
														CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return complete<DirectoryHandle>( Err( cancelledError( uri ) ) );
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	std::string k = key( uri );
	auto node = find( k );
	if ( !node )
		return complete<DirectoryHandle>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) );
	if ( !node->directory )
		return complete<DirectoryHandle>( Err( VFSError( VFSErrorCode::NotDirectory, uri ) ) );
	std::string prefix = isRootKey( k ) ? k : k + "/";
	std::vector<FileInfo> entries;
	for ( auto it = mNodes.lower_bound( prefix );
		  it != mNodes.end() && String::startsWith( it->first, prefix ); ++it ) {
		std::string_view rest( it->first );
		rest.remove_prefix( prefix.size() );
		if ( rest.empty() || rest.find( '/' ) != std::string_view::npos )
			continue;
		entries.push_back( describe( uri.withAppendedSegment( rest ), it->first, *it->second ) );
	}
	return complete<DirectoryHandle>( Ok( DirectoryHandle(
		uri, std::make_shared<detail::VectorDirectoryImpl>( std::move( entries ) ) ) ) );
}

VFSTask<FileHandle> MemoryProvider::open( const URI& uri, const OpenOptions& options,
										  CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return complete<FileHandle>( Err( cancelledError( uri ) ) );
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	std::string k = key( uri );
	auto node = find( k );
	OpenFlags flags = options.flags;
	if ( node ) {
		if ( hasFlag( flags, OpenFlags::Create ) && hasFlag( flags, OpenFlags::Exclusive ) )
			return complete<FileHandle>( Err( VFSError( VFSErrorCode::AlreadyExists, uri ) ) );
		if ( node->directory )
			return complete<FileHandle>( Err( VFSError( VFSErrorCode::IsDirectory, uri ) ) );
		if ( hasFlag( flags, OpenFlags::Truncate ) ) {
			std::lock_guard<std::mutex> nodeLock( node->mutex );
			node->data.clear();
			node->revision = nextRevision();
		}
	} else {
		if ( !hasFlag( flags, OpenFlags::Create ) )
			return complete<FileHandle>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) );
		auto created = createNode( uri, k, false, options.createPermissions.value_or( 0644 ) );
		if ( !created )
			return complete<FileHandle>( Err( std::move( created ).error() ) );
		node = std::move( created ).value();
	}
	return complete<FileHandle>( Ok( FileHandle(
		uri, std::make_shared<MemoryFileImpl>( node, uri, hasFlag( flags, OpenFlags::Read ),
											   hasFlag( flags, OpenFlags::Write ) ) ) ) );
}

VFSTask<void> MemoryProvider::mkdir( const URI& uri, const MkDirOptions& options,
									 CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return complete<void>( Err( cancelledError( uri ) ) );
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	auto created = createNode( uri, key( uri ), true, options.permissions.value_or( 0755 ) );
	if ( !created )
		return complete<void>( Err( std::move( created ).error() ) );
	return complete<void>( Ok() );
}

VFSTask<void> MemoryProvider::remove( const URI& uri, const RemoveOptions&,
									  CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return complete<void>( Err( cancelledError( uri ) ) );
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	std::string k = key( uri );
	if ( isRootKey( k ) )
		return complete<void>( Err( VFSError( VFSErrorCode::PermissionDenied, uri ) ) );
	auto it = mNodes.find( k );
	if ( it == mNodes.end() )
		return complete<void>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) );
	if ( it->second->directory ) {
		auto child = mNodes.lower_bound( k + "/" );
		if ( child != mNodes.end() && String::startsWith( child->first, k + "/" ) )
			return complete<void>( Err( VFSError( VFSErrorCode::NotEmpty, uri ) ) );
	}
	mNodes.erase( it );
	return complete<void>( Ok() );
}

VFSTask<void> MemoryProvider::rename( const URI& from, const URI& to, const RenameOptions& options,
									  CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return complete<void>( Err( cancelledError( from ) ) );
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	std::string src = key( from );
	std::string dst = key( to );
	if ( from.getHost() != to.getHost() )
		return complete<void>(
			Err( VFSError( VFSErrorCode::Unsupported, from, "Different namespaces" ) ) );
	auto srcIt = mNodes.find( src );
	if ( srcIt == mNodes.end() )
		return complete<void>( Err( VFSError( VFSErrorCode::NotFound, from ) ) );
	if ( src == dst )
		return complete<void>( Ok() );
	if ( String::startsWith( dst, src + "/" ) )
		return complete<void>(
			Err( VFSError( VFSErrorCode::InvalidOperation, to, "Destination inside source" ) ) );
	auto parent = find( parentKey( dst ) );
	if ( !parent || !parent->directory )
		return complete<void>( Err( VFSError( VFSErrorCode::NotFound, to ) ) );
	auto dstIt = mNodes.find( dst );
	bool srcIsDir = srcIt->second->directory;
	if ( dstIt != mNodes.end() ) {
		if ( !options.overwrite )
			return complete<void>( Err( VFSError( VFSErrorCode::AlreadyExists, to ) ) );
		if ( dstIt->second->directory != srcIsDir )
			return complete<void>( Err( VFSError(
				srcIsDir ? VFSErrorCode::NotDirectory : VFSErrorCode::IsDirectory, to ) ) );
		if ( dstIt->second->directory ) {
			auto child = mNodes.lower_bound( dst + "/" );
			if ( child != mNodes.end() && String::startsWith( child->first, dst + "/" ) )
				return complete<void>( Err( VFSError( VFSErrorCode::NotEmpty, to ) ) );
		}
		mNodes.erase( dstIt );
	}
	// Move the node and, for directories, its whole subtree.
	std::vector<std::pair<std::string, std::shared_ptr<Node>>> moved;
	moved.emplace_back( dst, srcIt->second );
	mNodes.erase( srcIt );
	if ( srcIsDir ) {
		std::string prefix = src + "/";
		for ( auto it = mNodes.lower_bound( prefix );
			  it != mNodes.end() && String::startsWith( it->first, prefix ); ) {
			moved.emplace_back( dst + "/" + it->first.substr( prefix.size() ), it->second );
			it = mNodes.erase( it );
		}
	}
	for ( auto& entry : moved )
		mNodes[std::move( entry.first )] = std::move( entry.second );
	return complete<void>( Ok() );
}

VFSTask<URI> MemoryProvider::canonicalize( const URI& uri, CancellationToken ) {
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	std::string k = key( uri );
	if ( !find( k ) )
		return complete<URI>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) );
	URI canonical;
	canonical.setScheme( mScheme );
	canonical.setHost( uri.getHost() );
	canonical.setRawPath( k.substr( k.find( '/' ) ) );
	return complete<URI>( Ok( std::move( canonical ) ) );
}

VFSTask<void> MemoryProvider::setPermissions( const URI& uri, Uint32 permissions,
											  CancellationToken ) {
	std::lock_guard<std::mutex> lock( mMutex );
	++mOperations;
	auto node = find( key( uri ) );
	if ( !node || isRootKey( key( uri ) ) )
		return complete<void>( Err( VFSError( VFSErrorCode::NotFound, uri ) ) );
	std::lock_guard<std::mutex> nodeLock( node->mutex );
	node->permissions = permissions & 07777;
	return complete<void>( Ok() );
}

void MemoryProvider::putDirectory( const URI& uri ) {
	std::lock_guard<std::mutex> lock( mMutex );
	std::string k = key( uri );
	std::vector<std::string> chain;
	while ( !isRootKey( k ) && !mNodes.count( k ) ) {
		chain.push_back( k );
		k = parentKey( k );
	}
	for ( auto it = chain.rbegin(); it != chain.rend(); ++it ) {
		auto node = std::make_shared<Node>();
		node->directory = true;
		node->permissions = 0755;
		node->revision = nextRevision();
		mNodes[*it] = std::move( node );
	}
}

void MemoryProvider::putFile( const URI& uri, std::string_view content, Uint32 permissions ) {
	putDirectory( uri.getParent() );
	std::lock_guard<std::mutex> lock( mMutex );
	auto node = std::make_shared<Node>();
	node->data.assign( content.begin(), content.end() );
	node->permissions = permissions;
	node->modified = System::Sys::getUnixTimestamp();
	node->revision = nextRevision();
	mNodes[key( uri )] = std::move( node );
}

std::optional<std::string> MemoryProvider::fileContent( const URI& uri ) const {
	std::lock_guard<std::mutex> lock( mMutex );
	auto node = find( key( uri ) );
	if ( !node || node->directory )
		return std::nullopt;
	std::lock_guard<std::mutex> nodeLock( node->mutex );
	return std::string( node->data.begin(), node->data.end() );
}

Uint64 MemoryProvider::operationCount() const {
	std::lock_guard<std::mutex> lock( mMutex );
	return mOperations;
}

}} // namespace EE::VFS
