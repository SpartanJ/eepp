#include "crc4.hpp"

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

void cRC4::SetKey( const std::vector<Uint8>& Key ) {
	Uint8 a = 0;
	eeInt i;

	if ( Key.size() < 1 )
		return;

	for( i = 0; i < 256; i++)
		mKey.state[i] = i;

	for( i = 0; i < 256; i++ ) {
		a = ( a  + mKey.state[i] + Key[ i % Key.size() ] ) % 256;
		Swap( mKey.state[i], mKey.state[a] );
	}
}

void cRC4::EncryptByte( std::vector<Uint8>& buffer ) {
	Uint8 x = 0;
	Uint8 y = 0;
	Uint8 xorIndex;
	Uint32 i;
	RC4Key tKey;

	memcpy( &tKey, &mKey, sizeof(RC4Key) );

	for( i = 0; i < buffer.size(); i++) {
		x = (x + 1) % 256;
		y = ( tKey.state[x] + y ) % 256;
		Swap( tKey.state[x], tKey.state[y] );

		xorIndex = ( tKey.state[x] + tKey.state[y] ) % 256;

		buffer[i] ^= tKey.state[xorIndex];
	}
}

void cRC4::EncryptString( std::string& buffer ) {
	std::vector<Uint8> intbuf = stringToUint8( buffer );

	EncryptByte( intbuf );

	buffer = Uint8Tostring( intbuf );
}

bool cRC4::EncryptFile( const std::string& SourceFile, const std::string& DestFile ) {
	std::fstream fs;
	std::vector<Uint8> tmpv;

	if ( !FileExists( SourceFile ) )
		return false;

	tmpv.resize( FileSize( SourceFile ) );

	fs.open( SourceFile.c_str() , std::ios::in | std::ios::out | std::ios::binary );
	fs.read( reinterpret_cast<char*>(&tmpv[0]), (std::streamsize)tmpv.size() );

	EncryptByte( tmpv );

	if ( SourceFile != DestFile ) {
		std::fstream fs2;
		fs2.open( DestFile.c_str() , std::ios::out | std::ios::binary );
		fs2.write( reinterpret_cast<const char*>( &tmpv[0] ), (std::streamsize)tmpv.size() );
		fs2.close();
	} else {
		fs.seekg( 0, ios::beg );
		fs.write( reinterpret_cast<const char*>( &tmpv[0] ), (std::streamsize)tmpv.size() );
	}

	fs.close();

	return true;
}

void cRC4::EncryptByte( std::vector<Uint8>& buffer, const std::vector<Uint8>& Key ) {
	if ( Key.size() > 0 )
		SetKey( Key );

	EncryptByte( buffer );
}

void cRC4::EncryptString( std::string& buffer, const std::vector<Uint8>& Key ) {
	if ( Key.size() > 0 )
		SetKey( Key );

	EncryptString( buffer );
}

bool cRC4::EncryptFile( const std::string& SourceFile, const std::string& DestFile, const std::vector<Uint8>& Key ) {
	if ( Key.size() > 0 )
		SetKey( Key );

	return EncryptFile( SourceFile, DestFile );
}

void cRC4::SetKey( const std::string& Key ) {
	SetKey( stringToUint8( Key ) );
}

void cRC4::EncryptByte( std::vector<Uint8>& buffer, const std::string& Key ) {
	EncryptByte( buffer, stringToUint8( Key ) );
}

void cRC4::DecryptByte( std::vector<Uint8>& buffer, const std::vector<Uint8>& Key ) {
	EncryptByte( buffer, Key );
}

void cRC4::DecryptByte( std::vector<Uint8>& buffer, const std::string& Key ) {
	EncryptByte( buffer, stringToUint8( Key ) );
}

void cRC4::DecryptByte( std::vector<Uint8>& buffer ) {
	EncryptByte( buffer );
}

void cRC4::EncryptString( std::string& buffer, const std::string& Key ) {
	EncryptString( buffer, stringToUint8( Key ) );
}

void cRC4::DecryptString( std::string& buffer, const std::vector<Uint8>& Key ) {
	EncryptString( buffer, Key );
}

void cRC4::DecryptString( std::string& buffer, const std::string& Key ) {
	EncryptString( buffer, Key );
}

void cRC4::DecryptString( std::string& buffer ) {
	EncryptString( buffer );
}

bool cRC4::EncryptFile( const std::string& SourceFile, const std::string& DestFile, const std::string& Key ) {
	return EncryptFile( SourceFile, DestFile, stringToUint8( Key ) );
}

bool cRC4::DecryptFile( const std::string& SourceFile, const std::string& DestFile ) {
	return EncryptFile( SourceFile, DestFile );
}

bool cRC4::DecryptFile( const std::string& SourceFile, const std::string& DestFile, const std::vector<Uint8>& Key ) {
	return EncryptFile( SourceFile, DestFile, Key );
}

bool cRC4::DecryptFile( const std::string& SourceFile, const std::string& DestFile, const std::string& Key ) {
	return EncryptFile( SourceFile, DestFile, Key );
}

}}
