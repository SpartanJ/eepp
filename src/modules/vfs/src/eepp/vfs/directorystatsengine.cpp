#include <eepp/vfs/directorystatsengine.hpp>

#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <tuple>

namespace EE { namespace VFS {

using detail::await;

namespace {

struct IdentityKey {
	FileIdentity identity;
	const VFSProvider* provider;

	bool operator==( const IdentityKey& other ) const {
		return identity == other.identity && provider == other.provider;
	}
};

struct IdentityHash {
	size_t operator()( const IdentityKey& key ) const {
		return hashCombine( std::hash<Uint64>{}( key.identity.volume ),
							std::hash<Uint64>{}( key.identity.file ),
							std::hash<const VFSProvider*>{}( key.provider ) );
	}
};

class Measurement {
  public:
	Measurement( VirtualFileSystem& vfs, DirectoryStatsProgressCallback onProgress,
				 CancellationToken token ) :
		mVfs( vfs ), mOnProgress( std::move( onProgress ) ), mToken( std::move( token ) ) {}

	VFSResult<DirectoryStats> run( const std::vector<URI>& roots ) {
		for ( const auto& root : roots )
			mPending.push_back( { root, 0 } );
		// Visit possible ancestors first. A lexical descendant is covered only when traversal
		// actually reaches it: intermediate symlinks are never followed.
		std::sort( mPending.begin(), mPending.end(), []( const Work& a, const Work& b ) {
			auto authority = []( const URI& uri ) {
				return std::tuple<std::string_view, std::string_view, std::string_view,
								  unsigned short>( uri.getScheme(), uri.getUserInfo(),
												   uri.getHost(), uri.getPort() );
			};
			if ( !a.uri.hasSameAuthority( b.uri ) )
				return authority( b.uri ) < authority( a.uri );
			const auto& aPath = a.uri.getPath();
			const auto& bPath = b.uri.getPath();
			if ( aPath == bPath )
				return b.uri < a.uri;
			// Keep subtrees contiguous: /a/file belongs before /a-sibling, rather than
			// letting raw '/' ordering split descendants from their selected ancestor.
			return std::lexicographical_compare( bPath.begin(), bPath.end(), aPath.begin(),
												 aPath.end(),
												 []( unsigned char x, unsigned char y ) {
													 return x == '/' ? y != '/' : y != '/' && x < y;
												 } );
		} );
		mPending.erase(
			std::unique( mPending.begin(), mPending.end(),
						 []( const Work& a, const Work& b ) { return a.uri == b.uri; } ),
			mPending.end() );
		const URI* ancestor = nullptr;
		for ( auto it = mPending.rbegin(); it != mPending.rend(); ++it ) {
			if ( ancestor && it->uri.isSameOrChildOf( *ancestor ) )
				mRootCoverage[it->uri.getParent()].emplace( it->uri.getLastPathSegment(), false );
			else
				ancestor = &it->uri;
		}
		while ( !mPending.empty() ) {
			if ( mToken.isCancellationRequested() ) {
				publish( true );
				return Err( cancelledError( mPending.back().uri ) );
			}
			auto work = std::move( mPending.back() );
			mPending.pop_back();
			auto result = visit( work );
			if ( !result ) {
				publish( true );
				return Err( std::move( result ).error() );
			}
			publish();
		}
		publish( true );
		return Ok( std::move( mStats ) );
	}

  private:
	struct Work {
		URI uri;
		Uint32 depth;
	};

	using SelectedChildren = SmallUnorderedMap<std::string, bool, 4>;

	bool* coverage( const URI& uri ) {
		if ( mRootCoverage.empty() )
			return nullptr;
		auto parent = mRootCoverage.find( uri.getParent() );
		if ( parent == mRootCoverage.end() )
			return nullptr;
		auto child = parent->second.find( uri.getLastPathSegment() );
		return child == parent->second.end() ? nullptr : &child->second;
	}

	void publish( bool force = false ) {
		if ( mOnProgress &&
			 ( force || mProgressClock.getElapsedTime() >= System::Milliseconds( 200 ) ) ) {
			mProgressClock.restart();
			mOnProgress( mStats );
		}
	}

	void recordError( VFSError error ) {
		++mStats.unreadableEntries;
		if ( !mStats.firstError )
			mStats.firstError = std::make_shared<const VFSError>( std::move( error ) );
	}

	/** Count directory entries, but count each native object's bytes and contents only once. */
	bool account( const VFSFileInfo& info, const VFSProvider* provider, bool root ) {
		if ( info.type == FileType::Directory ) {
			if ( !root )
				++mStats.directories;
		} else {
			++mStats.files;
		}
		if ( info.isSymlink() )
			return false;
		if ( info.type == FileType::Unknown )
			++mStats.unknownSizes;
		if ( auto identity = info.identity();
			 identity && ( root || info.type == FileType::Directory || info.hasMultipleLinks() ) &&
			 !mSeen.emplace( IdentityKey{ *identity, provider }, true ).second )
			return false;
		if ( mStats.allocatedSize ) {
			if ( auto allocated = info.allocatedSize() )
				*mStats.allocatedSize += *allocated;
			else
				mStats.allocatedSize.reset();
		}
		if ( info.type == FileType::Regular ) {
			if ( auto size = info.size() )
				mStats.size += *size;
			else
				++mStats.unknownSizes;
		}
		return info.type == FileType::Directory;
	}

	VFSResult<void> visit( const Work& work ) {
		auto* covered = coverage( work.uri );
		if ( work.depth == 0 && covered && *covered )
			return Ok();
		auto provider = mVfs.providerFor( work.uri );
		if ( !provider )
			return Err( VFSError( VFSErrorCode::Unsupported, work.uri, "Unknown scheme" ) );
		// Recheck queued directories without following links: a directory may have been
		// replaced while its parent was listed. This is a best-effort scan, not a snapshot.
		auto info = await( provider->stat( work.uri, StatOptions{ false }, mToken ) );
		if ( !info )
			return failure( std::move( info ).error(), work.depth == 0 );
		if ( covered )
			*covered = true;
		if ( !account( info.value(), provider.get(), work.depth == 0 ) )
			return Ok();
		if ( work.depth >= 256 ) {
			recordError(
				VFSError( VFSErrorCode::TooManySymlinks, work.uri, "Directory tree too deep" ) );
			return Ok();
		}
		auto opened = await( provider->openDirectory(
			work.uri, DirectoryOpenOptions{ DirectoryMetadata::Storage, false }, mToken ) );
		if ( !opened )
			return failure( std::move( opened ).error(), work.depth == 0 );
		auto directory = std::move( opened ).value();
		// Only explicitly selected descendants need coverage state. Look up their parent
		// once per directory, avoiding URI construction or storage for every scanned file.
		auto selected = mRootCoverage.find( work.uri );
		if ( selected == mRootCoverage.end() && !mRootCoverage.empty() &&
			 work.uri.getPath().size() > 1 && work.uri.getPath().back() == '/' ) {
			// getParent() omits trailing separators except on filesystem roots.
			auto key = work.uri;
			auto path = key.getPath();
			while ( path.size() > 1 && path.back() == '/' )
				path.pop_back();
			key.setPath( path );
			selected = mRootCoverage.find( key );
		}
		for ( ;; ) {
			auto batch = await( directory.read( 256, mToken ) );
			if ( !batch ) {
				directory.close();
				return failure( std::move( batch ).error(), work.depth == 0 );
			}
			for ( auto& entry : batch.value().entries ) {
				if ( mToken.isCancellationRequested() ) {
					directory.close();
					return Err( cancelledError( work.uri ) );
				}
				if ( !VFSPath::validName( entry.name, work.uri ) ) {
					recordError( VFSError( VFSErrorCode::ProtocolError, work.uri,
										   "Invalid directory entry name" ) );
					continue;
				}
				if ( entry.type == FileType::Directory ) {
					mPending.push_back( { entry.uriIn( work.uri ), work.depth + 1 } );
				} else {
					// Some listings omit individual metadata failures. Retry only those entries;
					// a missing size must never silently turn into a valid zero.
					if ( entry.type == FileType::Unknown ||
						 ( entry.type == FileType::Regular && !entry.size() ) ) {
						const URI child = entry.uriIn( work.uri );
						auto current =
							await( provider->stat( child, StatOptions{ false }, mToken ) );
						if ( !current ) {
							if ( current.error().is( VFSErrorCode::Cancelled ) ) {
								directory.close();
								return Err( std::move( current ).error() );
							}
							recordError( std::move( current ).error() );
							continue;
						}
						entry = std::move( current ).value();
						if ( entry.type == FileType::Directory ) {
							mPending.push_back( { child, work.depth + 1 } );
							continue;
						}
					}
					account( entry, provider.get(), false );
					if ( selected != mRootCoverage.end() ) {
						auto child = selected->second.find( entry.name );
						if ( child != selected->second.end() )
							child->second = true;
					}
				}
				publish();
			}
			if ( batch.value().endOfDirectory )
				break;
		}
		directory.close();
		return Ok();
	}

	VFSResult<void> failure( VFSError error, bool root ) {
		if ( root || error.is( VFSErrorCode::Cancelled ) )
			return Err( std::move( error ) );
		recordError( std::move( error ) );
		return Ok();
	}

	VirtualFileSystem& mVfs;
	DirectoryStatsProgressCallback mOnProgress;
	CancellationToken mToken;
	DirectoryStats mStats;
	SmallVector<Work, 16> mPending;
	UnorderedMap<URI, SelectedChildren> mRootCoverage;
	UnorderedMap<IdentityKey, bool, IdentityHash> mSeen;
	System::Clock mProgressClock;
};

} // namespace

VFSResult<DirectoryStats> DirectoryStatsEngine::run( VirtualFileSystem& vfs,
													 const std::vector<URI>& roots,
													 DirectoryStatsProgressCallback onProgress,
													 CancellationToken token ) {
	return Measurement( vfs, std::move( onProgress ), std::move( token ) ).run( roots );
}

}} // namespace EE::VFS
