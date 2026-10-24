#include <eepp/vfs/providers/zipprovider.hpp>

#include <algorithm>
#include <array>
#include <eepp/core/containers.hpp>
#include <eepp/system/zip.hpp>
#include <eepp/vfs/detail/archivelocation.hpp>
#include <eepp/vfs/detail/memoryhandles.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <limits>
#include <mutex>

using namespace EE::System;

namespace EE { namespace VFS {

namespace {

constexpr ProviderCapabilities
	ZipCapabilities( static_cast<Uint32>( VFSCapability::Read ) |
					 static_cast<Uint32>( VFSCapability::List ) |
					 static_cast<Uint32>( VFSCapability::Seek ) |
					 static_cast<Uint32>( VFSCapability::CaseSensitive ) );
constexpr Uint64 NoEntry = std::numeric_limits<Uint64>::max();
constexpr size_t MaxEntries = 262144;
constexpr size_t MaxMetadata = 64 * 1024 * 1024;

VFSError readOnly( const URI& uri ) {
	return VFSError( VFSErrorCode::ReadOnly, uri, "ZIP archives are read-only" );
}

VFSError readerError( const Zip& reader, const URI& uri ) {
	VFSErrorCode code = VFSErrorCode::IO;
	switch ( reader.error() ) {
		case Zip::Error::NotFound:
			code = VFSErrorCode::NotFound;
			break;
		case Zip::Error::PermissionDenied:
			code = VFSErrorCode::PermissionDenied;
			break;
		case Zip::Error::InvalidArchive:
			code = VFSErrorCode::InvalidOperation;
			break;
		case Zip::Error::Unsupported:
			code = VFSErrorCode::Unsupported;
			break;
		default:
			break;
	}
	return VFSError( code, uri, reader.errorMessage() );
}

struct ZipNode {
	VFSFileInfo info;
	std::vector<size_t> children;
	Uint64 entry{ NoEntry };
};

/** Immutable directory index. Entry streams open independent readers, so their I/O and teardown
 * never wait for another handle on the same archive. */
struct ZipArchive {
	std::string path;
	std::vector<ZipNode> nodes;
	UnorderedMap<std::string, size_t> paths;
	FileRevision revision;
	LocalIOScheduler::Route route;
	Uint64 stamp{ 0 };

	VFSFileInfo info( std::string name, bool directory, Uint64 entry = NoEntry,
					  const Zip::Entry* metadata = nullptr ) const {
		VFSFileInfo value;
		value.name = std::move( name );
		value.type = directory ? FileType::Directory : FileType::Regular;
		value.hidden = !value.name.empty() && value.name[0] == '.';
		// Read-only is a provider capability, not the stored entry's mode. The archive API does
		// not expose Unix permissions: leave them unknown so copies use writable native defaults.
		if ( metadata ) {
			value.setModifiedTime( metadata->modified );
			if ( !directory )
				value.setSize( metadata->size );
		}
		value.revision = FileRevision::fromValues(
			{ stamp, entry, metadata ? metadata->crc : 0, metadata ? metadata->size : 0 } );
		return value;
	}

	VFSResult<void> index( Zip& reader, const URI& uri, const CancellationToken& token ) {
		const auto count = reader.entryCount();
		if ( count > MaxEntries )
			return Err(
				VFSError( VFSErrorCode::InvalidOperation, uri, "ZIP has too many entries" ) );
		const Uint64 libraryMetadata = reader.metadataSize();
		if ( libraryMetadata > MaxMetadata ) {
			return Err(
				VFSError( VFSErrorCode::InvalidOperation, uri, "ZIP metadata budget exceeded" ) );
		}
		size_t budget = MaxMetadata - static_cast<size_t>( libraryMetadata );
		nodes.push_back( { info( "/", true ), {}, NoEntry } );
		paths.emplace( "", 0 );
		Zip::Entry metadata;
		for ( Uint64 entry = 0; entry < count; ++entry ) {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( uri ) );
			if ( !reader.entry( entry, metadata ) )
				return Err( readerError( reader, uri ) );
			std::string_view name( metadata.name );
			if ( !detail::ArchiveLocation::safeEntry( name ) )
				continue;
			const bool directory = name.back() == '/';
			if ( directory )
				name.remove_suffix( 1 );
			size_t start = 0;
			size_t parent = 0;
			while ( start < name.size() ) {
				size_t end = name.find( '/', start );
				if ( end == std::string_view::npos )
					end = name.size();
				const bool last = end == name.size();
				const bool folder = !last || directory;
				std::string path( name.substr( 0, end ) );
				auto found = paths.find( path );
				size_t node;
				if ( found == paths.end() ) {
					// Include the node/name, full path-map key, child index, hash buckets and
					// allocator/vector growth slack before retaining any of those allocations.
					const size_t cost = 2 * ( sizeof( ZipNode ) + path.size() + end - start + 128 );
					if ( nodes.size() >= MaxEntries || cost > budget ) {
						return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
											  "ZIP metadata budget exceeded" ) );
					}
					budget -= cost;
					node = nodes.size();
					paths.emplace( std::move( path ), node );
					nodes[parent].children.push_back( node );
					nodes.push_back(
						{ info( std::string( name.substr( start, end - start ) ), folder ),
						  {},
						  NoEntry } );
				} else {
					node = found->second;
				}
				ZipNode& value = nodes[node];
				// A directory wins a file/directory collision so that every listed child remains
				// reachable. Duplicate file names resolve to their first central-directory entry.
				if ( folder && value.info.type != FileType::Directory ) {
					value.info = info( std::move( value.info.name ), true );
					value.entry = NoEntry;
				}
				if ( last && ( folder || value.info.type != FileType::Directory ) &&
					 value.entry == NoEntry ) {
					value.info = info( std::move( value.info.name ), folder, entry, &metadata );
					value.entry = entry;
				}
				parent = node;
				start = end + 1;
			}
		}
		return Ok();
	}

	VFSResult<size_t> find( const std::string& member, const URI& uri ) const {
		auto found = paths.find( member );
		if ( found != paths.end() ) {
			if ( uri.getPath().ends_with( '/' ) &&
				 nodes[found->second].info.type != FileType::Directory )
				return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
			return Ok( found->second );
		}
		// Distinguish a missing member from a path through a regular file.
		size_t slash = member.find( '/' );
		while ( slash != std::string::npos ) {
			auto parent = paths.find( member.substr( 0, slash ) );
			if ( parent != paths.end() && nodes[parent->second].info.type != FileType::Directory )
				return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
			slash = member.find( '/', slash + 1 );
		}
		return Err( VFSError( VFSErrorCode::NotFound, uri ) );
	}
};

class ZipFileImpl : public FileHandleImpl, public std::enable_shared_from_this<ZipFileImpl> {
	template <typename T, typename F> auto guarded( CancellationToken token, F&& fn ) {
		return [self = shared_from_this(), token = std::move( token ),
				fn = std::forward<F>( fn )]() mutable -> VFSResult<T> {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( self->mUri ) );
			std::lock_guard<std::mutex> lock( self->mMutex );
			if ( !self->mStream )
				return Err( VFSError( VFSErrorCode::InvalidOperation, self->mUri, "File closed" ) );
			return fn( *self, token );
		};
	}

  public:
	ZipFileImpl( std::shared_ptr<LocalIOScheduler> scheduler, std::shared_ptr<ZipArchive> archive,
				 std::unique_ptr<Zip> reader, std::unique_ptr<IOStream> stream, URI uri,
				 size_t node ) :
		mScheduler( std::move( scheduler ) ),
		mArchive( std::move( archive ) ),
		mReader( std::move( reader ) ),
		mStream( std::move( stream ) ),
		mUri( std::move( uri ) ),
		mNode( node ) {}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		if ( !mArchive->route.isNetworkMount() ) {
			return mScheduler->submit<size_t>(
				mArchive->route, guarded<size_t>( std::move( token ),
												  [offset, data]( ZipFileImpl& self,
																  const CancellationToken& token ) {
													  return self.readInto( offset, data, token );
												  } ) );
		}
		// A mount's read can outlive its timeout. Only the winning completion copies into data.
		return mScheduler->submitCommitted<size_t>(
			mArchive->route,
			guarded<std::vector<Uint8>>(
				std::move( token ),
				[offset, size = data.size()]( ZipFileImpl& self, const CancellationToken& token )
					-> VFSResult<std::vector<Uint8>> {
					std::vector<Uint8> buffer( size );
					auto result = self.readInto( offset, buffer, token );
					if ( !result )
						return Err( std::move( result ).error() );
					buffer.resize( result.value() );
					return Ok( std::move( buffer ) );
				} ),
			[data]( VFSResult<std::vector<Uint8>>&& result ) -> VFSResult<size_t> {
				if ( !result )
					return Err( std::move( result ).error() );
				std::copy( result.value().begin(), result.value().end(), data.begin() );
				return Ok( result.value().size() );
			} );
	}

	VFSTask<size_t> write( Uint64, std::span<const Uint8>, CancellationToken ) override {
		return readyError<size_t>( readOnly( mUri ) );
	}

	VFSTask<void> truncate( Uint64 ) override { return readyError<void>( readOnly( mUri ) ); }

	VFSTask<void> flush() override { return readyValue<void>(); }

	VFSTask<VFSFileInfo> stat() override {
		return readyValue<VFSFileInfo>( mArchive->nodes[mNode].info );
	}

	VFSTask<void> close() override {
		return mScheduler->submit<void>( mArchive->route,
										 [self = shared_from_this()]() -> VFSResult<void> {
											 std::lock_guard<std::mutex> lock( self->mMutex );
											 self->mStream.reset();
											 self->mReader.reset();
											 return Ok();
										 } );
	}

	ProviderCapabilities capabilities() const override { return ZipCapabilities; }

  private:
	VFSResult<size_t> readInto( Uint64 offset, std::span<Uint8> data,
								const CancellationToken& token ) {
		const auto& node = mArchive->nodes[mNode];
		if ( offset >= node.info.size().value_or( 0 ) )
			return Ok( size_t( 0 ) );
		if ( offset < static_cast<Uint64>( mStream->tell() ) ) {
			auto stream = mReader->openStream( node.entry );
			if ( !stream )
				return Err( readerError( *mReader, mUri ) );
			mStream = std::move( stream );
		}
		std::array<char, 16384> scratch;
		while ( static_cast<Uint64>( mStream->tell() ) < offset ) {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( mUri ) );
			const auto count = std::min<Uint64>( offset - mStream->tell(), scratch.size() );
			if ( mStream->read( scratch.data(), count ) <= 0 )
				return Err( VFSError( VFSErrorCode::IO, mUri, "Could not seek ZIP entry" ) );
		}
		size_t done = 0;
		while ( done < data.size() ) {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( mUri ) );
			const auto count = std::min<size_t>( data.size() - done, 65536 );
			const auto got = mStream->read( reinterpret_cast<char*>( data.data() + done ), count );
			if ( got < 0 )
				return Err( VFSError( VFSErrorCode::IO, mUri,
									  "Could not read ZIP entry (corrupt data or CRC)" ) );
			if ( got == 0 )
				break;
			done += static_cast<size_t>( got );
		}
		return Ok( done );
	}

	std::shared_ptr<LocalIOScheduler> mScheduler;
	// Streams borrow the reader and are destroyed before it.
	std::shared_ptr<ZipArchive> mArchive;
	std::unique_ptr<Zip> mReader;
	std::unique_ptr<IOStream> mStream;
	std::mutex mMutex;
	URI mUri;
	size_t mNode;
};

} // namespace

class ZipProviderImpl : public std::enable_shared_from_this<ZipProviderImpl> {
  public:
	explicit ZipProviderImpl( std::shared_ptr<LocalIOScheduler> scheduler ) :
		scheduler( std::move( scheduler ) ) {}

	template <typename T, typename F>
	VFSTask<T> run( const URI& uri, CancellationToken token, F&& fn ) {
		if ( token.isCancellationRequested() )
			return readyError<T>( cancelledError( uri ) );
		auto address = detail::ArchiveLocation::parse( uri, "zip" );
		if ( !address )
			return readyError<T>( std::move( address ).error() );
		auto native = LocalProvider::toNativePath( address.value().archive );
		if ( !native )
			return readyError<T>( std::move( native ).error() );
		std::string member = std::move( address ).value().member;
		while ( !member.empty() && member.front() == '/' )
			member.erase( member.begin() );
		while ( !member.empty() && member.back() == '/' )
			member.pop_back();
		std::string path = std::move( native ).value();
		return scheduler->submitResolved<T>(
			path, true,
			[self = shared_from_this(), uri, path, member = std::move( member ),
			 token = std::move( token ), fn = std::forward<F>( fn )]() mutable -> VFSResult<T> {
				if ( token.isCancellationRequested() )
					return Err( cancelledError( uri ) );
				auto archive = self->load( path, uri, token );
				if ( !archive )
					return Err( std::move( archive ).error() );
				return fn( std::move( archive ).value(), member, uri );
			} );
	}

	std::shared_ptr<LocalIOScheduler> scheduler;

  private:
	struct Cached {
		std::string path;
		std::shared_ptr<ZipArchive> archive;
		Uint64 used{ 0 };
	};

	VFSResult<std::shared_ptr<ZipArchive>> load( const std::string& path, const URI& uri,
												 const CancellationToken& token ) {
		const URI backing = URI::fromFSPath( path );
		auto metadata = LocalProvider::statSync( backing, path, true );
		if ( !metadata )
			return Err( std::move( metadata ).error() );
		if ( !metadata.value().isRegularFile() )
			return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
								  "ZIP backing file is not a regular file" ) );
		const auto revision = metadata.value().revision;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			for ( auto& cached : mCache ) {
				if ( cached.path == path && cached.archive &&
					 cached.archive->revision == revision ) {
					cached.used = ++mUsed;
					return Ok( cached.archive );
				}
			}
		}
		auto archive = std::make_shared<ZipArchive>();
		archive->revision = revision;
		archive->path = path;
		archive->stamp = std::hash<std::string_view>{}(
			std::string_view( reinterpret_cast<const char*>( revision.data() ), revision.size() ) );
		archive->route = scheduler->resolvedRouteFor( path, true );
		Zip reader( false );
		if ( !reader.open( path, Zip::OpenMode::ReadOnlyBounded ) )
			return Err( readerError( reader, uri ) );
		auto indexed = archive->index( reader, uri, token );
		if ( !indexed )
			return Err( std::move( indexed ).error() );
		auto after = LocalProvider::statSync( backing, path, true );
		if ( !after || after.value().revision != revision )
			return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
								  "Archive changed while it was opened" ) );
		std::shared_ptr<ZipArchive> retired;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto slot = mCache.begin();
			for ( auto it = mCache.begin(); it != mCache.end(); ++it ) {
				if ( it->path == path ) {
					slot = it;
					break;
				}
				if ( it->used < slot->used )
					slot = it;
			}
			retired = std::move( slot->archive );
			slot->path = path;
			slot->archive = archive;
			slot->used = ++mUsed;
		}
		// Free a retired directory index outside the cache lock.
		return Ok( std::move( archive ) );
	}

	std::mutex mMutex;
	std::array<Cached, 8> mCache;
	Uint64 mUsed{ 0 };
};

ZipProvider::ZipProvider( std::shared_ptr<LocalIOScheduler> scheduler ) :
	mImpl( std::make_shared<ZipProviderImpl>( std::move( scheduler ) ) ) {}

ZipProvider::~ZipProvider() = default;

URI ZipProvider::makeURI( const URI& archive, std::string_view member ) {
	return detail::ArchiveLocation::make( "zip", archive, member );
}

VFSResult<URI> ZipProvider::archiveURI( const URI& uri ) {
	auto address = detail::ArchiveLocation::parse( uri, "zip" );
	if ( !address )
		return Err( std::move( address ).error() );
	return Ok( std::move( address ).value().archive );
}

VFSResult<std::string> ZipProvider::memberPath( const URI& uri ) {
	auto address = detail::ArchiveLocation::parse( uri, "zip" );
	if ( !address )
		return Err( std::move( address ).error() );
	return Ok( std::move( address ).value().member );
}

bool ZipProvider::relocateURI( URI& uri, const URI& from, const URI& to ) const {
	auto address = detail::ArchiveLocation::parse( uri, scheme() );
	if ( !address || !VFSProvider::relocateURI( address.value().archive, from, to ) )
		return false;
	uri = makeURI( address.value().archive, address.value().member );
	return true;
}

ProviderCapabilities ZipProvider::capabilities( const URI& ) const {
	return ZipCapabilities;
}

VFSTask<VFSFileInfo> ZipProvider::stat( const URI& uri, const StatOptions&,
										CancellationToken token ) {
	return mImpl->run<VFSFileInfo>( uri, std::move( token ),
									[]( const std::shared_ptr<ZipArchive>& archive,
										const std::string& member,
										const URI& uri ) -> VFSResult<VFSFileInfo> {
										auto node = archive->find( member, uri );
										if ( !node )
											return Err( std::move( node ).error() );
										return Ok( archive->nodes[node.value()].info );
									} );
}

VFSTask<DirectoryHandle> ZipProvider::openDirectory( const URI& uri, const DirectoryOpenOptions&,
													 CancellationToken token ) {
	return mImpl->run<DirectoryHandle>(
		uri, std::move( token ),
		[]( const std::shared_ptr<ZipArchive>& archive, const std::string& member,
			const URI& uri ) -> VFSResult<DirectoryHandle> {
			auto node = archive->find( member, uri );
			if ( !node )
				return Err( std::move( node ).error() );
			const auto& folder = archive->nodes[node.value()];
			if ( folder.info.type != FileType::Directory )
				return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
			std::vector<VFSFileInfo> entries;
			entries.reserve( folder.children.size() );
			for ( auto child : folder.children )
				entries.push_back( archive->nodes[child].info );
			return Ok( DirectoryHandle(
				uri, std::make_shared<detail::VectorDirectoryImpl>( std::move( entries ) ) ) );
		} );
}

VFSTask<FileHandle> ZipProvider::open( const URI& uri, const OpenOptions& options,
									   CancellationToken token ) {
	if ( hasFlag( options.flags, OpenFlags::Write ) ||
		 hasFlag( options.flags, OpenFlags::Create ) ||
		 hasFlag( options.flags, OpenFlags::Truncate ) )
		return readyError<FileHandle>( readOnly( uri ) );
	return mImpl->run<FileHandle>(
		uri, std::move( token ),
		[scheduler = mImpl->scheduler]( std::shared_ptr<ZipArchive> archive,
										const std::string& member,
										const URI& uri ) -> VFSResult<FileHandle> {
			auto node = archive->find( member, uri );
			if ( !node )
				return Err( std::move( node ).error() );
			const auto& info = archive->nodes[node.value()];
			if ( info.info.type == FileType::Directory )
				return Err( VFSError( VFSErrorCode::IsDirectory, uri ) );
			auto reader = std::make_unique<Zip>( false );
			if ( !reader->open( archive->path, Zip::OpenMode::ReadOnlyBounded ) )
				return Err( readerError( *reader, uri ) );
			auto current =
				LocalProvider::statSync( URI::fromFSPath( archive->path ), archive->path, true );
			if ( !current || current.value().revision != archive->revision )
				return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
									  "Archive changed while opening the entry" ) );
			auto stream = reader->openStream( info.entry );
			if ( !stream )
				return Err( readerError( *reader, uri ) );
			return Ok( FileHandle( uri, std::make_shared<ZipFileImpl>(
											scheduler, std::move( archive ), std::move( reader ),
											std::move( stream ), uri, node.value() ) ) );
		} );
}

VFSTask<void> ZipProvider::mkdir( const URI& uri, const MkDirOptions&, CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<void> ZipProvider::remove( const URI& uri, const RemoveOptions&, CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<void> ZipProvider::rename( const URI& from, const URI&, const RenameOptions&,
								   CancellationToken ) {
	return readyError<void>( readOnly( from ) );
}

VFSTask<URI> ZipProvider::canonicalize( const URI& uri, CancellationToken token ) {
	return mImpl->run<URI>( uri, std::move( token ),
							[]( const std::shared_ptr<ZipArchive>& archive,
								const std::string& member, const URI& uri ) -> VFSResult<URI> {
								auto found = archive->find( member, uri );
								if ( !found )
									return Err( std::move( found ).error() );
								auto backing = ZipProvider::archiveURI( uri );
								URI normalized = ZipProvider::makeURI( backing.value(), member );
								return Ok( std::move( normalized ) );
							} );
}

VFSTask<bool> ZipProvider::sameFileSystem( const URI& from, const URI& to,
										   CancellationToken token ) {
	if ( token.isCancellationRequested() )
		return readyError<bool>( cancelledError( from ) );
	if ( !VFSPath::validURI( from ) || !VFSPath::validURI( to ) )
		return readyError<bool>( VFSErrorCode::InvalidURI, to, "NUL in provider URI" );
	// Canonical URI paths include the backing archive path and the member path. Different
	// archive files remain comparable within one authority; UNC host aliases are uncertain.
	if ( from.hasSameAuthority( to ) )
		return readyValue<bool>( true );
	return readyError<bool>( VFSErrorCode::Unsupported, to,
							 "Cannot determine archive identity across provider authorities" );
}

}} // namespace EE::VFS
