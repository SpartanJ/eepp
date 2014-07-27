#include <eepp/graphics/shader.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>
#include <eepp/graphics/renderer/renderergl3cp.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>

namespace EE { namespace Graphics {

bool Shader::sEnsure = true;

void Shader::Ensure( bool ensure ) {
	sEnsure = ensure;
}

bool Shader::Ensure() {
	return sEnsure;
}

Shader::Shader( const Uint32& Type ) {
	Init( Type );
}

Shader::Shader( const Uint32& Type, const std::string& Filename ) {
	Init( Type );

	mFilename = FileSystem::FileNameFromPath( Filename );

	if ( FileSystem::FileExists( Filename ) ) {
		SafeDataPointer PData;

		FileSystem::FileGet( Filename, PData );

		SetSource( (const char*)PData.Data, PData.DataSize );
	} else {
		std::string tPath = Filename;
		Pack * tPack = NULL;

		if ( PackManager::instance()->FallbackToPacks() && NULL != ( tPack = PackManager::instance()->Exists( tPath ) ) ) {
			SafeDataPointer PData;

			tPack->ExtractFileToMemory( tPath, PData );

			SetSource( reinterpret_cast<char*> ( PData.Data ), PData.DataSize );
		} else {
			eePRINTL( "Couldn't open shader object: %s", Filename.c_str() );
		}
	}

	Compile();
}

Shader::Shader( const Uint32& Type, const char * Data, const Uint32& DataSize ) {
	Init( Type );

	SetSource( Data, DataSize );

	Compile();
}

Shader::Shader( const Uint32& Type, Pack * Pack, const std::string& Filename ) {
	SafeDataPointer PData;

	Init( Type );

	mFilename = FileSystem::FileNameFromPath( Filename );

	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( Filename ) ) {
		Pack->ExtractFileToMemory( Filename, PData );

		SetSource( reinterpret_cast<char*> ( PData.Data ), PData.DataSize );
	}

	Compile();
}

Shader::Shader( const Uint32& Type, const char ** Data, const Uint32& NumLines ) {
	Init( Type );

	SetSource( Data, NumLines );

	Compile();
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

void Shader::Reload() {
	Init( mType );

	Shader::Ensure( false );
	SetSource( mSource );
	Shader::Ensure( true );

	Compile();
}

std::string Shader::GetName() {
	std::string name;

	if ( mFilename.size() ) {
		name = mFilename;
	} else {
		name = String::ToStr( mGLId );
	}

	return name;
}

void Shader::EnsureVersion() {
	#ifdef EE_GL3_ENABLED
	if ( Shader::Ensure() && ( GLi->Version() == GLv_3 || GLi->Version() == GLv_3CP || GLi->Version() == GLv_ES2 ) ) {
		eePRINTL( "Shader %s converted to programmable pipeline automatically.", GetName().c_str() );

		if ( GL_VERTEX_SHADER == mType ) {
			if ( mSource.find( "ftransform" ) != std::string::npos || mSource.find("dgl_Vertex") == std::string::npos ) {
				if ( GLi->Version() == GLv_3 ) {
					mSource = GLi->GetRendererGL3()->GetBaseVertexShader();
				} else if ( GLi->Version() == GLv_3CP ) {
					mSource = GLi->GetRendererGL3CP()->GetBaseVertexShader();
				} else {
					mSource = GLi->GetRendererGLES2()->GetBaseVertexShader();
				}
			}
		} else {
			if ( mSource.find( "gl_FragColor" ) != std::string::npos ) {
				#ifndef EE_GLES
				if ( GLi->Version() != GLv_3CP )
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

				String::ReplaceAll( mSource, "gl_Color"		, "dgl_Color"		);
				String::ReplaceAll( mSource, "gl_TexCoord"	, "dgl_TexCoord"	);

				if ( GLi->Version() == GLv_3CP ) {
					#ifndef EE_GLES
					String::ReplaceAll( mSource, "gl_FragColor"	, "dgl_FragColor"	);
					#endif
				}
			}
		}
	}

	if ( GLi->Version() == GLv_3CP ) {
		#ifndef EE_GLES
		String::ReplaceAll( mSource, "texture2D"	, "texture"	);
		#endif
	}
	#endif
}

void Shader::SetSource( const std::string& Source ) {
	if ( IsCompiled() ) {
		eePRINTL( "Shader %s report: can't set source for compiled shaders", GetName().c_str() );
		return;
	}

	mSource = Source;

	EnsureVersion();

	#ifdef EE_SHADERS_SUPPORTED
	const char * src = reinterpret_cast<const char *> ( &mSource[0] );

	glShaderSource( mGLId, 1, &src, NULL );
	#endif
}

void Shader::SetSource( const char** Data, const Uint32& NumLines ) {
	std::string tstr;

	for ( Uint32 i = 0; i < NumLines; i++ ) {
		tstr += std::string( Data[i] );
	}

	SetSource( tstr );
}

void Shader::SetSource( const char * Data, const Uint32& DataSize ) {
	std::string _dst( DataSize, 0 );

	memcpy( reinterpret_cast<void*>( &_dst[0] ), reinterpret_cast<const void*>( &Data[0] ), DataSize );

	SetSource( _dst );
}

void Shader::SetSource( const std::vector<Uint8>& Source ) {
	std::string _dst( Source.size(), 0 );

	memcpy( reinterpret_cast<void*>( &_dst[0] ), reinterpret_cast<const void*>( &Source[0] ), Source.size() );

	SetSource( _dst );
}

bool Shader::Compile() {
	if ( IsCompiled() ) {
		eePRINTL( "Shader %s report: can't compile a shader twice", GetName().c_str() );
		return false;
	}

	#ifdef EE_SHADERS_SUPPORTED

	glCompileShader( GetId() );
	mCompiled = true;

	int Compiled;
	glGetShaderiv( GetId(), GL_COMPILE_STATUS, &Compiled );
	mValid = 0 != Compiled;

	if ( !mValid ) {
		int logsize = 0, logarraysize = 0;
		glGetShaderiv( GetId(), GL_INFO_LOG_LENGTH, &logarraysize );

		if ( logarraysize > 0 ) {
			mCompileLog.resize( logarraysize - 1 );

			glGetShaderInfoLog( GetId(), logarraysize, &logsize, reinterpret_cast<GLchar*>( &mCompileLog[0] ) );
		}

		eePRINTL( "Couldn't compile shader %s. Log follows:\n", GetName().c_str() );
		eePRINTL( mCompileLog.c_str() );
		eePRINTL( mSource.c_str() );
	} else {
		eePRINTL( "Shader %s compiled succesfully", GetName().c_str() );
	}

	#endif

	return mValid;
}

bool Shader::IsValid() const {
	return mValid;
}

bool Shader::IsCompiled() const {
	return mCompiled;
}

std::string Shader::CompileLog() const {
	return mCompileLog;
}

Uint32 Shader::GetType() const {
	return mType;
}

Uint32 Shader::GetId() const {
	return mGLId;
}

cVertexShader::cVertexShader() :
	Shader( GL_VERTEX_SHADER )
{
}

cVertexShader::cVertexShader( const std::string& Filename ) :
	Shader( GL_VERTEX_SHADER, Filename )
{
}

cVertexShader::cVertexShader( const char * Data, const Uint32& DataSize ) :
	Shader( GL_VERTEX_SHADER, Data, DataSize )
{
}

cVertexShader::cVertexShader( Pack * Pack, const std::string& Filename ) :
	Shader( GL_VERTEX_SHADER, Pack, Filename )
{
}

cVertexShader::cVertexShader( const char ** Data, const Uint32& NumLines ) :
	Shader( GL_VERTEX_SHADER, Data, NumLines )
{
}

cFragmentShader::cFragmentShader() :
	Shader( GL_FRAGMENT_SHADER )
{
}

cFragmentShader::cFragmentShader( const std::string& Filename ) :
	Shader( GL_FRAGMENT_SHADER, Filename )
{
}

cFragmentShader::cFragmentShader( const char * Data, const Uint32& DataSize ) :
	Shader( GL_FRAGMENT_SHADER, Data, DataSize )
{
}

cFragmentShader::cFragmentShader( Pack * Pack, const std::string& Filename ) :
	Shader( GL_FRAGMENT_SHADER, Pack, Filename )
{
}

cFragmentShader::cFragmentShader( const char ** Data, const Uint32& NumLines ) :
	Shader( GL_FRAGMENT_SHADER, Data, NumLines )
{
}

}}
