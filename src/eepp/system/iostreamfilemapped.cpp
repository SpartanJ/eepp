#include <eepp/system/iostreamfilemapped.hpp>

#include <algorithm>
#include <cstring>
#include <limits>

namespace EE { namespace System {

IOStreamFileMapped::IOStreamFileMapped() = default;

IOStreamFileMapped::IOStreamFileMapped( const std::string& path ) {
	open( path );
}

IOStreamFileMapped::IOStreamFileMapped( FileMapped&& file ) :
	mFile( std::move( file ) ), mPosition( 0 ) {}

bool IOStreamFileMapped::open( const std::string& path ) {
	mPosition = 0;
	return mFile.open( path, FileMapped::Mode::Read );
}

void IOStreamFileMapped::close() {
	mFile.close();
	mPosition = 0;
}

ios_size IOStreamFileMapped::read( char* data, ios_size size ) {
	if ( !isOpen() || nullptr == data || size <= 0 )
		return 0;

	const ios_size fileSize = getSize();

	if ( mPosition >= fileSize )
		return 0;

	const ios_size remaining = fileSize - mPosition;
	const ios_size toRead = std::min( size, remaining );

	if ( toRead <= 0 )
		return 0;

	std::memcpy( data, mFile.cdata() + static_cast<size_t>( mPosition ),
				 static_cast<size_t>( toRead ) );

	mPosition += toRead;

	return toRead;
}

ios_size IOStreamFileMapped::write( const char*, ios_size ) {
	return -1;
}

ios_size IOStreamFileMapped::seek( ios_size position ) {
	if ( !isOpen() )
		return -1;

	const ios_size fileSize = getSize();

	if ( position < 0 )
		position = 0;
	else if ( position > fileSize )
		position = fileSize;

	mPosition = position;

	return mPosition;
}

ios_size IOStreamFileMapped::tell() {
	return isOpen() ? mPosition : -1;
}

ios_size IOStreamFileMapped::getSize() {
	if ( !isOpen() )
		return -1;

	const size_t size = mFile.size();

	if ( size > static_cast<size_t>( std::numeric_limits<ios_size>::max() ) )
		return -1;

	return static_cast<ios_size>( size );
}

bool IOStreamFileMapped::isOpen() {
	return mFile.isOpen();
}

}} // namespace EE::System
