#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace System {

IOStreamFile::IOStreamFile( const std::string& path, std::ios_base::openmode mode ) :
	mSize(0)
{
	mFS.open( path.c_str(), mode );
}

IOStreamFile::~IOStreamFile() {
	if ( IsOpen() ) {
		mFS.close();
	}
}

ios_size IOStreamFile::Read( char * data, ios_size size ) {
	if ( IsOpen() ) {
		mFS.read( data, size );
	}

	return size;
}

ios_size IOStreamFile::Write( const char * data, ios_size size ) {
	if ( IsOpen() ) {
		mFS.write( data, size );
	}

	return size;
}

ios_size IOStreamFile::Seek( ios_size position ) {
	if ( IsOpen() ) {
		mFS.seekg( position , std::ios::beg );
	}

	return position;
}

ios_size IOStreamFile::Tell() {
	return mFS.tellg();
}

ios_size IOStreamFile::GetSize() {
	if ( IsOpen() ) {
		if ( 0 == mSize ) {
			ios_size Pos = Tell();

			mFS.seekg ( 0, std::ios::end );

			mSize = mFS.tellg();

			mFS.seekg ( Pos, std::ios::beg );
		}

		return mSize;
	}

	return 0;
}

bool IOStreamFile::IsOpen() {
	return mFS.is_open();
}

void IOStreamFile::Flush() {
	mFS.flush();
}

}}
