#include <eepp/vfs/transferengine.hpp>

#include <chrono>
#include <eepp/system/filesystem.hpp>
#include <eepp/vfs/detail/filewriter.hpp>
#include <eepp/vfs/detail/transferstate.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>

namespace EE { namespace VFS {

using detail::await;
using detail::FileWriter;

Transfer TransferEngine::start( VirtualFileSystem& vfs, std::shared_ptr<ThreadPool> pool,
								URI source, URI destination, const CopyOptions& options, bool move,
								TransferConflictResolver onConflict ) {
	auto state = std::make_shared<detail::TransferState>();
	Transfer transfer( state );
	if ( !pool ) {
		state->complete(
			Err( VFSError( VFSErrorCode::InvalidOperation, source, "VFS is shutting down" ) ) );
		return transfer;
	}
	auto engine =
		std::make_shared<TransferEngine>( vfs, state, std::move( source ), std::move( destination ),
										  options, move, std::move( onConflict ) );
	pool->run( [engine = std::move( engine ), state]() { state->complete( engine->run() ); } );
	return transfer;
}

TransferEngine::TransferEngine( VirtualFileSystem& vfs,
								std::shared_ptr<detail::TransferState> state, URI source,
								URI destination, CopyOptions options, bool move,
								TransferConflictResolver onConflict ) :
	mVfs( vfs ),
	mState( std::move( state ) ),
	mSource( std::move( source ) ),
	mDestination( std::move( destination ) ),
	mOptions( options ),
	mOnConflict( std::move( onConflict ) ),
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

	if ( mSrcProvider == mDstProvider ) {
		auto same = await( mSrcProvider->sameFileSystem( mSource, mDestination, mToken ) );
		if ( !same )
			return Err( std::move( same ).error() );
		mSameFileSystem = same.value();
	}
	// Equivalent authorities can name the very same file. Check before even attempting a
	// native move; copy/delete must never publish onto, or remove, its own source.
	if ( mSameFileSystem && !mSource.hasSameAuthority( mDestination ) ) {
		auto destination = await( mDstProvider->stat( mDestination, {}, mToken ) );
		if ( destination ) {
			auto same = isSameFile( mSource, mDestination );
			if ( !same )
				return Err( std::move( same ).error() );
			if ( same.value() ) {
				return Err( VFSError( VFSErrorCode::InvalidOperation, mDestination,
									  "Source and destination are the same file" ) );
			}
		} else if ( !destination.error().is( VFSErrorCode::NotFound ) ) {
			return Err( std::move( destination ).error() );
		}
	}

	if ( mMove && mSameFileSystem && mOnConflict ) {
		for ( ;; ) {
			auto existing =
				await( mDstProvider->stat( mDestination, StatOptions{ false }, mToken ) );
			if ( !existing ) {
				if ( !existing.error().is( VFSErrorCode::NotFound ) )
					return Err( std::move( existing ).error() );
				break;
			}
			auto info = await( mSrcProvider->stat( mSource, StatOptions{ false }, mToken ) );
			if ( !info )
				return Err( std::move( info ).error() );
			if ( mOptions.overwrite == OverwritePolicy::MergeDirectories &&
				 info.value().isDirectory() && existing.value().isDirectory() ) {
				mRootConflictResolved = true;
				break;
			}
			auto resolved =
				resolveConflict( info.value(), existing.value(), mSource, mDestination );
			if ( !resolved )
				return Err( std::move( resolved ).error() );
			const auto& decision = resolved.value();
			if ( decision.action == ConflictAction::Skip ) {
				++mResult.filesSkipped;
				return Ok( mResult );
			}
			if ( decision.action == ConflictAction::Rename ) {
				auto renamed =
					renamedDestination( mDestination, decision.name, info.value().isDirectory() );
				if ( !renamed )
					return Err( std::move( renamed ).error() );
				mDestination = std::move( renamed ).value();
				continue;
			}
			if ( info.value().isDirectory() != existing.value().isDirectory() )
				return Err( VFSError( existing.value().isDirectory() ? VFSErrorCode::IsDirectory
																	 : VFSErrorCode::NotDirectory,
									  mDestination ) );
			mRootConflictResolved = true;
			break;
		}
	}
	if ( mMove && mSameFileSystem ) {
		auto renamed = await( mSrcProvider->rename(
			mSource, mDestination,
			RenameOptions{ mRootConflictResolved ||
						   ( !mOnConflict && mOptions.overwrite == OverwritePolicy::Overwrite ) },
			mToken ) );
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
		bool fallback = code == VFSErrorCode::Unsupported || code == VFSErrorCode::NotEmpty ||
						( code == VFSErrorCode::AlreadyExists &&
						  ( mOptions.overwrite != OverwritePolicy::Fail || mOnConflict ) ) ||
						( code == VFSErrorCode::IsDirectory &&
						  ( mOptions.overwrite != OverwritePolicy::Fail || mOnConflict ) );
		if ( !fallback )
			return Err( std::move( renamed ).error() );
	}

	// A copy-and-remove move cannot succeed without removal support. Reject it before
	// creating or replacing destination entries, but still allow native renames above.
	if ( mMove && !mSrcProvider->capabilities( mSource ).has( VFSCapability::Remove ) ) {
		return Err( VFSError( VFSErrorCode::Unsupported, mSource,
							  "Moving requires source removal support" ) );
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

	if ( rootInfo.value().isDirectory() && mSameFileSystem ) {
		if ( auto checked = checkDirectoryDestination( mSource, mDestination ); !checked )
			return Err( std::move( checked ).error() );
	}

	// Scan first: totals for progress, and the copy phase reuses the listing instead of
	// enumerating (possibly remote) directories twice.
	Node root{ std::move( rootInfo ).value(), {} };
	Uint64 files = 0, bytes = 0;
	if ( auto scanned = scan( root, mSource, mDestination, 0, files, bytes ); !scanned )
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

VFSResult<void> TransferEngine::checkSource( const VFSFileInfo& info, const URI& uri ) {
	if ( info.revision.empty() )
		return Err(
			VFSError( VFSErrorCode::Unsupported, uri, "Moving requires a source revision" ) );
	auto current = await( mSrcProvider->stat( uri, StatOptions{ false }, mToken ) );
	if ( !current && !current.error().is( VFSErrorCode::NotFound ) )
		return Err( std::move( current ).error() );
	if ( !current || current.value().type != info.type ||
		 !info.revision.matches( current.value().revision ) ) {
		return Err(
			VFSError( VFSErrorCode::ChangedExternally, uri,
					  "Source changed during move; source retained and any copied data kept" ) );
	}
	return Ok();
}

VFSResult<void> TransferEngine::checkDirectoryDestination( const URI& sourceURI,
														   const URI& destinationURI ) {
	auto invalid = [&] {
		return Err( VFSError( VFSErrorCode::InvalidOperation, destinationURI,
							  "Cannot copy a directory into itself" ) );
	};
	if ( destinationURI.isSameOrChildOf( sourceURI ) )
		return invalid();
	auto source = await( mSrcProvider->canonicalize( sourceURI, mToken ) );
	if ( !source ) {
		if ( source.error().is( VFSErrorCode::Unsupported ) )
			return Ok();
		return Err( std::move( source ).error() );
	}
	URI ancestor = destinationURI;
	std::vector<std::string> missing;
	while ( true ) {
		auto resolved = await( mDstProvider->canonicalize( ancestor, mToken ) );
		if ( resolved ) {
			ancestor = std::move( resolved ).value();
			break;
		}
		if ( resolved.error().is( VFSErrorCode::Unsupported ) )
			return Ok();
		if ( !resolved.error().is( VFSErrorCode::NotFound ) )
			return Err( std::move( resolved ).error() );
		URI parent = ancestor.getParent();
		if ( parent == ancestor || missing.size() >= MaxDepth )
			return Err( std::move( resolved ).error() );
		missing.push_back( ancestor.getLastPathSegment() );
		ancestor = std::move( parent );
	}
	for ( auto it = missing.rbegin(); it != missing.rend(); ++it )
		ancestor = ancestor.withAppendedSegment( *it );
	ancestor.normalize();
	// Both paths have already been resolved in the same provider namespace. Use one authority
	// for the containment comparison so spelling an SSH alias cannot bypass it.
	ancestor.setAuthority( source.value().getAuthority() );
	if ( ancestor.isSameOrChildOf( source.value() ) )
		return invalid();
	return Ok();
}

VFSResult<void> TransferEngine::scan( Node& node, const URI& uri, const URI& destination, int depth,
									  Uint64& files, Uint64& bytes ) {
	if ( auto c = checkCancelled( uri ); !c )
		return c;
	if ( depth > MaxDepth )
		return Err( VFSError( VFSErrorCode::TooManySymlinks, uri, "Directory tree too deep" ) );
	if ( mMove &&
		 ( node.info.type == FileType::Regular || node.info.type == FileType::Directory ) &&
		 node.info.revision.empty() )
		return Err(
			VFSError( VFSErrorCode::Unsupported, uri, "Moving requires a source revision" ) );
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
		if ( !VFSPath::validName( entry.name, destination ) )
			return Err( VFSError( VFSErrorCode::InvalidURI, destination,
								  "Directory entry is not a valid destination basename" ) );
		Node child{ std::move( entry ), {} };
		if ( child.info.isSymlink() && child.info.targetType == FileType::Directory ) {
			// Providers cannot create symlinks, and following directory links risks cycles:
			// symlinked directories inside the tree are skipped.
			node.children.push_back( std::move( child ) );
			continue;
		}
		if ( auto scanned = scan( child, uri.withAppendedSegment( child.info.name ),
								  destination.withAppendedSegment( child.info.name ), depth + 1,
								  files, bytes );
			 !scanned ) {
			return scanned;
		}
		node.children.push_back( std::move( child ) );
	}
	return Ok();
}

VFSResult<bool> TransferEngine::copyNode( const Node& node, const URI& source, URI destination,
										  int depth ) {
	if ( auto c = checkCancelled( source ); !c )
		return Err( std::move( c ).error() );
	const VFSFileInfo& info = node.info;

	if ( depth > 0 && info.isSymlink() && ( mMove || info.targetType == FileType::Directory ) ) {
		// Copying link contents during a move can remove the target before another link
		// is read, and cannot reproduce the link at the destination. Preserve the link itself.
		++mResult.filesSkipped;
		if ( info.targetType != FileType::Directory )
			advanceFiles( info );
		return Ok( false );
	}

	if ( info.isDirectory() )
		return copyDirectory( node, source, destination, depth );

	if ( !info.isRegularFile() ) {
		// Sockets, FIFOs, devices and dangling links are not copied.
		++mResult.filesSkipped;
		advanceFiles( info );
		return Ok( false );
	}

	bool overwrite = ( !mOnConflict && mOptions.overwrite == OverwritePolicy::Overwrite ) ||
					 ( depth == 0 && mRootConflictResolved );
	for ( ;; ) {
		auto existing = await( mDstProvider->stat( destination, StatOptions{}, mToken ) );
		if ( existing ) {
			if ( mOnConflict && !( depth == 0 && mRootConflictResolved ) ) {
				auto resolved = resolveConflict( info, existing.value(), source, destination );
				if ( !resolved )
					return Err( std::move( resolved ).error() );
				const auto& decision = resolved.value();
				if ( decision.action == ConflictAction::Rename ) {
					auto renamed = renamedDestination( destination, decision.name, false );
					if ( !renamed )
						return Err( std::move( renamed ).error() );
					destination = std::move( renamed ).value();
					overwrite = false;
					continue;
				}
				if ( decision.action == ConflictAction::Skip ) {
					++mResult.filesSkipped;
					advanceFiles( info );
					return Ok( false );
				}
				overwrite = true;
			} else if ( !mOnConflict ) {
				if ( existing.value().isDirectory() )
					return Err( VFSError( VFSErrorCode::IsDirectory, destination ) );
				if ( mOptions.overwrite == OverwritePolicy::Fail ||
					 mOptions.overwrite == OverwritePolicy::MergeDirectories )
					return Err( VFSError( VFSErrorCode::AlreadyExists, destination ) );
				if ( mOptions.overwrite == OverwritePolicy::Skip ) {
					++mResult.filesSkipped;
					advanceFiles( info );
					return Ok( false );
				}
			}
			if ( mSameFileSystem ) {
				auto same = isSameFile( source, destination );
				if ( !same )
					return Err( std::move( same ).error() );
				if ( same.value() )
					return Err( VFSError( VFSErrorCode::InvalidOperation, destination,
										  "Source and destination are the same file" ) );
			}
			if ( existing.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::IsDirectory, destination ) );
		} else if ( !existing.error().is( VFSErrorCode::NotFound ) ) {
			return Err( std::move( existing ).error() );
		}

		if ( mMove ) {
			if ( auto checked = checkSource( info, source ); !checked )
				return Err( std::move( checked ).error() );
			// A conflict decision can still use a native move, including an auto-renamed target.
			if ( mSameFileSystem && mOnConflict ) {
				auto renamed = await( mSrcProvider->rename( source, destination,
															RenameOptions{ overwrite }, mToken ) );
				if ( renamed ) {
					mResult.renamed = depth == 0;
					++mResult.filesTransferred;
					++mResult.sourcesRemoved;
					advanceFiles( info );
					return Ok( true );
				}
				if ( renamed.error().is( VFSErrorCode::AlreadyExists ) )
					continue;
				if ( !renamed.error().is( VFSErrorCode::Unsupported ) )
					return Err( std::move( renamed ).error() );
			}
		}
		if ( auto copied = copyFile( info, source, destination, overwrite ); !copied ) {
			// Resolve destinations which appeared while writing, without replaying completed files.
			if ( copied.error().is( VFSErrorCode::AlreadyExists ) ) {
				if ( mOnConflict )
					continue;
				if ( mOptions.overwrite == OverwritePolicy::Skip ) {
					++mResult.filesSkipped;
					advanceFiles( info );
					return Ok( false );
				}
			}
			return Err( std::move( copied ).error() );
		}
		break;
	}
	++mResult.filesTransferred;
	advanceFiles( info, false );

	if ( mMove ) {
		// The destination is recoverable even if reading or publishing it raced an editor.
		// A path re-stat cannot make the final unlink atomic; never delete a known newer source.
		if ( auto checked = checkSource( info, source ); !checked )
			return Err( std::move( checked ).error() );
		auto removed = await( mSrcProvider->remove( source, {}, mToken ) );
		if ( !removed )
			return Err( std::move( removed ).error() );
		++mResult.sourcesRemoved;
	}
	return Ok( true );
}

VFSResult<bool> TransferEngine::copyDirectory( const Node& node, const URI& source, URI destination,
											   int depth ) {
	if ( mMove ) {
		if ( auto checked = checkSource( node.info, source ); !checked )
			return Err( std::move( checked ).error() );
	}
	for ( ;; ) {
		auto existing = await( mDstProvider->stat( destination, StatOptions{}, mToken ) );
		if ( existing ) {
			if ( mOnConflict && !( depth == 0 && mRootConflictResolved ) &&
				 ( mOptions.overwrite != OverwritePolicy::MergeDirectories ||
				   !existing.value().isDirectory() ) ) {
				auto resolved = resolveConflict( node.info, existing.value(), source, destination );
				if ( !resolved )
					return Err( std::move( resolved ).error() );
				const auto& decision = resolved.value();
				if ( decision.action == ConflictAction::Rename ) {
					auto renamed = renamedDestination( destination, decision.name, true );
					if ( !renamed )
						return Err( std::move( renamed ).error() );
					destination = std::move( renamed ).value();
					continue;
				}
				if ( decision.action == ConflictAction::Skip ) {
					++mResult.filesSkipped;
					return Ok( false );
				}
			} else if ( !mOnConflict ) {
				if ( !existing.value().isDirectory() )
					return Err( VFSError( VFSErrorCode::NotDirectory, destination ) );
				if ( mOptions.overwrite == OverwritePolicy::Fail )
					return Err( VFSError( VFSErrorCode::AlreadyExists, destination ) );
			}
			if ( !existing.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::NotDirectory, destination ) );
		} else if ( !existing.error().is( VFSErrorCode::NotFound ) ) {
			return Err( std::move( existing ).error() );
		} else {
			MkDirOptions mkdir;
			if ( mOptions.preservePermissions &&
				 mDstProvider->capabilities( destination ).has( VFSCapability::Permissions ) ) {
				mkdir.permissions = node.info.permissions();
			}
			auto made = await( mDstProvider->mkdir( destination, mkdir, mToken ) );
			if ( !made ) {
				if ( mOnConflict && made.error().is( VFSErrorCode::AlreadyExists ) )
					continue;
				return Err( std::move( made ).error() );
			}
			++mResult.directoriesCreated;
		}
		break;
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
		// Removing our children changes a directory's revision. Empty source directories have
		// no such expected changes and can still be checked against their scanned revision.
		if ( node.children.empty() ) {
			if ( auto checked = checkSource( node.info, source ); !checked )
				return Err( std::move( checked ).error() );
		}
		auto removed = await( mSrcProvider->remove( source, {}, mToken ) );
		if ( !removed )
			return Err( std::move( removed ).error() );
		++mResult.sourcesRemoved;
	}
	return Ok( complete );
}

void TransferEngine::advanceFiles( const VFSFileInfo& info, bool addBytes ) {
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
	return Ok( canonicalSource.value().getPath() == canonicalDestination.value().getPath() );
}

VFSResult<ConflictResolution> TransferEngine::resolveConflict( const VFSFileInfo& sourceInfo,
															   const VFSFileInfo& destinationInfo,
															   const URI& source,
															   const URI& destination ) {
	if ( auto cancelled = checkCancelled( source ); !cancelled )
		return Err( std::move( cancelled ).error() );
	TransferConflict conflict{ source, destination, sourceInfo, destinationInfo };
	auto task = mOnConflict( std::move( conflict ), mToken );
	while ( task.waitFor( std::chrono::milliseconds( 20 ) ) == TaskState::Pending ) {
		if ( auto cancelled = checkCancelled( source ); !cancelled )
			return Err( std::move( cancelled ).error() );
	}
	if ( auto cancelled = checkCancelled( source ); !cancelled )
		return Err( std::move( cancelled ).error() );
	auto resolved = await( std::move( task ) );
	if ( !resolved )
		return resolved;
	auto& decision = resolved.value();
	if ( decision.action == ConflictAction::Cancel )
		return Err( cancelledError( source ) );
	if ( decision.action == ConflictAction::OverwriteOlder ) {
		const auto src = sourceInfo.modifiedTime();
		const auto dst = destinationInfo.modifiedTime();
		decision.action =
			src && dst && *src > *dst && !sourceInfo.isDirectory() && !destinationInfo.isDirectory()
				? ConflictAction::Overwrite
				: ConflictAction::Skip;
	}
	return resolved;
}

VFSResult<URI> TransferEngine::renamedDestination( const URI& destination, const std::string& name,
												   bool directory ) {
	auto validate = [this, directory]( URI renamed ) -> VFSResult<URI> {
		if ( renamed == mSource )
			return Err( VFSError( VFSErrorCode::InvalidOperation, renamed,
								  "Source and destination must remain distinct" ) );
		if ( directory && mSameFileSystem ) {
			if ( auto checked = checkDirectoryDestination( mSource, renamed ); !checked )
				return Err( std::move( checked ).error() );
		}
		return Ok( std::move( renamed ) );
	};
	if ( !name.empty() ) {
		if ( !VFSPath::validName( name, destination.getParent() ) ||
			 name == destination.getLastPathSegment() )
			return Err( VFSError( VFSErrorCode::InvalidURI, destination, "Invalid new name" ) );
		return validate( destination.getParent().withAppendedSegment( name ) );
	}
	const auto original = destination.getLastPathSegment();
	auto parts = directory ? std::make_pair( original, std::string() )
						   : System::FileSystem::fileNameSplitExtension( original );
	for ( Uint64 number = 1;; ++number ) {
		if ( auto cancelled = checkCancelled( destination ); !cancelled )
			return Err( std::move( cancelled ).error() );
		URI candidate = destination.getParent().withAppendedSegment(
			parts.first + " (" + String::toString( number ) + ")" + parts.second );
		auto existing = await( mDstProvider->stat( candidate, StatOptions{ false }, mToken ) );
		if ( !existing && existing.error().is( VFSErrorCode::NotFound ) )
			return validate( std::move( candidate ) );
		if ( !existing )
			return Err( std::move( existing ).error() );
	}
}

VFSResult<void> TransferEngine::copyFile( const VFSFileInfo& info, const URI& source,
										  const URI& destination, bool overwrite ) {
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
	replace.overwrite = overwrite;
	std::vector<Uint8> buffer( mOptions.chunkSize );
	Uint64 offset = 0;
	auto copied = FileWriter::replace(
		*mDstProvider, target.value(), replace, mToken,
		[&]( FileHandle& dst ) -> VFSResult<void> {
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
				mState->updateProgress( [got]( TransferProgress& p ) { p.bytesDone += got; } );
			}
		},
		[]() -> VFSResult<void> { return Ok(); } );
	await( src.close() );
	if ( copied ) {
		mResult.bytesTransferred += offset;
	} else if ( offset > 0 ) {
		// The temporary was discarded. Do not count it again if a publication conflict
		// resolves to retry or skip this entry.
		mState->updateProgress( [offset]( TransferProgress& p ) { p.bytesDone -= offset; }, true );
	}
	return copied;
}

}} // namespace EE::VFS
