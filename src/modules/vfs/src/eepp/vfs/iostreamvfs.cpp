#include <algorithm>
#include <cstring>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/iostreamvfs.hpp>

namespace EE { namespace VFS {

using detail::await;

IOStreamVFS::IOStreamVFS( VirtualFileSystem& vfs, const URI& uri, OpenFlags flags,
						  size_t readAheadSize ) :
	mReadAhead( std::max<size_t>( 1, readAheadSize ) ) {
	auto opened = await( vfs.open( uri, flags ) );
	if ( opened )
		mHandle = std::move( opened ).value();
	else
		mError = std::move( opened ).error();
}

IOStreamVFS::~IOStreamVFS() {
	close();
}

void IOStreamVFS::close() {
	if ( mHandle.isOpen() )
		await( mHandle.close() );
}

ios_size IOStreamVFS::read( char* data, ios_size size ) {
	if ( !mHandle.isOpen() || size <= 0 )
		return 0;
	size_t wanted = static_cast<size_t>( size );
	size_t done = 0;
	while ( done < wanted ) {
		// Serve from the read-ahead buffer when the position falls inside it.
		if ( mPosition >= mBufferOffset && mPosition < mBufferOffset + mBufferLength ) {
			size_t offset = static_cast<size_t>( mPosition - mBufferOffset );
			size_t count = std::min( wanted - done, mBufferLength - offset );
			std::memcpy( data + done, mBuffer.data() + offset, count );
			done += count;
			mPosition += count;
			continue;
		}
		size_t remaining = wanted - done;
		if ( remaining >= mReadAhead ) {
			// Large reads go straight into the caller's buffer.
			auto read = await(
				mHandle.read( mPosition, std::span<Uint8>( reinterpret_cast<Uint8*>( data + done ),
														   remaining ) ) );
			if ( !read ) {
				mError = std::move( read ).error();
				break;
			}
			if ( read.value() == 0 )
				break;
			done += read.value();
			mPosition += read.value();
			continue;
		}
		mBuffer.resize( mReadAhead );
		auto read = await( mHandle.read( mPosition, mBuffer ) );
		if ( !read ) {
			mError = std::move( read ).error();
			mBufferLength = 0;
			break;
		}
		mBufferOffset = mPosition;
		mBufferLength = read.value();
		if ( mBufferLength == 0 )
			break;
	}
	return static_cast<ios_size>( done );
}

ios_size IOStreamVFS::write( const char* data, ios_size size ) {
	if ( !mHandle.isOpen() || size <= 0 )
		return 0;
	// Writes invalidate the read-ahead window.
	mBufferLength = 0;
	auto written = await(
		mHandle.write( mPosition, std::span<const Uint8>( reinterpret_cast<const Uint8*>( data ),
														  static_cast<size_t>( size ) ) ) );
	if ( !written ) {
		mError = std::move( written ).error();
		return 0;
	}
	mPosition += written.value();
	if ( mSize && mPosition > *mSize )
		mSize = mPosition;
	return static_cast<ios_size>( written.value() );
}

ios_size IOStreamVFS::seek( ios_size position ) {
	mPosition = position < 0 ? 0 : static_cast<Uint64>( position );
	return static_cast<ios_size>( mPosition );
}

ios_size IOStreamVFS::getSize() {
	if ( !mSize && mHandle.isOpen() ) {
		auto info = await( mHandle.stat() );
		if ( info && info.value().size() )
			mSize = *info.value().size();
		else if ( !info )
			mError = std::move( info ).error();
	}
	return mSize ? static_cast<ios_size>( *mSize ) : 0;
}

}} // namespace EE::VFS
