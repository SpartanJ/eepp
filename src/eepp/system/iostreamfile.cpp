#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace System {

IOStreamFile::IOStreamFile( const std::string& path, std::ios_base::openmode mode ) :
	mSize(0)
{
	mFS.open( path.c_str(), mode );
}

IOStreamFile::~IOStreamFile() {
	if ( isOpen() ) {
		mFS.close();
	}
}

ios_size IOStreamFile::read( char * data, ios_size size ) {
	if ( isOpen() ) {
		mFS.read( data, size );
	}

	return size;
}

ios_size IOStreamFile::write( const char * data, ios_size size ) {
	if ( isOpen() ) {
		mFS.write( data, size );
	}

	return size;
}

ios_size IOStreamFile::seek( ios_size position ) {
	if ( isOpen() ) {
		mFS.seekg( position , std::ios::beg );
	}

	return position;
}

ios_size IOStreamFile::tell() {
	return mFS.tellg();
}

ios_size IOStreamFile::getSize() {
	if ( isOpen() ) {
		if ( 0 == mSize ) {
			ios_size Pos = tell();

			mFS.seekg ( 0, std::ios::end );

			mSize = mFS.tellg();

			mFS.seekg ( Pos, std::ios::beg );
		}

		return mSize;
	}

	return 0;
}

bool IOStreamFile::isOpen() {
	return mFS.is_open();
}

void IOStreamFile::flush() {
	mFS.flush();
}

}}
