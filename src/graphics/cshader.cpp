#include "cshader.hpp"
#include "glhelper.hpp"
#include "renderer/crenderergl3.hpp"

namespace EE { namespace Graphics {

bool cShader::Ensure = true;

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

	fs.seekg ( 0, std::ios::end );
	Int32 Length = fs.tellg();
	fs.seekg ( 0, std::ios::beg );
	std::string Buffer( Length + 1, 0 );
	fs.read( reinterpret_cast<char*> ( &Buffer[0] ), Length );
	fs.close();

    SetSource( Buffer );

    Compile();
}

cShader::cShader( const Uint32& Type, const char * Data, const Uint32& DataSize ) {
	Init( Type );

	SetSource( Data, DataSize );

	Compile();
}

cShader::cShader( const Uint32& Type, cPack * Pack, const std::string& Filename ) {
	cPack::PointerData PData;

	Init( Type );

	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( Filename ) ) {
		Pack->ExtractFileToMemory( Filename, PData );

		SetSource( reinterpret_cast<char*> ( PData.Data ), PData.DataSize );
	}

	Compile();

	eeSAFE_DELETE( PData.Data );
}

cShader::cShader( const Uint32& Type, const char ** Data, const Uint32& NumLines ) {
	Init( Type );

	SetSource( Data, NumLines );

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

	cShader::Ensure = false;
	SetSource( mSource );
	cShader::Ensure = true;

	Compile();
}

void cShader::EnsureVersion() {
	#ifdef EE_GL3_ENABLED
	if ( cShader::Ensure && ( GLi->Version() == GLv_3 || GLi->Version() == GLv_ES2 ) ) {
		cLog::instance()->Write( "Shader " + toStr( mGLId ) + " converted to programmable pipeline automatically." );

		if ( GL_VERTEX_SHADER == mType ) {
			if ( mSource.find( "ftransform" ) != std::string::npos ) {
				mSource = GLi->GetRendererGL3()->GetBaseVertexShader();
			}
		} else {
			if ( mSource.find( "gl_FragColor" ) != std::string::npos ) {
				mSource = "#version 150\nuniform		int			dgl_TexActive = 1;\ninvariant in	vec4		gl_Color;\ninvariant in	vec4		gl_TexCoord[ 4 ];\nout				vec4		gl_FragColor;\n" + mSource;

				ReplaceSubStr( mSource, "gl_FragColor"	, "dgl_FragColor"	);
				ReplaceSubStr( mSource, "gl_Color"		, "dgl_Color"		);
				ReplaceSubStr( mSource, "gl_TexCoord"	, "dgl_TexCoord"	);
			}
		}
	}
	#endif
}

void cShader::SetSource( const std::string& Source ) {
	if ( IsCompiled() ) {
		cLog::instance()->Write( "Can't set source for compiled shaders" );
		return;
	}

	mSource = Source;

	EnsureVersion();

	const char * src = reinterpret_cast<const char *> ( &mSource[0] );

	glShaderSource( mGLId, 1, &src, NULL );
}

void cShader::SetSource( const char** Data, const Uint32& NumLines ) {
	std::string tstr;

	for ( Uint32 i = 0; i < NumLines; i++ ) {
		tstr += std::string( Data[i] );
	}

	SetSource( tstr );
}

void cShader::SetSource( const char * Data, const Uint32& DataSize ) {
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

		#ifdef EE_DEBUG
		std::cout << "Couldn't compile shader. Log follows:"  << std::endl;
		std::cout << mCompileLog << std::endl;
		std::cout << mSource << std::endl;
		#endif
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

cVertexShader::cVertexShader( const char * Data, const Uint32& DataSize ) :
	cShader( GL_VERTEX_SHADER, Data, DataSize )
{
}

cVertexShader::cVertexShader( cPack * Pack, const std::string& Filename ) :
	cShader( GL_VERTEX_SHADER, Pack, Filename )
{
}

cVertexShader::cVertexShader( const char ** Data, const Uint32& NumLines ) :
	cShader( GL_VERTEX_SHADER, Data, NumLines )
{
}

cFragmentShader::cFragmentShader() :
	cShader( GL_FRAGMENT_SHADER )
{
}

cFragmentShader::cFragmentShader( const std::string& Filename ) :
	cShader( GL_FRAGMENT_SHADER, Filename )
{
}

cFragmentShader::cFragmentShader( const char * Data, const Uint32& DataSize ) :
	cShader( GL_FRAGMENT_SHADER, Data, DataSize )
{
}

cFragmentShader::cFragmentShader( cPack * Pack, const std::string& Filename ) :
	cShader( GL_FRAGMENT_SHADER, Pack, Filename )
{
}

cFragmentShader::cFragmentShader( const char ** Data, const Uint32& NumLines ) :
	cShader( GL_FRAGMENT_SHADER, Data, NumLines )
{
}

}}
