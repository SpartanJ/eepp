#include <eepp/vfs/providers/archiveprovider.hpp>

#include <algorithm>
#include <array>
#include <eepp/core/containers.hpp>
#include <eepp/vfs/archive/libarchive.hpp>
#include <eepp/vfs/detail/archivelocation.hpp>
#include <eepp/vfs/detail/memoryhandles.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <limits>
#include <mutex>

namespace EE { namespace VFS {

using detail::ArchiveLocation;
using detail::LibArchive;
using detail::LibArchiveReader;

namespace {
constexpr ProviderCapabilities ArchiveCapabilities =
	VFSCapability::Read | VFSCapability::List | VFSCapability::Seek | VFSCapability::CaseSensitive;
constexpr size_t MaxEntries = 262144;
constexpr size_t MaxMetadata = 64 * 1024 * 1024;
constexpr Uint64 NoEntry = std::numeric_limits<Uint64>::max();

VFSError readOnly( const URI& uri ) {
	return VFSError( VFSErrorCode::ReadOnly, uri, "Archives are read-only" );
}

struct ArchiveNode {
	VFSFileInfo info;
	std::vector<size_t> children;
	std::string hardLink;
	Uint64 entry = NoEntry;
};

/** Immutable index; readers are independent and retain the library and revision they opened. */
struct ArchiveIndex {
	std::string path;
	std::vector<ArchiveNode> nodes;
	UnorderedMap<std::string, size_t> paths;
	std::shared_ptr<LibArchive> library;
	FileRevision revision;
	LocalIOScheduler::Route route;

	struct IdleReader {
		std::unique_ptr<LibArchiveReader> reader;
		Uint64 entry = NoEntry;
	};

	IdleReader takeReader( Uint64 entry ) {
		std::lock_guard<std::mutex> lock( mReaderMutex );
		if ( mIdle.reader && mIdle.entry < entry )
			return std::move( mIdle );
		return {};
	}

	void recycleReader( std::unique_ptr<LibArchiveReader> reader, Uint64 entry ) {
		if ( !reader )
			return;
		IdleReader retired;
		{
			std::lock_guard<std::mutex> lock( mReaderMutex );
			retired = std::move( mIdle );
			mIdle = { std::move( reader ), entry };
		}
		// Native reader teardown never holds the index lock. At most one idle reader is kept,
		// allowing sequential copies to advance without replaying each entry's archive prefix.
	}

	VFSResult<size_t> find( std::string member, const URI& uri, bool follow = true,
							std::string* resolved = nullptr ) const {
		// Resolve links only within the archive. URI normalization clamps parent components at
		// the member root; no target is ever passed to a native filesystem API.
		for ( size_t links = 0; links <= 40; ++links ) {
			size_t start = 0;
			bool redirected = false;
			while ( start < member.size() ) {
				size_t end = member.find( '/', start );
				if ( end == std::string::npos )
					end = member.size();
				auto found = paths.find( member.substr( 0, end ) );
				if ( found == paths.end() )
					return Err( VFSError( VFSErrorCode::NotFound, uri ) );
				const auto& node = nodes[found->second];
				if ( ( node.info.isSymlink() && ( follow || end < member.size() ) ) ||
					 !node.hardLink.empty() ) {
					const std::string& target =
						node.hardLink.empty() ? node.info.symlinkTarget : node.hardLink;
					if ( target.empty() || target.size() > 4096 ||
						 target.find_first_of( "\\:" ) != std::string::npos )
						return Err(
							VFSError( VFSErrorCode::Unsupported, uri, "Invalid archive link" ) );
					std::string path;
					if ( !node.hardLink.empty() || target.front() == '/' )
						path = "/" + target;
					else
						path = "/" + member.substr( 0, start ) + target;
					path += member.substr( end );
					URI normalized;
					normalized.setRawPath( std::move( path ) );
					normalized.normalize();
					member = normalized.getPath();
					while ( !member.empty() && member.front() == '/' )
						member.erase( member.begin() );
					redirected = true;
					break;
				}
				if ( end < member.size() && !node.info.isDirectory() )
					return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
				start = end + 1;
			}
			if ( redirected )
				continue;
			auto found = paths.find( member );
			if ( found == paths.end() )
				return Err( VFSError( VFSErrorCode::NotFound, uri ) );
			if ( uri.getPath().ends_with( '/' ) && !nodes[found->second].info.isDirectory() )
				return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
			if ( resolved )
				*resolved = std::move( member );
			return Ok( found->second );
		}
		return Err( VFSError( VFSErrorCode::TooManySymlinks, uri ) );
	}

	VFSResult<void> index( LibArchiveReader& reader, const URI& uri, CancellationToken token ) {
		VFSFileInfo root;
		root.name = "/";
		root.type = FileType::Directory;
		root.revision = revision;
		nodes.push_back( { std::move( root ), {}, {}, NoEntry } );
		paths.emplace( "", 0 );
		size_t budget = MaxMetadata;
		for ( Uint64 ordinal = 0;; ++ordinal ) {
			auto next = reader.next( token );
			if ( !next )
				return Err( std::move( next ).error() );
			if ( !next.value() )
				break;
			if ( ordinal >= MaxEntries )
				return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
									  "Archive has too many entries" ) );
			const char* raw = reader.path();
			if ( !raw )
				return Err( VFSError( VFSErrorCode::Unsupported, uri,
									  "Archive filename cannot be decoded as UTF-8" ) );
			std::string_view name( raw );
			while ( name.starts_with( "./" ) )
				name.remove_prefix( 2 );
			if ( name == "." || name.empty() )
				continue;
			if ( !ArchiveLocation::safeEntry( name ) )
				continue;
			if ( name.ends_with( '/' ) )
				name.remove_suffix( 1 );
			auto info = reader.info();
			if ( !info )
				return Err( std::move( info ).error() );
			VFSFileInfo metadata = std::move( info ).value();
			const char* hardLink = reader.hardLink();
			if ( metadata.symlinkTarget.size() > 4096 ||
				 ( hardLink && std::char_traits<char>::length( hardLink ) > 4096 ) )
				return Err(
					VFSError( VFSErrorCode::InvalidOperation, uri, "Archive link is too long" ) );
			size_t start = 0, parent = 0;
			while ( start < name.size() ) {
				size_t end = name.find( '/', start );
				if ( end == std::string_view::npos )
					end = name.size();
				bool last = end == name.size();
				bool folder = !last || metadata.isDirectory();
				std::string key( name.substr( 0, end ) );
				auto found = paths.find( key );
				size_t node;
				if ( found == paths.end() ) {
					const size_t cost =
						2 * ( sizeof( ArchiveNode ) + key.size() + end - start + 128 );
					if ( nodes.size() >= MaxEntries || cost > budget )
						return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
											  "Archive metadata budget exceeded" ) );
					budget -= cost;
					node = nodes.size();
					paths.emplace( std::move( key ), node );
					nodes[parent].children.push_back( node );
					VFSFileInfo info;
					info.name = std::string( name.substr( start, end - start ) );
					info.type = folder ? FileType::Directory : FileType::Unknown;
					info.hidden = info.name.starts_with( '.' );
					info.revision = revision;
					nodes.push_back( { std::move( info ), {}, {}, NoEntry } );
				} else {
					node = found->second;
				}
				auto& value = nodes[node];
				if ( folder && !value.info.isDirectory() ) {
					VFSFileInfo directory;
					directory.name = std::move( value.info.name );
					directory.hidden = value.info.hidden;
					directory.type = FileType::Directory;
					directory.revision = revision;
					value.info = std::move( directory );
					value.entry = NoEntry;
					value.hardLink.clear();
				}
				// As with ZIP, directories win collisions and duplicate files keep the first.
				if ( last && ( folder || !value.info.isDirectory() ) && value.entry == NoEntry ) {
					size_t cost =
						2 * ( metadata.symlinkTarget.size() +
							  ( hardLink ? std::char_traits<char>::length( hardLink ) : 0 ) );
					if ( cost > budget )
						return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
											  "Archive metadata budget exceeded" ) );
					budget -= cost;
					metadata.name = std::move( value.info.name );
					metadata.hidden = metadata.name.starts_with( '.' );
					metadata.revision = FileRevision::fromValues(
						{ std::hash<std::string_view>{}( std::string_view(
							  reinterpret_cast<const char*>( revision.data() ), revision.size() ) ),
						  ordinal } );
					value.info = std::move( metadata );
					value.entry = ordinal;
					if ( hardLink )
						value.hardLink = hardLink;
				}
				parent = node;
				start = end + 1;
			}
		}
		return Ok();
	}

  private:
	std::mutex mReaderMutex;
	IdleReader mIdle;
};

class ArchiveFile : public FileHandleImpl, public std::enable_shared_from_this<ArchiveFile> {
  public:
	ArchiveFile( std::shared_ptr<LocalIOScheduler> scheduler, std::shared_ptr<ArchiveIndex> index,
				 std::unique_ptr<LibArchiveReader> reader, size_t node, URI uri ) :
		mScheduler( std::move( scheduler ) ),
		mIndex( std::move( index ) ),
		mReader( std::move( reader ) ),
		mUri( std::move( uri ) ),
		mNode( static_cast<Uint32>( node ) ) {}

	static VFSResult<std::unique_ptr<LibArchiveReader>> openReader( ArchiveIndex& index,
																	size_t node, const URI& uri,
																	CancellationToken token,
																	bool reuse = true ) {
		auto current = LocalProvider::statSync( URI::fromFSPath( index.path ), index.path, true );
		if ( !current || current.value().revision != index.revision )
			return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
								  "Archive changed while opening entry" ) );
		auto idle =
			reuse ? index.takeReader( index.nodes[node].entry ) : ArchiveIndex::IdleReader{};
		auto reader = std::move( idle.reader );
		Uint64 first = reader ? idle.entry + 1 : 0;
		if ( reader ) {
			reader->setURI( uri );
			auto resumed = reader->resume( token );
			if ( !resumed )
				return Err( std::move( resumed ).error() );
		} else {
			reader = std::make_unique<LibArchiveReader>( index.library, index.path, uri );
			auto opened = reader->open( token );
			if ( !opened )
				return Err( std::move( opened ).error() );
			first = 0;
		}
		current = LocalProvider::statSync( URI::fromFSPath( index.path ), index.path, true );
		if ( !current || current.value().revision != index.revision )
			return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
								  "Archive changed while opening entry" ) );
		reader->setURI( uri );
		for ( Uint64 i = first; i <= index.nodes[node].entry; ++i ) {
			auto next = reader->next( token );
			if ( !next )
				return Err( std::move( next ).error() );
			if ( !next.value() )
				return Err( VFSError( VFSErrorCode::IO, uri, "Archive entry is missing" ) );
		}
		return Ok( std::move( reader ) );
	}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		auto work = [self = shared_from_this(), offset,
					 token]( std::span<Uint8> buffer ) -> VFSResult<size_t> {
			std::lock_guard<std::mutex> lock( self->mMutex );
			auto result = self->readInto( offset, buffer, token );
			if ( !result )
				self->mReusable = false;
			return result;
		};
		if ( !mIndex->route.isNetworkMount() ) {
			return mScheduler->submit<size_t>(
				mIndex->route,
				[work = std::move( work ), data]() mutable { return work( data ); } );
		}
		// A mount can return after its timeout, when data no longer exists. Commit only if
		// this call still owns completion; its work touches an independent buffer.
		return mScheduler->submitCommitted<size_t>(
			mIndex->route,
			[work = std::move( work ),
			 size = data.size()]() mutable -> VFSResult<std::vector<Uint8>> {
				std::vector<Uint8> buffer( size );
				auto result = work( buffer );
				if ( !result )
					return Err( std::move( result ).error() );
				buffer.resize( result.value() );
				return Ok( std::move( buffer ) );
			},
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
		return readyValue<VFSFileInfo>( mIndex->nodes[mNode].info );
	}

	VFSTask<void> close() override {
		return mScheduler->submit<void>(
			mIndex->route, [self = shared_from_this()]() -> VFSResult<void> {
				std::lock_guard<std::mutex> lock( self->mMutex );
				if ( self->mReusable && self->mReader && self->mReader->canReuse() &&
					 self->mReader->suspend() ) {
					self->mIndex->recycleReader( std::move( self->mReader ),
												 self->mIndex->nodes[self->mNode].entry );
				} else {
					self->mReader.reset();
				}
				return Ok();
			} );
	}

	ProviderCapabilities capabilities() const override { return ArchiveCapabilities; }

  private:
	VFSResult<size_t> readInto( Uint64 offset, std::span<Uint8> data, CancellationToken token ) {
		if ( token.isCancellationRequested() )
			return Err( cancelledError( mUri ) );
		if ( !mReader )
			return Err( VFSError( VFSErrorCode::InvalidOperation, mUri, "File closed" ) );
		if ( data.empty() )
			return Ok( size_t( 0 ) );
		const auto size = mIndex->nodes[mNode].info.size();
		if ( size && offset >= *size )
			return Ok( size_t( 0 ) );
		if ( offset < mPosition ) {
			auto reopened = openReader( *mIndex, mNode, mUri, token, false );
			if ( !reopened )
				return Err( std::move( reopened ).error() );
			mReader = std::move( reopened ).value();
			mPosition = 0;
		}
		std::array<Uint8, 16384> scratch;
		while ( mPosition < offset ) {
			auto got = mReader->read( std::span<Uint8>( scratch ).first(
										  std::min<Uint64>( scratch.size(), offset - mPosition ) ),
									  token );
			if ( !got )
				return got;
			if ( got.value() == 0 )
				return Err( VFSError( VFSErrorCode::IO, mUri, "Truncated archive entry" ) );
			mPosition += got.value();
		}
		size_t done = 0;
		const size_t wanted = size ? std::min<Uint64>( data.size(), *size - offset ) : data.size();
		while ( done < wanted ) {
			auto got = mReader->read(
				data.subspan( done, std::min<size_t>( wanted - done, 65536 ) ), token );
			if ( !got )
				return got;
			if ( got.value() == 0 ) {
				if ( size )
					return Err( VFSError( VFSErrorCode::IO, mUri, "Truncated archive entry" ) );
				break;
			}
			done += got.value();
			mPosition += got.value();
		}
		// Check the entry's end marker too: some compression/checksum errors surface only
		// when a read advances past the final output byte.
		if ( size && mPosition == *size ) {
			auto end = mReader->read( std::span<Uint8>( scratch ).first( 1 ), token );
			if ( !end )
				return end;
			if ( end.value() != 0 )
				return Err(
					VFSError( VFSErrorCode::IO, mUri, "Archive entry exceeds declared size" ) );
		}
		return Ok( done );
	}

	std::shared_ptr<LocalIOScheduler> mScheduler;
	std::shared_ptr<ArchiveIndex> mIndex;
	std::unique_ptr<LibArchiveReader> mReader;
	std::mutex mMutex;
	URI mUri;
	Uint64 mPosition = 0;
	Uint32 mNode; // The index is capped at MaxEntries, well below UINT32_MAX.
	bool mReusable = true;
};
} // namespace

class ArchiveProviderImpl : public std::enable_shared_from_this<ArchiveProviderImpl> {
  public:
	ArchiveProviderImpl( std::shared_ptr<LocalIOScheduler> scheduler, std::string path ) :
		scheduler( std::move( scheduler ) ), mLibraryPath( std::move( path ) ) {}

	VFSResult<std::shared_ptr<LibArchive>> library( const URI& uri ) {
		std::call_once( mLoad, [&] {
			auto loaded = LibArchive::load( mLibraryPath, uri );
			if ( loaded )
				mLibrary = std::move( loaded ).value();
		} );
		if ( !mLibrary )
			return Err(
				VFSError( VFSErrorCode::Unsupported, uri,
						  "A compatible libarchive 3.x runtime library is not available" ) );
		return Ok( mLibrary );
	}

	template <typename T, typename F>
	VFSTask<T> run( const URI& uri, CancellationToken token, F&& fn ) {
		if ( token.isCancellationRequested() )
			return readyError<T>( cancelledError( uri ) );
		auto address = ArchiveLocation::parse( uri, "archive" );
		if ( !address )
			return readyError<T>( std::move( address ).error() );
		auto native = LocalProvider::toNativePath( address.value().archive );
		if ( !native )
			return readyError<T>( std::move( native ).error() );
		std::string path = std::move( native ).value();
		std::string member = std::move( address ).value().member;
		while ( !member.empty() && member.front() == '/' )
			member.erase( member.begin() );
		while ( !member.empty() && member.back() == '/' )
			member.pop_back();
		return scheduler->submitResolved<T>(
			path, true,
			[self = shared_from_this(), uri, path, member = std::move( member ),
			 token = std::move( token ), fn = std::forward<F>( fn )]() mutable -> VFSResult<T> {
				if ( token.isCancellationRequested() )
					return Err( cancelledError( uri ) );
				auto index = self->load( path, uri, token );
				if ( !index )
					return Err( std::move( index ).error() );
				return fn( std::move( index ).value(), member, uri, token );
			} );
	}

	std::shared_ptr<LocalIOScheduler> scheduler;

  private:
	struct Cached {
		std::shared_ptr<ArchiveIndex> index;
		Uint64 used = 0;
	};

	VFSResult<std::shared_ptr<ArchiveIndex>> load( const std::string& path, const URI& uri,
												   CancellationToken token ) {
		auto loaded = library( uri );
		if ( !loaded )
			return Err( std::move( loaded ).error() );
		const URI backing = URI::fromFSPath( path );
		auto before = LocalProvider::statSync( backing, path, true );
		if ( !before )
			return Err( std::move( before ).error() );
		if ( !before.value().isRegularFile() )
			return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
								  "Archive backing file is not a regular file" ) );
		const auto revision = before.value().revision;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			for ( auto& cached : mCache ) {
				if ( cached.index && cached.index->path == path &&
					 cached.index->revision == revision ) {
					cached.used = ++mUsed;
					return Ok( cached.index );
				}
			}
		}
		auto index = std::make_shared<ArchiveIndex>();
		index->path = path;
		index->revision = revision;
		index->library = std::move( loaded ).value();
		index->route = scheduler->resolvedRouteFor( path, true );
		LibArchiveReader reader( index->library, path, uri );
		auto opened = reader.open( token );
		if ( !opened )
			return Err( std::move( opened ).error() );
		auto indexed = index->index( reader, uri, token );
		if ( !indexed )
			return Err( std::move( indexed ).error() );
		auto after = LocalProvider::statSync( backing, path, true );
		if ( !after || after.value().revision != revision )
			return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
								  "Archive changed while indexing" ) );
		std::shared_ptr<ArchiveIndex> retired;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			auto slot = mCache.begin();
			for ( auto it = mCache.begin(); it != mCache.end(); ++it ) {
				if ( it->index && it->index->path == path ) {
					slot = it;
					break;
				}
				if ( it->used < slot->used )
					slot = it;
			}
			retired = std::move( slot->index );
			slot->index = index;
			slot->used = ++mUsed;
		}
		return Ok( std::move( index ) );
	}

	std::string mLibraryPath;
	std::shared_ptr<LibArchive> mLibrary;
	std::once_flag mLoad;
	std::mutex mMutex;
	std::array<Cached, 8> mCache;
	Uint64 mUsed = 0;
};

ArchiveProvider::ArchiveProvider( std::shared_ptr<LocalIOScheduler> scheduler,
								  std::string libraryPath ) :
	mImpl( std::make_shared<ArchiveProviderImpl>( std::move( scheduler ),
												  std::move( libraryPath ) ) ) {}

ArchiveProvider::~ArchiveProvider() = default;

URI ArchiveProvider::makeURI( const URI& archive, std::string_view member ) {
	return ArchiveLocation::make( "archive", archive, member );
}

VFSResult<URI> ArchiveProvider::archiveURI( const URI& uri ) {
	auto address = ArchiveLocation::parse( uri, "archive" );
	if ( !address )
		return Err( std::move( address ).error() );
	return Ok( std::move( address ).value().archive );
}

VFSResult<std::string> ArchiveProvider::memberPath( const URI& uri ) {
	auto address = ArchiveLocation::parse( uri, "archive" );
	if ( !address )
		return Err( std::move( address ).error() );
	return Ok( std::move( address ).value().member );
}

VFSTask<ArchiveSupport> ArchiveProvider::support( CancellationToken token ) {
	return mImpl->scheduler->submit<ArchiveSupport>(
		LocalIOScheduler::Route{},
		[impl = mImpl, token = std::move( token )]() -> VFSResult<ArchiveSupport> {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( {} ) );
			auto library = impl->library( {} );
			if ( !library )
				return Err( std::move( library ).error() );
			return Ok( library.value()->support() );
		} );
}

bool ArchiveProvider::relocateURI( URI& uri, const URI& from, const URI& to ) const {
	auto address = detail::ArchiveLocation::parse( uri, scheme() );
	if ( !address || !VFSProvider::relocateURI( address.value().archive, from, to ) )
		return false;
	uri = makeURI( address.value().archive, address.value().member );
	return true;
}

ProviderCapabilities ArchiveProvider::capabilities( const URI& ) const {
	return ArchiveCapabilities;
}

VFSTask<VFSFileInfo> ArchiveProvider::stat( const URI& uri, const StatOptions& options,
											CancellationToken token ) {
	return mImpl->run<VFSFileInfo>(
		uri, std::move( token ),
		[options]( const std::shared_ptr<ArchiveIndex>& index, const std::string& member,
				   const URI& uri, CancellationToken ) -> VFSResult<VFSFileInfo> {
			auto found = index->find( member, uri, options.followSymlinks );
			if ( !found )
				return Err( std::move( found ).error() );
			auto info = index->nodes[found.value()].info;
			if ( !member.empty() )
				info.name = member.substr( member.find_last_of( '/' ) + 1 );
			info.hidden = info.name.starts_with( '.' );
			return Ok( std::move( info ) );
		} );
}

VFSTask<DirectoryHandle> ArchiveProvider::openDirectory( const URI& uri,
														 const DirectoryOpenOptions&,
														 CancellationToken token ) {
	return mImpl->run<DirectoryHandle>(
		uri, std::move( token ),
		[]( const std::shared_ptr<ArchiveIndex>& index, const std::string& member, const URI& uri,
			CancellationToken token ) -> VFSResult<DirectoryHandle> {
			auto found = index->find( member, uri );
			if ( !found )
				return Err( std::move( found ).error() );
			const auto& folder = index->nodes[found.value()];
			if ( !folder.info.isDirectory() )
				return Err( VFSError( VFSErrorCode::NotDirectory, uri ) );
			std::vector<VFSFileInfo> entries;
			entries.reserve( folder.children.size() );
			for ( auto child : folder.children ) {
				if ( token.isCancellationRequested() )
					return Err( cancelledError( uri ) );
				auto info = index->nodes[child].info;
				if ( info.isSymlink() || !index->nodes[child].hardLink.empty() ) {
					auto target =
						index->find( member.empty() ? info.name : member + "/" + info.name, uri );
					if ( target ) {
						if ( !index->nodes[child].hardLink.empty() ) {
							std::string name = std::move( info.name );
							info = index->nodes[target.value()].info;
							info.name = std::move( name );
							info.hidden = info.name.starts_with( '.' );
						} else {
							info.targetType = index->nodes[target.value()].info.type;
							if ( auto size = index->nodes[target.value()].info.size() )
								info.setSize( *size );
						}
					}
				}
				entries.push_back( std::move( info ) );
			}
			return Ok( DirectoryHandle(
				uri, std::make_shared<detail::VectorDirectoryImpl>( std::move( entries ) ) ) );
		} );
}

VFSTask<FileHandle> ArchiveProvider::open( const URI& uri, const OpenOptions& options,
										   CancellationToken token ) {
	if ( hasFlag( options.flags, OpenFlags::Write ) ||
		 hasFlag( options.flags, OpenFlags::Create ) ||
		 hasFlag( options.flags, OpenFlags::Truncate ) )
		return readyError<FileHandle>( readOnly( uri ) );
	return mImpl->run<FileHandle>(
		uri, std::move( token ),
		[scheduler = mImpl->scheduler]( std::shared_ptr<ArchiveIndex> index,
										const std::string& member, const URI& uri,
										CancellationToken token ) -> VFSResult<FileHandle> {
			auto found = index->find( member, uri );
			if ( !found )
				return Err( std::move( found ).error() );
			const auto& node = index->nodes[found.value()];
			if ( node.info.isDirectory() )
				return Err( VFSError( VFSErrorCode::IsDirectory, uri ) );
			if ( !node.info.isRegularFile() || node.entry == NoEntry )
				return Err( VFSError( VFSErrorCode::Unsupported, uri,
									  "Archive entry is not a regular file" ) );
			auto reader = ArchiveFile::openReader( *index, found.value(), uri, token );
			if ( !reader )
				return Err( std::move( reader ).error() );
			return Ok( FileHandle( uri, std::make_shared<ArchiveFile>(
											scheduler, std::move( index ),
											std::move( reader ).value(), found.value(), uri ) ) );
		} );
}

VFSTask<void> ArchiveProvider::mkdir( const URI& uri, const MkDirOptions&, CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<void> ArchiveProvider::remove( const URI& uri, const RemoveOptions&, CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<void> ArchiveProvider::rename( const URI& uri, const URI&, const RenameOptions&,
									   CancellationToken ) {
	return readyError<void>( readOnly( uri ) );
}

VFSTask<URI> ArchiveProvider::canonicalize( const URI& uri, CancellationToken token ) {
	return mImpl->run<URI>( uri, std::move( token ),
							[]( const std::shared_ptr<ArchiveIndex>& index,
								const std::string& member, const URI& uri,
								CancellationToken ) -> VFSResult<URI> {
								std::string resolved;
								auto found = index->find( member, uri, true, &resolved );
								if ( !found )
									return Err( std::move( found ).error() );
								auto backing = ArchiveProvider::archiveURI( uri );
								return Ok( ArchiveProvider::makeURI( backing.value(), resolved ) );
							} );
}

VFSTask<bool> ArchiveProvider::sameFileSystem( const URI& from, const URI& to,
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
