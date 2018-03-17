#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace System {

IOStreamFile::IOStreamFile( const std::string& path, std::ios_base::openmode mode ) :
	mFS(NULL),
	mSize(0)
{
	mFS = std::fopen(path.c_str(), "rb");;
}

IOStreamFile::~IOStreamFile() {
	if ( isOpen() ) {
		std::fclose(mFS);
	}
}

ios_size IOStreamFile::read( char * data, ios_size size ) {
	if ( isOpen() ) {
		std::fread(data, 1, static_cast<std::size_t>(size), mFS);
	}

	return size;
}

ios_size IOStreamFile::write( const char * data, ios_size size ) {
	if ( isOpen() ) {
		std::fwrite( data, 1, size, mFS );
	}

	return size;
}

ios_size IOStreamFile::seek( ios_size position ) {
	if ( isOpen() ) {
		std::fseek( mFS, position, SEEK_SET );
	}

	return position;
}

ios_size IOStreamFile::tell() {
	if ( mFS ) {
		ios_size Pos = std::ftell( mFS );
		return Pos;
	}

	return -1;
}

ios_size IOStreamFile::getSize() {
	if ( isOpen() ) {
		if ( 0 == mSize && mFS ) {
			Int64 position = tell();

			std::fseek(mFS, 0, SEEK_END);

			mSize = tell();

			seek(position);
		}

		return mSize;
	}

	return 0;
}

bool IOStreamFile::isOpen() {
	return NULL != mFS;
}

void IOStreamFile::flush() {
	if ( mFS )
		std::fflush( mFS );
}

}}
