#include <eepp/system/iostreampak.hpp>
#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace System {

IOStreamPak * IOStreamPak::New( Pak * pack, const std::string& path, bool writeMode ) {
	return eeNew( IOStreamPak, ( pack, path, writeMode ) );
}

IOStreamPak::IOStreamPak( Pak * pack, const std::string & path, bool writeMode ) :
	mFile( NULL ),
	mPos( 0 ),
	mOpen( false )
{
	int index = -1;

	if ( -1 != ( index = pack->exists( path ) ) ) {
		mEntry = pack->getPackEntry( (Uint32)index );

		mFile = IOStreamFile::New( pack->getPackPath(), ( writeMode ? "wb" : "rb" ) );

		if ( mFile->isOpen() ) {
			mFile->seek( mEntry.file_position );
			mOpen = true;
		}
	}
}

IOStreamPak::~IOStreamPak() {
	eeSAFE_DELETE( mFile );
}

ios_size IOStreamPak::read( char * data, ios_size size ) {
	if ( isOpen() ) {
		mFile->read( data, size );

		mPos += size;
	}

	return size;
}

ios_size IOStreamPak::write( const char * data, ios_size size)  {
	if ( isOpen() && static_cast<Uint32>( mPos ) + size < mEntry.file_length ) {
		mFile->write( data, size );
	}

	return size;
}

ios_size IOStreamPak::seek(ios_size position) {
	if ( isOpen() ) {
		mFile->seek( mEntry.file_position + position );
		mPos = position;
	}

	return position;
}

ios_size IOStreamPak::tell() {
	return mPos;
}

ios_size IOStreamPak::getSize() {
	return isOpen() ? mEntry.file_length : 0;
}

bool IOStreamPak::isOpen() {
	return mOpen;
}

}}
