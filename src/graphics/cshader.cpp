#include "cshader.hpp"

namespace EE { namespace Graphics {

cShader::cShader( const Uint32& Type ) {
	Init( Type );
}

cShader::cShader( const Uint32& Type, const std::string& Filename ) {
    Init( Type );

	std::fstream fs;
	fs.open( Filename.c_str(), std::ios::in | std::ios::binary );

	if ( !fs.is_open() ) {
		cLog::instance()->Write( std::string( "Couldn't open shader object: " ) + Filename );
	}

	fs.seekg ( 0, ios::end );
	Int32 Length = fs.tellg();
	fs.seekg ( 0, ios::beg );
	std::string Buffer( Length + 1, 0 );
	fs.read( reinterpret_cast<char*> ( &Buffer[0] ), Length );
	fs.close();

    SetSource( Buffer );

    Compile();
}

cShader::cShader( const Uint32& Type, const Uint8 * Data, const Uint32& DataSize ) {
	Init( Type );

	SetSource( Data, DataSize );

	Compile();
}

cShader::cShader( const Uint32& Type, cPack * Pack, const std::string& Filename ) {
	Init( Type );

	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( Filename ) ) {
		std::vector<Uint8> TempData;

		Pack->ExtractFileToMemory( Filename, TempData );

		SetSource( reinterpret_cast<Uint8*> ( &TempData[0] ), TempData.size() );
	}

	Compile();
}

cShader::~cShader() {
	if ( 0 != mGLId )
		glDeleteShader( mGLId );
}

void cShader::Init( const Uint32& Type ) {
	mType 		= Type;
	mValid 		= false;
	mCompiled 	= false;
	mGLId 		= glCreateShader( mType );
}

void cShader::Reload() {
	Init( mType );

	SetSource( mSource );

	Compile();
}

void cShader::SetSource( const std::string& Source ) {
	if ( IsCompiled() ) {
		cLog::instance()->Write( "Can't set source for compiled shaders" );
		return;
	}

	mSource = Source;

	const char * src = reinterpret_cast<const char *> ( &Source[0] );

	glShaderSource( mGLId, 1, &src, NULL );
}

void cShader::SetSource( const Uint8 * Data, const Uint32& DataSize ) {
	std::string _dst( DataSize, 0 );

	memcpy( reinterpret_cast<void*>( &_dst[0] ), reinterpret_cast<const void*>( &Data[0] ), DataSize );

	SetSource( _dst );
}

void cShader::SetSource( const std::vector<Uint8>& Source ) {
	std::string _dst( Source.size(), 0 );

    memcpy( reinterpret_cast<void*>( &_dst[0] ), reinterpret_cast<const void*>( &Source[0] ), Source.size() );

    SetSource( _dst );
}

bool cShader::Compile() {
	if ( IsCompiled() ) {
		cLog::instance()->Write( "Can't compile a shader twice" );
		return false;
	}

	glCompileShader( GetId() );
	mCompiled = true;

	int Compiled;
	glGetShaderiv( GetId(), GL_COMPILE_STATUS, &Compiled );
	mValid = 0 != Compiled;

	if ( !mValid ) {
		GLsizei logsize, logarraysize;
		glGetShaderiv( GetId(), GL_INFO_LOG_LENGTH, &logarraysize );

		mCompileLog.resize( logarraysize - 1 );

		glGetShaderInfoLog( GetId(), logarraysize, &logsize, reinterpret_cast<GLchar*>( &mCompileLog[0] ) );

		cLog::instance()->Write( "Couldn't compile shader. Log follows:" );
		cLog::instance()->Write( mCompileLog );
		cLog::instance()->Write( mSource );
	} else {
		cLog::instance()->Write( "Shader Loaded Succesfully" );
	}

	return mValid;
}

cVertexShader::cVertexShader() :
	cShader( GL_VERTEX_SHADER )
{
}

cVertexShader::cVertexShader( const std::string& Filename ) :
	cShader( GL_VERTEX_SHADER, Filename )
{
}

cVertexShader::cVertexShader( const Uint8 * Data, const Uint32& DataSize ) :
	cShader( GL_VERTEX_SHADER, Data, DataSize )
{
}

cVertexShader::cVertexShader( cPack * Pack, const std::string& Filename ) :
	cShader( GL_VERTEX_SHADER, Pack, Filename )
{
}

cFragmentShader::cFragmentShader() :
	cShader(GL_FRAGMENT_SHADER)
{
}

cFragmentShader::cFragmentShader( const std::string& Filename ) :
	cShader( GL_FRAGMENT_SHADER, Filename )
{
}

cFragmentShader::cFragmentShader( const Uint8 * Data, const Uint32& DataSize ) :
	cShader( GL_FRAGMENT_SHADER, Data, DataSize )
{
}

cFragmentShader::cFragmentShader( cPack * Pack, const std::string& Filename ) :
	cShader( GL_FRAGMENT_SHADER, Pack, Filename )
{
}

}}
