#include <eepp/vfs/iostreamvfs.hpp>

#include <algorithm>
#include <cstring>
#include <eepp/system/scopedop.hpp>
#include <eepp/vfs/detail/await.hpp>

namespace EE { namespace VFS {

using detail::await;

VFSResult<std::unique_ptr<IOStreamVFS>>
IOStreamVFS::create( FileHandle handle, OpenFlags flags, CancellationToken token, Uint64 maxSize ) {
	System::ScopedOp cleanup( nullptr, [&handle] {
		if ( handle.isOpen() )
			handle.close();
	} );
	if ( token.isCancellationRequested() )
		return Err( cancelledError( handle.uri() ) );
	auto info = await( handle.stat() );
	if ( !info )
		return Err( std::move( info ).error() );
	if ( info.value().isDirectory() )
		return Err( VFSError( VFSErrorCode::IsDirectory, handle.uri() ) );
	const auto size = info.value().size();
	if ( !size ) {
		return Err( VFSError( VFSErrorCode::Unsupported, handle.uri(),
							  "A sized stream requires the file length" ) );
	}
	if ( *size > maxSize || *size > Unlimited ) {
		return Err( VFSError( VFSErrorCode::InvalidOperation, handle.uri(),
							  "File exceeds the maximum stream size" ) );
	}
	if ( token.isCancellationRequested() )
		return Err( cancelledError( handle.uri() ) );
	return Ok( std::unique_ptr<IOStreamVFS>( new IOStreamVFS(
		std::move( handle ), static_cast<ios_size>( *size ), flags, std::move( token ) ) ) );
}

IOStreamVFS::IOStreamVFS( FileHandle handle, ios_size size, OpenFlags flags,
						  CancellationToken token ) :
	mHandle( std::move( handle ) ), mToken( std::move( token ) ), mSize( size ), mFlags( flags ) {}

IOStreamVFS::~IOStreamVFS() {
	// FileHandle::close() transfers ownership to the provider's cleanup task.
	if ( mHandle.isOpen() )
		mHandle.close();
}

void IOStreamVFS::fail( VFSError error ) {
	if ( !mError )
		mError = std::move( error );
}

bool IOStreamVFS::check() {
	if ( !isOpen() )
		return false;
	if ( mToken.isCancellationRequested() ) {
		fail( cancelledError( mHandle.uri() ) );
		return false;
	}
	return true;
}

ios_size IOStreamVFS::read( char* data, ios_size size ) {
	if ( size <= 0 || !check() )
		return 0;
	if ( !data || !hasFlag( mFlags, OpenFlags::Read ) ) {
		fail( VFSError( VFSErrorCode::InvalidOperation, mHandle.uri(), "Stream is not readable" ) );
		return 0;
	}
	const ios_size requested = std::min( size, std::max<ios_size>( 0, mSize - mPosition ) );
	ios_size total = 0;
	while ( total < requested && check() ) {
		const ios_size cached = mPosition - mBufferOffset;
		if ( cached >= 0 && static_cast<Uint64>( cached ) < mBufferValid ) {
			const size_t count = static_cast<size_t>( std::min<Uint64>(
				requested - total, mBufferValid - static_cast<size_t>( cached ) ) );
			std::memcpy( data + total, mBuffer.data() + cached, count );
			mPosition += count;
			total += count;
			continue;
		}
		constexpr size_t CacheSize = 64 * 1024;
		constexpr size_t ChunkSize = 256 * 1024;
		const bool buffered = requested - total < CacheSize;
		size_t count = static_cast<size_t>( std::min<ios_size>(
			buffered ? CacheSize : ChunkSize, buffered ? mSize - mPosition : requested - total ) );
		if ( buffered )
			mBuffer.resize( count );
		auto target = buffered ? mBuffer.data() : reinterpret_cast<Uint8*>( data + total );
		auto result = await( mHandle.read( mPosition, { target, count }, mToken ) );
		if ( !result ) {
			fail( std::move( result ).error() );
			break;
		}
		if ( result.value() == 0 || result.value() > count ) {
			fail( VFSError( VFSErrorCode::IO, mHandle.uri(),
							"Unexpected end of stream or invalid read count" ) );
			break;
		}
		if ( buffered ) {
			mBufferOffset = mPosition;
			mBufferValid = result.value();
		} else {
			mPosition += result.value();
			total += result.value();
		}
	}
	return total;
}

ios_size IOStreamVFS::write( const char* data, ios_size size ) {
	if ( size <= 0 || !check() )
		return 0;
	if ( !data || !hasFlag( mFlags, OpenFlags::Write ) ||
		 size > std::numeric_limits<ios_size>::max() - mPosition ) {
		fail( VFSError( VFSErrorCode::InvalidOperation, mHandle.uri(), "Invalid stream write" ) );
		return 0;
	}
	mBufferValid = 0;
	ios_size total = 0;
	while ( total < size && check() ) {
		const size_t count = static_cast<size_t>( std::min<ios_size>( 256 * 1024, size - total ) );
		auto result = await( mHandle.write(
			mPosition, { reinterpret_cast<const Uint8*>( data + total ), count }, mToken ) );
		if ( !result ) {
			fail( std::move( result ).error() );
			break;
		}
		if ( result.value() == 0 || result.value() > count ) {
			fail( VFSError( VFSErrorCode::IO, mHandle.uri(), "Invalid stream write count" ) );
			break;
		}
		mPosition += result.value();
		total += result.value();
		mSize = std::max( mSize, mPosition );
	}
	return total;
}

ios_size IOStreamVFS::seek( ios_size position ) {
	if ( !check() )
		return -1;
	if ( position < 0 ) {
		fail(
			VFSError( VFSErrorCode::InvalidOperation, mHandle.uri(), "Negative stream position" ) );
		return -1;
	}
	const bool cached = mBufferValid && position >= mBufferOffset &&
						static_cast<Uint64>( position - mBufferOffset ) <= mBufferValid;
	if ( position != mPosition && !cached && !mHandle.capabilities().has( VFSCapability::Seek ) ) {
		fail( VFSError( VFSErrorCode::Unsupported, mHandle.uri(),
						"Stream does not support seeking" ) );
		return -1;
	}
	mPosition = position;
	return mPosition;
}

ios_size IOStreamVFS::tell() {
	return mPosition;
}

ios_size IOStreamVFS::getSize() {
	return mSize;
}

bool IOStreamVFS::isOpen() {
	return mHandle.isOpen() && !mError;
}

VFSResult<void> IOStreamVFS::flush() {
	if ( !check() ) {
		if ( mError )
			return Err( *mError );
		return Err( VFSError( VFSErrorCode::InvalidOperation, mHandle.uri(), "Stream is closed" ) );
	}
	auto result = await( mHandle.flush() );
	if ( !result )
		fail( std::move( result ).error() );
	return mError ? VFSResult<void>( Err( *mError ) ) : VFSResult<void>( Ok() );
}

VFSResult<void> IOStreamVFS::close() {
	auto result = await( mHandle.close() );
	if ( !result )
		fail( std::move( result ).error() );
	mBufferValid = 0;
	mBuffer = std::vector<Uint8>{};
	return mError ? VFSResult<void>( Err( *mError ) ) : VFSResult<void>( Ok() );
}

}} // namespace EE::VFS
