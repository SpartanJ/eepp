#include <eepp/system/iostreamzip.hpp>
#include <eepp/system/zip.hpp>
#include <libzip/zip.h>
#include <libzip/zipint.h>

namespace EE { namespace System {

IOStreamZip * IOStreamZip::New( Zip * pack, const std::string& path ) {
	return eeNew( IOStreamZip, ( pack, path ) );
}

IOStreamZip::IOStreamZip( Zip * pack, const std::string& path ) :
	mPath( path ),
	mZip( pack->getZip() ),
	mFile( NULL ),
	mPos( 0 )
{
	struct zip_stat zs;
	int err = zip_stat( mZip, path.c_str(), 0, &zs );

	if ( !err ) {
		mFile = zip_fopen_index( mZip, zs.index, 0 );
	}
}

IOStreamZip::~IOStreamZip() {
	if ( isOpen() ) {
		zip_fclose(mFile);
	}
}

ios_size IOStreamZip::read( char * data, ios_size size ) {
	int res = -1;

	if ( isOpen() ) {
		res = zip_fread( mFile, reinterpret_cast<void*> (&data[0]), size );

		if ( -1 != res ) {
			mPos += size;
		}
	}

	return -1 != res ? res : 0;
}

ios_size IOStreamZip::write( const char * data, ios_size size ) {
	return 0;
}

ios_size IOStreamZip::seek( ios_size position ) {
	if ( isOpen() && mPos != position ) {
		zip_fclose( mFile );

		struct zip_stat zs;
		int err = zip_stat( mZip, mPath.c_str(), 0, &zs );

		if ( !err ) {
			mFile = zip_fopen_index( mZip, zs.index, 0 );

			if ( 0 != position ) {
				ScopedBuffer ptr( position );
				read( (char*)ptr.get(), position );
			}

			mPos = position;

			return position;
		}
	}

	return 0;
}

ios_size IOStreamZip::tell() {
	return mPos;
}

ios_size IOStreamZip::getSize() {
	struct zip_stat zs;
	int err = zip_stat( mZip, mPath.c_str(), 0, &zs );
	return !err ? zs.size : 0;
}

bool IOStreamZip::isOpen() {
	return NULL != mFile;
}

}}
