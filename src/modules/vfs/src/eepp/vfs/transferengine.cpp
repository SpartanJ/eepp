#include <eepp/vfs/detail/filewriter.hpp>
#include <eepp/vfs/detail/transferstate.hpp>
#include <eepp/vfs/transferengine.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>

namespace EE { namespace VFS {

using detail::await;
using detail::FileWriter;

Transfer TransferEngine::start( VirtualFileSystem& vfs, std::shared_ptr<ThreadPool> pool,
								URI source, URI destination, const CopyOptions& options,
								bool move ) {
	auto state = std::make_shared<detail::TransferState>();
	Transfer transfer( state );
	if ( !pool ) {
		state->complete(
			Err( VFSError( VFSErrorCode::InvalidOperation, source, "VFS is shutting down" ) ) );
		return transfer;
	}
	auto engine = std::make_shared<TransferEngine>( vfs, state, std::move( source ),
													std::move( destination ), options, move );
	pool->run( [engine = std::move( engine ), state]() { state->complete( engine->run() ); } );
	return transfer;
}

TransferEngine::TransferEngine( VirtualFileSystem& vfs,
								std::shared_ptr<detail::TransferState> state, URI source,
								URI destination, CopyOptions options, bool move ) :
	mVfs( vfs ),
	mState( std::move( state ) ),
	mSource( std::move( source ) ),
	mDestination( std::move( destination ) ),
	mOptions( options ),
	mToken( mState->token() ),
	mMove( move ) {
	if ( mOptions.chunkSize == 0 )
		mOptions.chunkSize = FileWriter::IOChunkSize;
}

VFSResult<TransferResult> TransferEngine::run() {
	auto srcProvider = mVfs.providerFor( mSource );
	auto dstProvider = mVfs.providerFor( mDestination );
	if ( !srcProvider )
		return Err( VFSError( VFSErrorCode::Unsupported, mSource, "Unknown scheme" ) );
	if ( !dstProvider )
		return Err( VFSError( VFSErrorCode::Unsupported, mDestination, "Unknown scheme" ) );
	mSrcProvider = srcProvider.get();
	mDstProvider = dstProvider.get();

	if ( mSource == mDestination ) {
		return Err( VFSError( VFSErrorCode::InvalidOperation, mSource,
							  "Source and destination are the same" ) );
	}

	mSameProvider = mSrcProvider == mDstProvider && mSource.hasSameAuthority( mDestination );

	if ( mMove && mSameProvider ) {
		auto renamed = await( mSrcProvider->rename(
			mSource, mDestination,
			RenameOptions{ mOptions.overwrite == OverwritePolicy::Overwrite }, mToken ) );
		if ( renamed ) {
			mResult.renamed = true;
			mState->updateProgress(
				[&]( TransferProgress& p ) {
					p.currentSource = mSource;
					p.currentDestination = mDestination;
					p.filesDone = 1;
					p.filesTotal = 1;
				},
				true );
			return Ok( mResult );
		}
		const auto code = renamed.error().code;
		bool fallback =
			code == VFSErrorCode::Unsupported || code == VFSErrorCode::NotEmpty ||
			( code == VFSErrorCode::AlreadyExists &&
			  mOptions.overwrite != OverwritePolicy::Fail ) ||
			( code == VFSErrorCode::IsDirectory && mOptions.overwrite != OverwritePolicy::Fail );
		if ( !fallback )
			return Err( std::move( renamed ).error() );
	}

	auto rootInfo = await( mSrcProvider->stat( mSource, StatOptions{ false }, mToken ) );
	if ( !rootInfo )
		return Err( std::move( rootInfo ).error() );
	if ( rootInfo.value().isSymlink() ) {
		// Providers cannot create links, so a copy takes the link's content. A move cannot:
		// removing what was copied would empty the link's target, not move the link.
		if ( mMove ) {
			return Err( VFSError( VFSErrorCode::Unsupported, mSource,
								  "Symbolic links can only be moved within their file system" ) );
		}
		rootInfo = await( mSrcProvider->stat( mSource, StatOptions{ true }, mToken ) );
		if ( !rootInfo )
			return Err( std::move( rootInfo ).error() );
	}

	if ( rootInfo.value().isDirectory() && mSameProvider &&
		 mDestination.isSameOrChildOf( mSource ) ) {
		return Err( VFSError( VFSErrorCode::InvalidOperation, mDestination,
							  "Cannot copy a directory into itself" ) );
	}

	// Scan first: totals for progress, and the copy phase reuses the listing instead of
	// enumerating (possibly remote) directories twice.
	Node root{ std::move( rootInfo ).value(), {} };
	Uint64 files = 0, bytes = 0;
	if ( auto scanned = scan( root, mSource, 0, files, bytes ); !scanned )
		return Err( std::move( scanned ).error() );
	mState->updateProgress(
		[&]( TransferProgress& p ) {
			p.filesTotal = files;
			p.bytesTotal = bytes;
		},
		true );

	auto copied = copyNode( root, mSource, mDestination, 0 );
	if ( !copied )
		return Err( std::move( copied ).error() );
	return Ok( mResult );
}

VFSResult<void> TransferEngine::checkCancelled( const URI& uri ) const {
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( uri ) );
	return Ok();
}

VFSResult<void> TransferEngine::scan( Node& node, const URI& uri, int depth, Uint64& files,
									  Uint64& bytes ) {
	if ( auto c = checkCancelled( uri ); !c )
		return c;
	if ( depth > MaxDepth )
		return Err( VFSError( VFSErrorCode::TooManySymlinks, uri, "Directory tree too deep" ) );
	if ( !node.info.isDirectory() ) {
		++files;
		bytes += node.info.size().value_or( 0 );
		return Ok();
	}
	if ( !mOptions.recursive )
		return Err( VFSError( VFSErrorCode::IsDirectory, uri, "Recursive copy disabled" ) );
	auto listed = await( mVfs.listAll( uri, DirectoryOpenOptions{}, mToken ) );
	if ( !listed )
		return Err( std::move( listed ).error() );
	node.children.reserve( listed.value().size() );
	for ( auto& entry : listed.value() ) {
		Node child{ std::move( entry ), {} };
		if ( child.info.isSymlink() && child.info.targetType == FileType::Directory ) {
			// Providers cannot create symlinks, and following directory links risks cycles:
			// symlinked directories inside the tree are skipped.
			node.children.push_back( std::move( child ) );
			continue;
		}
		if ( auto scanned =
				 scan( child, uri.withAppendedSegment( child.info.name ), depth + 1, files, bytes );
			 !scanned ) {
			return scanned;
		}
		node.children.push_back( std::move( child ) );
	}
	return Ok();
}

VFSResult<bool> TransferEngine::copyNode( const Node& node, const URI& source,
										  const URI& destination, int depth ) {
	if ( auto c = checkCancelled( source ); !c )
		return Err( std::move( c ).error() );
	const FileInfo& info = node.info;

	if ( depth > 0 && info.isSymlink() && info.targetType == FileType::Directory ) {
		++mResult.filesSkipped;
		return Ok( false );
	}

	if ( info.isDirectory() )
		return copyDirectory( node, source, destination, depth );

	if ( !info.isRegularFile() ) {
		// Sockets, FIFOs, devices and dangling links are not copied.
		++mResult.filesSkipped;
		return Ok( false );
	}

	auto existing = await( mDstProvider->stat( destination, StatOptions{}, mToken ) );
	if ( existing ) {
		if ( existing.value().isDirectory() )
			return Err( VFSError( VFSErrorCode::IsDirectory, destination ) );
		if ( mOptions.overwrite == OverwritePolicy::Fail )
			return Err( VFSError( VFSErrorCode::AlreadyExists, destination ) );
		if ( mOptions.overwrite == OverwritePolicy::Skip ) {
			++mResult.filesSkipped;
			advanceFiles( info );
			return Ok( false );
		}
		if ( mSameProvider ) {
			auto same = isSameFile( source, destination );
			if ( !same )
				return Err( std::move( same ).error() );
			if ( same.value() ) {
				return Err( VFSError( VFSErrorCode::InvalidOperation, destination,
									  "Source and destination are the same file" ) );
			}
		}
	} else if ( !existing.error().is( VFSErrorCode::NotFound ) ) {
		return Err( std::move( existing ).error() );
	}

	if ( auto copied = copyFile( info, source, destination ); !copied ) {
		// A destination can appear while the temporary is written. Skip still applies at
		// publication time, and a move must keep that source.
		if ( mOptions.overwrite == OverwritePolicy::Skip &&
			 copied.error().is( VFSErrorCode::AlreadyExists ) ) {
			++mResult.filesSkipped;
			advanceFiles( info );
			return Ok( false );
		}
		return Err( std::move( copied ).error() );
	}
	++mResult.filesTransferred;
	advanceFiles( info, false );

	if ( mMove ) {
		auto removed = await( mSrcProvider->remove( source, {}, mToken ) );
		if ( !removed )
			return Err( std::move( removed ).error() );
		++mResult.sourcesRemoved;
	}
	return Ok( true );
}

VFSResult<bool> TransferEngine::copyDirectory( const Node& node, const URI& source,
											   const URI& destination, int depth ) {
	auto existing = await( mDstProvider->stat( destination, StatOptions{}, mToken ) );
	if ( existing ) {
		if ( !existing.value().isDirectory() )
			return Err( VFSError( VFSErrorCode::NotDirectory, destination ) );
		if ( mOptions.overwrite == OverwritePolicy::Fail )
			return Err( VFSError( VFSErrorCode::AlreadyExists, destination ) );
	} else if ( !existing.error().is( VFSErrorCode::NotFound ) ) {
		return Err( std::move( existing ).error() );
	} else {
		MkDirOptions mkdir;
		if ( mOptions.preservePermissions &&
			 mDstProvider->capabilities( destination ).has( VFSCapability::Permissions ) ) {
			mkdir.permissions = node.info.permissions();
		}
		auto made = await( mDstProvider->mkdir( destination, mkdir, mToken ) );
		if ( !made )
			return Err( std::move( made ).error() );
		++mResult.directoriesCreated;
	}

	bool complete = true;
	for ( const auto& child : node.children ) {
		auto copied = copyNode( child, source.withAppendedSegment( child.info.name ),
								destination.withAppendedSegment( child.info.name ), depth + 1 );
		if ( !copied )
			return copied;
		complete = complete && copied.value();
	}

	// A move only removes a source directory whose whole content was moved: skipped or
	// uncopyable entries keep it (and their data) in place.
	if ( mMove && complete ) {
		auto removed = await( mSrcProvider->remove( source, {}, mToken ) );
		if ( !removed )
			return Err( std::move( removed ).error() );
		++mResult.sourcesRemoved;
	}
	return Ok( complete );
}

void TransferEngine::advanceFiles( const FileInfo& info, bool addBytes ) {
	mState->updateProgress(
		[&]( TransferProgress& p ) {
			++p.filesDone;
			if ( addBytes )
				p.bytesDone += info.size().value_or( 0 );
		},
		true );
}

VFSResult<bool> TransferEngine::isSameFile( const URI& source, const URI& destination ) {
	auto canonicalSource = await( mSrcProvider->canonicalize( source, mToken ) );
	if ( !canonicalSource )
		return Err( std::move( canonicalSource ).error() );
	auto canonicalDestination = await( mDstProvider->canonicalize( destination, mToken ) );
	if ( !canonicalDestination )
		return Err( std::move( canonicalDestination ).error() );
	return Ok( canonicalSource.value() == canonicalDestination.value() );
}

VFSResult<void> TransferEngine::copyFile( const FileInfo& info, const URI& source,
										  const URI& destination ) {
	mState->updateProgress(
		[&]( TransferProgress& p ) {
			p.currentSource = source;
			p.currentDestination = destination;
		},
		true );

	auto target = FileWriter::resolveTarget( *mDstProvider, destination, mToken );
	if ( !target )
		return Err( std::move( target ).error() );

	auto srcOpened = await( mSrcProvider->open( source, OpenFlags::Read, mToken ) );
	if ( !srcOpened )
		return Err( std::move( srcOpened ).error() );
	FileHandle src( std::move( srcOpened ).value() );

	FileWriter::ReplaceOptions replace;
	if ( mOptions.preservePermissions && info.permissions() )
		replace.exactPermissions = info.permissions();
	replace.overwrite = mOptions.overwrite == OverwritePolicy::Overwrite;
	std::vector<Uint8> buffer( mOptions.chunkSize );
	auto copied = FileWriter::replace(
		*mDstProvider, target.value(), replace, mToken,
		[&]( FileHandle& dst ) -> VFSResult<void> {
			Uint64 offset = 0;
			while ( true ) {
				if ( auto c = checkCancelled( source ); !c )
					return c;
				auto read = await( src.read( offset, buffer, mToken ) );
				if ( !read )
					return Err( std::move( read ).error() );
				size_t got = read.value();
				if ( got == 0 )
					return Ok();
				if ( auto written = FileWriter::writeFully(
						 dst, std::span<const Uint8>( buffer.data(), got ), mToken, offset );
					 !written ) {
					return written;
				}
				offset += got;
				mResult.bytesTransferred += got;
				mState->updateProgress( [got]( TransferProgress& p ) { p.bytesDone += got; } );
			}
		},
		[]() -> VFSResult<void> { return Ok(); } );
	await( src.close() );
	return copied;
}

}} // namespace EE::VFS
