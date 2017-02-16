#include <eepp/graphics/shader.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>
#include <eepp/graphics/renderer/renderergl3cp.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>

namespace EE { namespace Graphics {

bool Shader::sEnsure = true;

void Shader::ensure( bool ensure ) {
	sEnsure = ensure;
}

bool Shader::ensure() {
	return sEnsure;
}

Shader::Shader( const Uint32& Type ) {
	Init( Type );
}

Shader::Shader( const Uint32& Type, const std::string& Filename ) {
	Init( Type );

	mFilename = FileSystem::fileNameFromPath( Filename );

	if ( FileSystem::fileExists( Filename ) ) {
		SafeDataPointer PData;

		FileSystem::fileGet( Filename, PData );

		setSource( (const char*)PData.Data, PData.DataSize );
	} else {
		std::string tPath = Filename;
		Pack * tPack = NULL;

		if ( PackManager::instance()->fallbackToPacks() && NULL != ( tPack = PackManager::instance()->exists( tPath ) ) ) {
			SafeDataPointer PData;

			tPack->extractFileToMemory( tPath, PData );

			setSource( reinterpret_cast<char*> ( PData.Data ), PData.DataSize );
		} else {
			eePRINTL( "Couldn't open shader object: %s", Filename.c_str() );
		}
	}

	compile();
}

Shader::Shader( const Uint32& Type, const char * Data, const Uint32& DataSize ) {
	Init( Type );

	setSource( Data, DataSize );

	compile();
}

Shader::Shader( const Uint32& Type, Pack * Pack, const std::string& Filename ) {
	SafeDataPointer PData;

	Init( Type );

	mFilename = FileSystem::fileNameFromPath( Filename );

	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( Filename ) ) {
		Pack->extractFileToMemory( Filename, PData );

		setSource( reinterpret_cast<char*> ( PData.Data ), PData.DataSize );
	}

	compile();
}

Shader::Shader( const Uint32& Type, const char ** Data, const Uint32& NumLines ) {
	Init( Type );

	setSource( Data, NumLines );

	compile();
}

Shader::~Shader() {
	if ( 0 != mGLId ) {
		#ifdef EE_SHADERS_SUPPORTED
		glDeleteShader( mGLId );
		#endif
	}
}

void Shader::Init( const Uint32& Type ) {
	mType 		= Type;
	mValid 		= false;
	mCompiled 	= false;
	#ifdef EE_SHADERS_SUPPORTED
	mGLId 		= glCreateShader( mType );
	#endif
}

void Shader::reload() {
	Init( mType );

	Shader::ensure( false );
	setSource( mSource );
	Shader::ensure( true );

	compile();
}

std::string Shader::getName() {
	std::string name;

	if ( mFilename.size() ) {
		name = mFilename;
	} else {
		name = String::toStr( mGLId );
	}

	return name;
}

void Shader::ensureVersion() {
	#ifdef EE_GL3_ENABLED
	if ( Shader::ensure() && ( GLi->version() == GLv_3 || GLi->version() == GLv_3CP || GLi->version() == GLv_ES2 ) ) {
		eePRINTL( "Shader %s converted to programmable pipeline automatically.", getName().c_str() );

		if ( GL_VERTEX_SHADER == mType ) {
			if ( mSource.find( "ftransform" ) != std::string::npos || mSource.find("dgl_Vertex") == std::string::npos ) {
				if ( GLi->version() == GLv_3 ) {
					mSource = GLi->getRendererGL3()->getBaseVertexShader();
				} else if ( GLi->version() == GLv_3CP ) {
					mSource = GLi->getRendererGL3CP()->getBaseVertexShader();
				} else {
					mSource = GLi->getRendererGLES2()->getBaseVertexShader();
				}
			}
		} else {
			if ( mSource.find( "gl_FragColor" ) != std::string::npos ) {
				#ifndef EE_GLES
				if ( GLi->version() != GLv_3CP )
				#else
				if ( true )
				#endif
				{
					#ifdef EE_GLES
					std::string preSource = "#ifdef GL_ES\nprecision mediump float;\nprecision lowp int;\n#endif";
					#else
					std::string preSource = "#version 120";
					#endif

					mSource = preSource + "\nvarying	vec4		gl_Color;\nvarying	vec4		gl_TexCoord[ 1 ];\n" + mSource;
				} else {
					mSource = "#version 330\nin	vec4		gl_Color;\nin	vec4		gl_TexCoord[ 1 ];\nout		vec4		gl_FragColor;\n" + mSource;
				}

				String::replaceAll( mSource, "gl_Color"		, "dgl_Color"		);
				String::replaceAll( mSource, "gl_TexCoord"	, "dgl_TexCoord"	);

				if ( GLi->version() == GLv_3CP ) {
					#ifndef EE_GLES
					String::replaceAll( mSource, "gl_FragColor"	, "dgl_FragColor"	);
					#endif
				}
			}
		}
	}

	if ( GLi->version() == GLv_3CP ) {
		#ifndef EE_GLES
		String::replaceAll( mSource, "texture2D"	, "texture"	);
		#endif
	}
	#endif
}

void Shader::setSource( const std::string& Source ) {
	if ( isCompiled() ) {
		eePRINTL( "Shader %s report: can't set source for compiled shaders", getName().c_str() );
		return;
	}

	mSource = Source;

	ensureVersion();

	#ifdef EE_SHADERS_SUPPORTED
	const char * src = reinterpret_cast<const char *> ( &mSource[0] );

	glShaderSource( mGLId, 1, &src, NULL );
	#endif
}

void Shader::setSource( const char** Data, const Uint32& NumLines ) {
	std::string tstr;

	for ( Uint32 i = 0; i < NumLines; i++ ) {
		tstr += std::string( Data[i] );
	}

	setSource( tstr );
}

void Shader::setSource( const char * Data, const Uint32& DataSize ) {
	std::string _dst( DataSize, 0 );

	memcpy( reinterpret_cast<void*>( &_dst[0] ), reinterpret_cast<const void*>( &Data[0] ), DataSize );

	setSource( _dst );
}

void Shader::setSource( const std::vector<Uint8>& Source ) {
	std::string _dst( Source.size(), 0 );

	memcpy( reinterpret_cast<void*>( &_dst[0] ), reinterpret_cast<const void*>( &Source[0] ), Source.size() );

	setSource( _dst );
}

bool Shader::compile() {
	if ( isCompiled() ) {
		eePRINTL( "Shader %s report: can't compile a shader twice", getName().c_str() );
		return false;
	}

	#ifdef EE_SHADERS_SUPPORTED

	glCompileShader( getId() );
	mCompiled = true;

	int Compiled;
	glGetShaderiv( getId(), GL_COMPILE_STATUS, &Compiled );
	mValid = 0 != Compiled;

	if ( !mValid ) {
		int logsize = 0, logarraysize = 0;
		glGetShaderiv( getId(), GL_INFO_LOG_LENGTH, &logarraysize );

		if ( logarraysize > 0 ) {
			mCompileLog.resize( logarraysize - 1 );

			glGetShaderInfoLog( getId(), logarraysize, &logsize, reinterpret_cast<GLchar*>( &mCompileLog[0] ) );
		}

		eePRINTL( "Couldn't compile shader %s. Log follows:\n", getName().c_str() );
		eePRINTL( mCompileLog.c_str() );
		eePRINTL( mSource.c_str() );
	} else {
		eePRINTL( "Shader %s compiled succesfully", getName().c_str() );
	}

	#endif

	return mValid;
}

bool Shader::isValid() const {
	return mValid;
}

bool Shader::isCompiled() const {
	return mCompiled;
}

std::string Shader::compileLog() const {
	return mCompileLog;
}

Uint32 Shader::getType() const {
	return mType;
}

Uint32 Shader::getId() const {
	return mGLId;
}

VertexShader::VertexShader() :
	Shader( GL_VERTEX_SHADER )
{
}

VertexShader::VertexShader( const std::string& Filename ) :
	Shader( GL_VERTEX_SHADER, Filename )
{
}

VertexShader::VertexShader( const char * Data, const Uint32& DataSize ) :
	Shader( GL_VERTEX_SHADER, Data, DataSize )
{
}

VertexShader::VertexShader( Pack * Pack, const std::string& Filename ) :
	Shader( GL_VERTEX_SHADER, Pack, Filename )
{
}

VertexShader::VertexShader( const char ** Data, const Uint32& NumLines ) :
	Shader( GL_VERTEX_SHADER, Data, NumLines )
{
}

FragmentShader::FragmentShader() :
	Shader( GL_FRAGMENT_SHADER )
{
}

FragmentShader::FragmentShader( const std::string& Filename ) :
	Shader( GL_FRAGMENT_SHADER, Filename )
{
}

FragmentShader::FragmentShader( const char * Data, const Uint32& DataSize ) :
	Shader( GL_FRAGMENT_SHADER, Data, DataSize )
{
}

FragmentShader::FragmentShader( Pack * Pack, const std::string& Filename ) :
	Shader( GL_FRAGMENT_SHADER, Pack, Filename )
{
}

FragmentShader::FragmentShader( const char ** Data, const Uint32& NumLines ) :
	Shader( GL_FRAGMENT_SHADER, Data, NumLines )
{
}

}}
