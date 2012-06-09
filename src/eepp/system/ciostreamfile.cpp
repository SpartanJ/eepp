#include <eepp/system/ciostreamfile.hpp>

namespace EE { namespace System {

cIOStreamFile::cIOStreamFile( const std::string& path, std::ios_base::openmode mode ) {
	mFS.open( path.c_str(), mode );
}

cIOStreamFile::~cIOStreamFile() {
	if ( IsOpen() ) {
		mFS.close();
	}
}

ios_size cIOStreamFile::Read( char * data, ios_size size ) {
	if ( IsOpen() ) {
		mFS.read( data, size );
	}

	return size;
}

ios_size cIOStreamFile::Write( const char * data, ios_size size ) {
	if ( IsOpen() ) {
		mFS.write( data, size );
	}

	return size;
}

ios_size cIOStreamFile::Seek( ios_size position ) {
	if ( IsOpen() ) {
		mFS.seekg( position , std::ios::beg );
	}

	return position;
}

ios_size cIOStreamFile::GetPosition() {
	return mFS.tellg();
}

ios_size cIOStreamFile::GetSize() {
	if ( IsOpen() ) {
		ios_size Pos = GetPosition();

		mFS.seekg ( 0, std::ios::end );

		ios_size Length = mFS.tellg();

		mFS.seekg ( Pos, std::ios::beg );

		return Length;
	}

	return 0;
}

bool cIOStreamFile::IsOpen() {
	return mFS.is_open();
}

}}
