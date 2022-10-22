#include <cstring>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>

namespace EE { namespace System {

IOStreamString::IOStreamString() {}

IOStreamString::IOStreamString( const std::string& filepath ) {
	FileSystem::fileGet( filepath, mStream );
}

ios_size IOStreamString::read( char* data, ios_size size ) {
	Int64 endPosition = mPos + size;
	Int64 count = endPosition <= getSize() ? size : getSize() - mPos;

	if ( count > 0 ) {
		memcpy( data, &mStream[mPos], static_cast<std::size_t>( count ) );
		mPos += count;
	}

	return count;
}

ios_size IOStreamString::write( const char* data, ios_size size ) {
	mStream.insert( mPos, data, size );

	mPos += size;

	return size;
}

ios_size IOStreamString::write( const std::string& string ) {
	return write( string.c_str(), string.size() );
}

ios_size IOStreamString::seek( ios_size position ) {
	mPos = ( position < getSize() ) ? position : getSize();
	return mPos;
}

ios_size IOStreamString::tell() {
	return getSize();
}

ios_size IOStreamString::getSize() {
	return mStream.size();
}

bool IOStreamString::isOpen() {
	return true;
}

void IOStreamString::clear() {
	mStream.clear();
	mPos = 0;
}

const char* IOStreamString::getPositionPointer() {
	return &mStream[mPos];
}

const char* IOStreamString::getStreamPointer() const {
	return mStream.c_str();
}

const std::string& IOStreamString::getStream() const {
	return mStream;
}

}} // namespace EE::System
