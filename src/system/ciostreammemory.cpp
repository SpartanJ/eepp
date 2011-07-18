#include "ciostreammemory.hpp"

namespace EE { namespace System {

cIOStreamMemory::cIOStreamMemory( const char * data, ios_size size ) :
	mReadPtr( data ),
	mWritePtr( NULL ),
	mPos( 0 ),
	mSize( size )
{
}

cIOStreamMemory::cIOStreamMemory( char * data, ios_size size ) :
	mReadPtr( const_cast<const char*>( data ) ),
	mWritePtr( data ),
	mPos( 0 ),
	mSize( size )
{
}

cIOStreamMemory::~cIOStreamMemory() {
}

ios_size cIOStreamMemory::Read( char * data, ios_size size ) {
	if ( mPos + size <= mSize ) {
		memcpy( data, mReadPtr + mPos, size );

		mPos += size;
	}

	return mPos;
}

ios_size cIOStreamMemory::Write( const char * data, ios_size size ) {
	if ( NULL != mWritePtr && mPos + size <= mSize ) {
		memcpy( mWritePtr + mPos, data, size );

		mPos += size;
	}

	return mPos;
}

ios_size cIOStreamMemory::Seek( ios_size position ) {
	if ( position < mSize )
		mPos = position;

	return mPos;
}

ios_size cIOStreamMemory::GetPosition() {
	return mPos;
}

ios_size cIOStreamMemory::GetSize() {
	return mSize;
}

bool cIOStreamMemory::IsOpen() {
	return NULL != mReadPtr;
}

}}
