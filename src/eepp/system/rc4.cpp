#include <eepp/system/rc4.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace System {

RC4::RC4()
{}

RC4::~RC4()
{}

void RC4::swap( Uint8& a, Uint8& b ) {
	Uint8 tmpb;
	tmpb = a;
	a = b;
	b = tmpb;
}

void RC4::setKey( const Uint8 * key, Uint32 size ) {
	Uint8 a = 0;
	int i;

	for( i = 0; i < 256; i++)
		mKey.state[i] = i;

	for( i = 0; i < 256; i++ ) {
		a = ( a  + mKey.state[i] + key[ i % size ] ) % 256;

		swap( mKey.state[i], mKey.state[a] );
	}
}

void RC4::setKey( const std::vector<Uint8>& Key ) {
	setKey( reinterpret_cast<const Uint8*>( &Key[0] ), Key.size() );
}

void RC4::setKey( const std::string& Key ) {
	setKey( reinterpret_cast<const Uint8*>( &Key[0] ), Key.size() );
}

void RC4::encryptByte( Uint8 * data, Uint32 size ) {
	Uint8 x = 0;
	Uint8 y = 0;
	Uint8 xorIndex;
	Uint32 i;
	RC4Key tKey;

	memcpy( &tKey, &mKey, sizeof(RC4Key) );

	for( i = 0; i < size; i++ ) {
		x = (x + 1) % 256;
		y = ( tKey.state[x] + y ) % 256;

		swap( tKey.state[x], tKey.state[y] );

		xorIndex = ( tKey.state[x] + tKey.state[y] ) % 256;

		data[i] ^= tKey.state[xorIndex];
	}
}

void RC4::encryptByte( std::vector<Uint8>& buffer ) {
	encryptByte( reinterpret_cast<Uint8*>( &buffer[0] ), buffer.size() );
}

void RC4::encryptString( std::string& buffer ) {
	encryptByte( reinterpret_cast<Uint8*>( &buffer[0] ), buffer.size() );
}

bool RC4::encryptFile( const std::string& SourceFile, const std::string& DestFile ) {
	if ( !FileSystem::fileExists( SourceFile ) )
		return false;

	SafeDataPointer data;

	FileSystem::fileGet( SourceFile, data );

	encryptByte( data.Data, data.DataSize );

	FileSystem::fileWrite( DestFile, data.Data, data.DataSize );

	return true;
}

void RC4::decryptByte( std::vector<Uint8>& buffer ) {
	encryptByte( buffer );
}

void RC4::decryptString( std::string& buffer ) {
	encryptString( buffer );
}

bool RC4::decryptFile( const std::string& SourceFile, const std::string& DestFile ) {
	return encryptFile( SourceFile, DestFile );
}

}}
