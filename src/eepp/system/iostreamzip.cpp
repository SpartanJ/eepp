#include <eepp/system/iostreamzip.hpp>

#include <algorithm>
#include <array>
#include <eepp/system/zip.hpp>
#include <libzip/zip.h>
#include <limits>

namespace EE { namespace System {

IOStreamZip* IOStreamZip::New( Zip* pack, const std::string& path ) {
	return eeNew( IOStreamZip, ( pack, path ) );
}

IOStreamZip::IOStreamZip( Zip* pack, const std::string& path ) :
	IOStreamZip( pack, static_cast<Uint64>( pack->exists( path ) ) ) {}

IOStreamZip::IOStreamZip( Zip* pack, Uint64 index ) :
	mZip( pack->getZip() ), mFile( nullptr ), mIndex( index ), mSize( 0 ), mPos( 0 ) {
	struct zip_stat info;
	zip_stat_init( &info );
	if ( mZip && zip_stat_index( mZip, index, 0, &info ) == 0 &&
		 info.size <= static_cast<Uint64>( std::numeric_limits<ios_size>::max() ) ) {
		mSize = static_cast<ios_size>( info.size );
		mFile = zip_fopen_index( mZip, index, 0 );
	}
}

IOStreamZip::~IOStreamZip() {
	if ( mFile )
		zip_fclose( mFile );
}

ios_size IOStreamZip::read( char* data, ios_size size ) {
	if ( !mFile || size < 0 )
		return -1;
	auto count = zip_fread( mFile, data, static_cast<Uint64>( size ) );
	if ( count > 0 ) {
		mPos += count;
		// libzip validates the CRC only on the read past EOF. Validate before reporting the
		// last bytes: offset-based callers may never request another read on this stream.
		if ( mPos >= mSize ) {
			char extra;
			if ( mPos != mSize || zip_fread( mFile, &extra, 1 ) != 0 )
				return -1;
		}
	}
	return count;
}

ios_size IOStreamZip::write( const char*, ios_size ) {
	return -1;
}

ios_size IOStreamZip::seek( ios_size position ) {
	if ( !mFile || position < 0 || position > mSize )
		return -1;
	if ( position < mPos ) {
		zip_fclose( mFile );
		mFile = zip_fopen_index( mZip, mIndex, 0 );
		mPos = 0;
		if ( !mFile )
			return -1;
	}
	std::array<char, 16384> scratch;
	while ( mPos < position ) {
		const auto count = std::min<ios_size>( position - mPos, scratch.size() );
		if ( read( scratch.data(), count ) <= 0 )
			return -1;
	}
	return mPos;
}

ios_size IOStreamZip::tell() {
	return mPos;
}

ios_size IOStreamZip::getSize() {
	return mSize;
}

bool IOStreamZip::isOpen() {
	return mFile != nullptr;
}

}} // namespace EE::System
