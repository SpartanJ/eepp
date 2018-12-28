#include <eepp/system/iostreammemory.hpp>
#include <eepp/core/memorymanager.hpp>
#include <cstring>

namespace EE { namespace System {

IOStreamMemory * IOStreamMemory::New( const char * data, ios_size size ) {
	return eeNew( IOStreamMemory, ( data, size ) );
}

IOStreamMemory * IOStreamMemory::New( char * data, ios_size size ) {
	return eeNew( IOStreamMemory, ( data, size ) );
}

IOStreamMemory::IOStreamMemory( const char * data, ios_size size ) :
	mReadPtr( data ),
	mWritePtr( NULL ),
	mPos( 0 ),
	mSize( size )
{
}

IOStreamMemory::IOStreamMemory( char * data, ios_size size ) :
	mReadPtr( const_cast<const char*>( data ) ),
	mWritePtr( data ),
	mPos( 0 ),
	mSize( size )
{
}

IOStreamMemory::~IOStreamMemory() {
}

ios_size IOStreamMemory::read( char * data, ios_size size ) {
	Int64 endPosition = mPos + size;
	Int64 count = endPosition <= mSize ? size : mSize - mPos;

	if ( count > 0 ) {
		memcpy( data, mReadPtr + mPos, static_cast<std::size_t>( count ) );
		mPos += count;
	}

	return count;
}

ios_size IOStreamMemory::write( const char * data, ios_size size ) {
	if ( NULL != mWritePtr && mPos + size <= mSize ) {
		memcpy( mWritePtr + mPos, data, size );

		mPos += size;
	}

	return mPos;
}

ios_size IOStreamMemory::seek( ios_size position ) {
	mPos = ( position < mSize ) ? position : mSize;

	return mPos;
}

ios_size IOStreamMemory::tell() {
	return mPos;
}

ios_size IOStreamMemory::getSize() {
	return mSize;
}

bool IOStreamMemory::isOpen() {
	return NULL != mReadPtr;
}

}}
