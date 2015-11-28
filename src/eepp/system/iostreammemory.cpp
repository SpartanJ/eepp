#include <eepp/system/iostreammemory.hpp>

namespace EE { namespace System {

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

ios_size IOStreamMemory::Read( char * data, ios_size size ) {
	if ( mPos + size <= mSize ) {
		memcpy( data, mReadPtr + mPos, size );

		mPos += size;

		return size;
	} else if ( mPos != mSize ) {
		memcpy( data, mReadPtr + mPos, mSize - mPos );

		mPos = mSize;

		return size;
	}

	return 0;
}

ios_size IOStreamMemory::Write( const char * data, ios_size size ) {
	if ( NULL != mWritePtr && mPos + size <= mSize ) {
		memcpy( mWritePtr + mPos, data, size );

		mPos += size;
	}

	return mPos;
}

ios_size IOStreamMemory::Seek( ios_size position ) {
	if ( position < mSize )
		mPos = position;

	return mPos;
}

ios_size IOStreamMemory::Tell() {
	return mPos;
}

ios_size IOStreamMemory::GetSize() {
	return mSize;
}

bool IOStreamMemory::IsOpen() {
	return NULL != mReadPtr;
}

}}
