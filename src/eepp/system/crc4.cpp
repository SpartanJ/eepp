#include <eepp/system/crc4.hpp>
#include <eepp/system/ciostreamfile.hpp>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace System {

cRC4::cRC4()
{}

cRC4::~cRC4()
{}

void cRC4::Swap( Uint8& a, Uint8& b ) {
	Uint8 tmpb;
	tmpb = a;
	a = b;
	b = tmpb;
}

void cRC4::SetKey( const Uint8 * key, Uint32 size ) {
	Uint8 a = 0;
	int i;

	for( i = 0; i < 256; i++)
		mKey.state[i] = i;

	for( i = 0; i < 256; i++ ) {
		a = ( a  + mKey.state[i] + key[ i % size ] ) % 256;

		Swap( mKey.state[i], mKey.state[a] );
	}
}

void cRC4::SetKey( const std::vector<Uint8>& Key ) {
	SetKey( reinterpret_cast<const Uint8*>( &Key[0] ), Key.size() );
}

void cRC4::SetKey( const std::string& Key ) {
	SetKey( reinterpret_cast<const Uint8*>( &Key[0] ), Key.size() );
}

void cRC4::EncryptByte( Uint8 * data, Uint32 size ) {
	Uint8 x = 0;
	Uint8 y = 0;
	Uint8 xorIndex;
	Uint32 i;
	RC4Key tKey;

	memcpy( &tKey, &mKey, sizeof(RC4Key) );

	for( i = 0; i < size; i++ ) {
		x = (x + 1) % 256;
		y = ( tKey.state[x] + y ) % 256;

		Swap( tKey.state[x], tKey.state[y] );

		xorIndex = ( tKey.state[x] + tKey.state[y] ) % 256;

		data[i] ^= tKey.state[xorIndex];
	}
}

void cRC4::EncryptByte( std::vector<Uint8>& buffer ) {
	EncryptByte( reinterpret_cast<Uint8*>( &buffer[0] ), buffer.size() );
}

void cRC4::EncryptString( std::string& buffer ) {
	EncryptByte( reinterpret_cast<Uint8*>( &buffer[0] ), buffer.size() );
}

bool cRC4::EncryptFile( const std::string& SourceFile, const std::string& DestFile ) {
	if ( !FileSystem::FileExists( SourceFile ) )
		return false;

	SafeDataPointer data;

	FileSystem::FileGet( SourceFile, data );

	EncryptByte( data.Data, data.DataSize );

	FileSystem::FileWrite( DestFile, data.Data, data.DataSize );

	return true;
}

void cRC4::DecryptByte( std::vector<Uint8>& buffer ) {
	EncryptByte( buffer );
}

void cRC4::DecryptString( std::string& buffer ) {
	EncryptString( buffer );
}

bool cRC4::DecryptFile( const std::string& SourceFile, const std::string& DestFile ) {
	return EncryptFile( SourceFile, DestFile );
}

}}
