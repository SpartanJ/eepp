#include "zipcreation.hpp"
#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/system/zip.hpp>
#include <eepp/vfs/detail/filewriter.hpp>
#include <eepp/vfs/detail/transferstate.hpp>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <limits>

namespace EE { namespace VFS {

using namespace EE::System;
using detail::await;
using detail::FileWriter;

namespace {

/** Keep the provider's typed read/seek failure before libzip releases its input source. */
class ZipInput final : public IOStream {
  public:
	ZipInput( std::unique_ptr<IOStreamVFS> stream, std::optional<VFSError>& error ) :
		mStream( std::move( stream ) ), mError( error ) {}

	ios_size read( char* data, ios_size size ) {
		auto count = mStream->read( data, size );
		rememberError();
		return count;
	}

	ios_size write( const char*, ios_size ) { return -1; }

	ios_size seek( ios_size position ) {
		auto result = mStream->seek( position );
		rememberError();
		return result;
	}

	ios_size tell() { return mStream->tell(); }

	ios_size getSize() { return mStream->getSize(); }

	bool isOpen() { return mStream->isOpen(); }

  private:
	void rememberError() {
		if ( !mError && mStream->error() )
			mError = *mStream->error();
	}

	std::unique_ptr<IOStreamVFS> mStream;
	std::optional<VFSError>& mError;
};

/** Borrows the transaction's handle; FileWriter owns flush, close and publication. */
class ZipOutput final : public IOStream {
  public:
	ZipOutput( FileHandle& file, CancellationToken token ) :
		mFile( file ), mToken( std::move( token ) ) {}

	ios_size read( char*, ios_size ) { return 0; }

	ios_size write( const char* data, ios_size size ) {
		if ( size < 0 || size > std::numeric_limits<ios_size>::max() - mPosition )
			return -1;
		auto result = FileWriter::writeFully(
			mFile, { reinterpret_cast<const Uint8*>( data ), static_cast<size_t>( size ) }, mToken,
			mPosition );
		if ( !result ) {
			error = std::move( result ).error();
			return -1;
		}
		mPosition += size;
		mSize = std::max( mSize, mPosition );
		return size;
	}

	ios_size seek( ios_size position ) {
		if ( position < 0 || position > mSize )
			return -1;
		return mPosition = position;
	}

	ios_size tell() { return mPosition; }

	ios_size getSize() { return mSize; }

	bool isOpen() { return !error && mFile.isOpen(); }

	std::optional<VFSError> error;

  private:
	FileHandle& mFile;
	CancellationToken mToken;
	ios_size mPosition{ 0 };
	ios_size mSize{ 0 };
};

} // namespace

Transfer ZipCreation::start( VirtualFileSystem& vfs, std::shared_ptr<ThreadPool> pool,
							 std::vector<URI> sources, URI destination ) {
	auto state = std::make_shared<detail::TransferState>();
	Transfer transfer( state );
	if ( !pool ) {
		state->complete( Err(
			VFSError( VFSErrorCode::InvalidOperation, destination, "VFS is shutting down" ) ) );
		return transfer;
	}
	pool->run( [&vfs, state, sources = std::move( sources ),
				destination = std::move( destination )]() mutable {
		ZipCreation job( vfs, state, std::move( sources ), std::move( destination ) );
		state->complete( job.run() );
	} );
	return transfer;
}

ZipCreation::ZipCreation( VirtualFileSystem& vfs, std::shared_ptr<detail::TransferState> state,
						  std::vector<URI> sources, URI destination ) :
	mVfs( vfs ),
	mState( std::move( state ) ),
	mToken( mState->token() ),
	mSources( std::move( sources ) ),
	mDestination( std::move( destination ) ) {}

VFSResult<void> ZipCreation::scan( const URI& uri, std::string name, unsigned depth ) {
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( uri ) );
	if ( depth > 256 )
		return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
							  "Directory nesting exceeds ZIP creation limit" ) );
	// Portable archive paths have only relative components and forward-slash separators.
	const auto segment = uri.getLastPathSegment();
	if ( !VFSPath::validName( segment, uri.getParent() ) ||
		 segment.find_first_of( "\\:" ) != std::string::npos )
		return Err( VFSError( VFSErrorCode::InvalidURI, uri,
							  "Filename cannot be represented as a portable ZIP path" ) );
	auto stat = await( mVfs.stat( uri, StatOptions{ false }, mToken ) );
	if ( !stat )
		return Err( std::move( stat ).error() );
	auto info = std::move( stat ).value();
	if ( info.type != FileType::Regular && info.type != FileType::Directory &&
		 info.type != FileType::Symlink )
		return Err( VFSError( VFSErrorCode::Unsupported, uri, "Cannot compress this file type" ) );
	if ( info.type == FileType::Regular && !info.size() )
		return Err( VFSError( VFSErrorCode::Unsupported, uri, "File length is unavailable" ) );
	if ( info.isSymlink() && info.symlinkTarget.empty() )
		return Err(
			VFSError( VFSErrorCode::Unsupported, uri, "Symbolic link target is unavailable" ) );
	const bool directory = info.type == FileType::Directory;
	if ( !directory ) {
		Uint64 size = info.isSymlink() ? info.symlinkTarget.size() : *info.size();
		if ( size > static_cast<Uint64>( std::numeric_limits<ios_size>::max() ) ||
			 size > std::numeric_limits<Uint64>::max() - mResult.bytesTransferred )
			return Err( VFSError( VFSErrorCode::InvalidOperation, uri, "Selection is too large" ) );
		mResult.bytesTransferred += size;
		++mResult.filesTransferred;
	} else {
		++mResult.directoriesCreated;
	}
	Zip::EntryOptions metadata;
	metadata.modified = info.modifiedTime();
	metadata.permissions = info.permissions();
	metadata.symbolicLink = info.isSymlink();
	// Retain only the metadata needed by the writer, not owner/group or presentation fields.
	mEntries.push_back( { uri, directory ? name : std::move( name ),
						  std::move( info.symlinkTarget ), metadata, info.size().value_or( 0 ),
						  info.revision, info.type } );
	if ( !directory )
		return Ok();
	// Directory handles keep traversal memory bounded by the plan and one listing page.
	auto opened = await( mVfs.openDirectory(
		uri, DirectoryOpenOptions{ DirectoryMetadata::Storage, false }, mToken ) );
	if ( !opened )
		return Err( std::move( opened ).error() );
	auto handle = std::move( opened ).value();
	for ( ;; ) {
		auto page = await( handle.read( 128, mToken ) );
		if ( !page )
			return Err( std::move( page ).error() );
		for ( const auto& child : page.value().entries ) {
			if ( !VFSPath::validName( child.name, uri ) )
				return Err( VFSError( VFSErrorCode::InvalidURI, uri,
									  "Provider returned an invalid entry name" ) );
			auto result =
				scan( uri.withAppendedSegment( child.name ), name + '/' + child.name, depth + 1 );
			if ( !result )
				return result;
		}
		if ( page.value().endOfDirectory )
			break;
	}
	return await( handle.close() );
}

VFSResult<void> ZipCreation::checkSource( const Entry& entry ) {
	auto current = await( mVfs.stat( entry.uri, StatOptions{ false }, mToken ) );
	if ( !current )
		return Err( std::move( current ).error() );
	bool revisionChanged =
		!entry.revision.empty() && !entry.revision.matches( current.value().revision );
	if ( revisionChanged && entry.type == FileType::Directory && entry.uri.getScheme() == "file" ) {
		// A temporary created inside the selection changes its parent's directory revision.
		// The input plan predates that temporary, so neither it nor the output is archived.
		auto directory = await( mVfs.canonicalize( entry.uri, mToken ) );
		auto outputParent = await( mVfs.canonicalize( mDestination.getParent(), mToken ) );
		if ( directory && outputParent && directory.value() == outputParent.value() )
			revisionChanged = false;
	}
	if ( mToken.isCancellationRequested() )
		return Err( cancelledError( entry.uri ) );
	if ( current.value().type != entry.type || revisionChanged ||
		 ( entry.type == FileType::Regular && current.value().size() != entry.size ) ||
		 current.value().symlinkTarget != entry.link )
		return Err( VFSError( VFSErrorCode::ChangedExternally, entry.uri,
							  "Source changed during ZIP creation" ) );
	return Ok();
}

VFSResult<TransferResult> ZipCreation::run() {
	if ( mSources.empty() || mDestination.getScheme() != "file" ||
		 !VFSPath::validURI( mDestination ) )
		return Err( VFSError( VFSErrorCode::Unsupported, mDestination,
							  "ZIP creation requires a selection and a local destination" ) );
	auto provider = mVfs.providerFor( mDestination );
	if ( !provider )
		return Err( VFSError( VFSErrorCode::Unsupported, mDestination ) );
	auto existing = await( mVfs.stat( mDestination, StatOptions{ false }, mToken ) );
	if ( existing )
		return Err( VFSError( VFSErrorCode::AlreadyExists, mDestination,
							  "Choose a different archive name" ) );
	if ( !existing.error().is( VFSErrorCode::NotFound ) )
		return Err( std::move( existing ).error() );
	UnorderedSet<std::string> names;
	for ( const auto& source : mSources ) {
		if ( !names.insert( source.getLastPathSegment() ).second )
			return Err( VFSError( VFSErrorCode::AlreadyExists, source,
								  "Selected entries have duplicate archive names" ) );
		auto result = scan( source, source.getLastPathSegment(), 0 );
		if ( !result )
			return Err( std::move( result ).error() );
	}
	mState->updateProgress(
		[&]( TransferProgress& progress ) {
			progress.currentDestination = mDestination;
			progress.bytesTotal = mResult.bytesTransferred;
			progress.filesTotal = mResult.filesTransferred;
		},
		true );
	FileWriter::ReplaceOptions options;
	options.overwrite = false;
	options.fsync = true;
	auto result = FileWriter::replace(
		*provider, mDestination, options, mToken,
		[&]( FileHandle& handle ) -> VFSResult<void> {
			ZipOutput output( handle, mToken );
			Zip archive( false );
			auto error = [&]() -> VFSResult<void> {
				archive.discard();
				if ( mToken.isCancellationRequested() )
					return Err( cancelledError( mDestination ) );
				if ( mSourceError )
					return Err( *mSourceError );
				if ( output.error )
					return Err( *output.error );
				return Err( VFSError( VFSErrorCode::IO, mDestination, archive.errorMessage() ) );
			};
			if ( !archive.create( output ) || !archive.setCancellationToken( mToken ) ||
				 !archive.setProgressCallback( [this]( double fraction ) {
					 mState->updateProgress( [&]( TransferProgress& progress ) {
						 progress.bytesDone =
							 static_cast<Uint64>( fraction * mResult.bytesTransferred );
					 } );
				 } ) )
				return error();
			for ( size_t i = 0; i < mEntries.size(); ++i ) {
				if ( mToken.isCancellationRequested() )
					return error();
				const auto& entry = mEntries[i];
				const auto& metadata = entry.options;
				bool added;
				if ( entry.type == FileType::Directory ) {
					added = archive.addDirectory( entry.name, metadata );
				} else if ( entry.options.symbolicLink ) {
					const auto& target = entry.link;
					added = archive.addData(
						{ reinterpret_cast<const Uint8*>( target.data() ), target.size() },
						entry.name, metadata );
				} else {
					added = archive.addStream(
						[this, i]() -> std::unique_ptr<IOStream> {
							const auto& entry = mEntries[i];
							auto checked = checkSource( entry );
							if ( !checked ) {
								mSourceError = std::move( checked ).error();
								return nullptr;
							}
							mState->updateProgress( [&]( TransferProgress& progress ) {
								progress.currentSource = entry.uri;
							} );
							auto stream = await( mVfs.openStream( entry.uri, {}, mToken ) );
							if ( !stream ) {
								mSourceError = std::move( stream ).error();
								return nullptr;
							}
							return std::make_unique<ZipInput>( std::move( stream ).value(),
															   mSourceError );
						},
						entry.size, entry.name, metadata );
				}
				if ( !added )
					return error();
			}
			if ( !archive.close() )
				return error();
			return Ok();
		},
		[&]() -> VFSResult<void> {
			// A source replaced while compression ran must not produce a successful archive.
			for ( const auto& entry : mEntries ) {
				auto checked = checkSource( entry );
				if ( !checked )
					return checked;
			}
			return Ok();
		} );
	if ( !result )
		return Err( std::move( result ).error() );
	mState->updateProgress(
		[&]( TransferProgress& progress ) {
			progress.bytesDone = mResult.bytesTransferred;
			progress.filesDone = mResult.filesTransferred;
		},
		true );
	return Ok( mResult );
}

}} // namespace EE::VFS
