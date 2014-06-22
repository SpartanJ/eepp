#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics {

ShaderProgram * ShaderProgram::New( const std::string& name ) {
	return eeNew( ShaderProgram, ( name ) );
}

ShaderProgram * ShaderProgram::New( const std::vector<Shader*>& Shaders, const std::string& name ) {
	return eeNew( ShaderProgram, ( Shaders, name ) );
}

ShaderProgram * ShaderProgram::New( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name ) {
	return eeNew( ShaderProgram, ( VertexShaderFile, FragmentShaderFile, name ) );
}

ShaderProgram * ShaderProgram::New( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& name ) {
	return eeNew( ShaderProgram, ( VertexShaderData, VertexShaderDataSize, FragmentShaderData, FragmentShaderDataSize, name ) );
}

ShaderProgram * ShaderProgram::New( Pack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& name ) {
	return eeNew( ShaderProgram, ( Pack, VertexShaderPath, FragmentShaderPath, name ) );
}

ShaderProgram * ShaderProgram::New( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& name ) {
	return eeNew( ShaderProgram, ( VertexShaderData, NumLinesVS, FragmentShaderData, NumLinesFS, name ) );
}

ShaderProgram::ShaderProgram( const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();
}

ShaderProgram::ShaderProgram( const std::vector<Shader*>& Shaders, const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();

	AddShaders( Shaders );

	Link();
}

ShaderProgram::ShaderProgram( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();

	cVertexShader * vs = eeNew( cVertexShader, ( VertexShaderFile ) );
	cFragmentShader * fs = eeNew( cFragmentShader, ( FragmentShaderFile ) );

	if ( !vs->IsValid() || !fs->IsValid() ) {
		eeSAFE_DELETE( vs );
		eeSAFE_DELETE( fs );
		return;
	}

	AddShader( vs );
	AddShader( fs );

	Link();
}

ShaderProgram::ShaderProgram( Pack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();

	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( VertexShaderPath ) && -1 != Pack->Exists( FragmentShaderPath ) ) {
		cVertexShader * vs = eeNew( cVertexShader, ( Pack, VertexShaderPath ) );
		cFragmentShader * fs = eeNew( cFragmentShader, ( Pack, FragmentShaderPath ) );

		if ( !vs->IsValid() || !fs->IsValid() ) {
			eeSAFE_DELETE( vs );
			eeSAFE_DELETE( fs );
			return;
		}

		AddShader( vs );
		AddShader( fs );

		Link();
	}
}

ShaderProgram::ShaderProgram( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();

	cVertexShader * vs = eeNew( cVertexShader, ( VertexShaderData, VertexShaderDataSize ) );
	cFragmentShader * fs = eeNew( cFragmentShader, ( FragmentShaderData, FragmentShaderDataSize ) );

	if ( !vs->IsValid() || !fs->IsValid() ) {
		eeSAFE_DELETE( vs );
		eeSAFE_DELETE( fs );
		return;
	}

	AddShader( vs );
	AddShader( fs );

	Link();
}

ShaderProgram::ShaderProgram( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();

	cVertexShader * vs = eeNew( cVertexShader, ( VertexShaderData, NumLinesVS ) );
	cFragmentShader * fs = eeNew( cFragmentShader, ( FragmentShaderData, NumLinesFS ) );

	if ( !vs->IsValid() || !fs->IsValid() ) {
		eeSAFE_DELETE( vs );
		eeSAFE_DELETE( fs );
		return;
	}

	AddShader( vs );
	AddShader( fs );

	Link();
}

ShaderProgram::~ShaderProgram() {
	if ( Handler() > 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glDeleteProgram( Handler() );
		#endif
	}

	mUniformLocations.clear();
	mAttributeLocations.clear();

	for ( unsigned int i = 0; i < mShaders.size(); i++ )
		eeSAFE_DELETE( mShaders[i] );

	if ( !ShaderProgramManager::instance()->IsDestroying() ) {
		RemoveFromManager();
	}
}

void ShaderProgram::AddToManager( const std::string& name ) {
	Name( name );

	ShaderProgramManager::instance()->Add( this );
}

void ShaderProgram::RemoveFromManager() {
	ShaderProgramManager::instance()->Remove( this, false );
}

void ShaderProgram::Init() {
	if ( GLi->ShadersSupported() && 0 == Handler() ) {
		#ifdef EE_SHADERS_SUPPORTED
		mHandler = glCreateProgram();
		#endif
		mValid = false;
		mUniformLocations.clear();
		mAttributeLocations.clear();
	} else {
		eePRINTL( "ShaderProgram::Init() %s: Couldn't create program.", mName.c_str() );
	}
}

void ShaderProgram::Reload() {
	mHandler = 0;

	Init();

	std::vector<Shader*> tmpShader = mShaders;

	mShaders.clear();

	for ( unsigned int i = 0; i < tmpShader.size(); i++ ) {
		tmpShader[i]->Reload();
		AddShader( tmpShader[i] );
	}

	Link();

	if ( mReloadCb.IsSet() ) {
		mReloadCb( this );
	}
}

void ShaderProgram::AddShader( Shader* Shader ) {
	if ( !Shader->IsValid() ) {
		eePRINTL( "ShaderProgram::AddShader() %s: Cannot add invalid shader", mName.c_str() );
		return;
	}

	if ( 0 != Handler() ) {
		#ifdef EE_SHADERS_SUPPORTED
		glAttachShader( Handler(), Shader->GetId() );
		#endif

		mShaders.push_back( Shader );
	}
}

void ShaderProgram::AddShaders( const std::vector<Shader*>& Shaders ) {
	for ( Uint32 i = 0; i < Shaders.size(); i++ )
		AddShader( Shaders[i] );
}

bool ShaderProgram::Link() {
	#ifdef EE_SHADERS_SUPPORTED
	glLinkProgram( Handler() );

	Int32 linked;
	glGetProgramiv( Handler(), GL_LINK_STATUS, &linked );
	mValid = 0 != linked;

	int logsize = 0, logarraysize = 0;
	glGetProgramiv( Handler(), GL_INFO_LOG_LENGTH, &logarraysize );

	if ( logarraysize > 0 ) {
		mLinkLog.resize( logarraysize );

		glGetProgramInfoLog( Handler(), logarraysize, &logsize, reinterpret_cast<GLchar*>( &mLinkLog[0] ) );

		mLinkLog.resize( logsize );
	}
	#endif

	if ( !mValid ) {
		eePRINTL( "ShaderProgram::Link(): %s: Couldn't link program. Log follows:\n%s", mName.c_str(), mLinkLog.c_str() );
	} else {
		if ( mLinkLog.size() > 1 ) {
			eePRINTL( "ShaderProgram::Link() %s: Program linked, but received some log:\n%s", mName.c_str(), mLinkLog.c_str() );
		}

		mUniformLocations.clear();
		mAttributeLocations.clear();
	}

	return mValid;
}

void ShaderProgram::Bind() const {
	GlobalBatchRenderer::instance()->Draw();

	GLi->SetShader( const_cast<ShaderProgram*>( this ) );
}

void ShaderProgram::Unbind() const {
	GlobalBatchRenderer::instance()->Draw();

	GLi->SetShader( NULL );
}

Int32 ShaderProgram::UniformLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mUniformLocations.find( Name );
	if ( it == mUniformLocations.end() ) {
		#ifdef EE_SHADERS_SUPPORTED
		Int32 Location = glGetUniformLocation( Handler(), Name.c_str() );
		mUniformLocations[Name] = Location;
		#endif
	}
	return mUniformLocations[Name];
}

Int32 ShaderProgram::AttributeLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mAttributeLocations.find( Name );
	if ( it == mAttributeLocations.end() ) {
		#ifdef EE_SHADERS_SUPPORTED
		Int32 Location = glGetAttribLocation( Handler(), Name.c_str() );
		mAttributeLocations[Name] = Location;
		#endif
	}
	return mAttributeLocations[Name];
}

void ShaderProgram::InvalidateLocations() {
	mUniformLocations.clear();
	mAttributeLocations.clear();
}

bool ShaderProgram::SetUniform( const std::string& Name, float Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1f( Location, Value );
		#endif
	}

	return ( Location >= 0 );
}

bool ShaderProgram::SetUniform( const std::string& Name, Vector2ff Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform2fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif
	}

	return ( Location >= 0 );
}

bool ShaderProgram::SetUniform( const std::string& Name, Vector3ff Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform3fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif
	}

	return ( Location >= 0 );
}

bool ShaderProgram::SetUniform( const std::string& Name, float x, float y, float z, float w ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform4f( Location, x, y, z, w );
		#endif
	}

	return ( Location >= 0 );
}

bool ShaderProgram::SetUniform( const std::string& Name, Int32 Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1i( Location, Value );
		#endif
	}

	return ( Location >= 0 );
}

bool ShaderProgram::SetUniform( const Int32& Location, Int32 Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1i( Location, Value );
		#endif

		return true;
	}

	return false;
}

bool ShaderProgram::SetUniform( const Int32& Location, float Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1f( Location, Value );
		#endif

		return true;
	}

	return false;
}

bool ShaderProgram::SetUniform( const Int32& Location, Vector2ff Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform2fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif

		return true;
	}

	return false;
}

bool ShaderProgram::SetUniform( const Int32& Location, Vector3ff Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform3fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif

		return true;
	}

	return false;
}

bool ShaderProgram::SetUniform( const Int32& Location, float x, float y, float z, float w ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform4f( Location, x, y, z, w );
		#endif

		return true;
	}

	return false;
}

bool ShaderProgram::SetUniformMatrix( const Int32& Location, const float * Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniformMatrix4fv( Location, 1, false, Value );
		#endif

		return true;
	}

	return false;
}

bool ShaderProgram::SetUniformMatrix( const std::string Name, const float * Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniformMatrix4fv( Location, 1, false, Value );
		#endif
	}

	return ( Location >= 0 );
}

const std::string& ShaderProgram::Name() const {
	return mName;
}

void ShaderProgram::Name( const std::string& name ) {
	mName = name;
	mId = String::Hash( mName );

	Uint32 NameCount = ShaderProgramManager::instance()->Exists( mName );

	if ( 0 != NameCount || 0 == name.size() ) {
		Name( name + String::ToStr( NameCount + 1 ) );
	}
}

void ShaderProgram::SetReloadCb( ShaderProgramReloadCb Cb ) {
	mReloadCb = Cb;
}

void ShaderProgram::EnableVertexAttribArray( const std::string& Name ) {
	EnableVertexAttribArray( AttributeLocation( Name ) );
}

void ShaderProgram::EnableVertexAttribArray( const Int32& Location ) {
	#ifdef EE_SHADERS_SUPPORTED
	glEnableVertexAttribArray( Location );
	#endif
}

void ShaderProgram::DisableVertexAttribArray( const std::string& Name ) {
	DisableVertexAttribArray( AttributeLocation( Name ) );
}

void ShaderProgram::DisableVertexAttribArray( const Int32& Location ) {
	#ifdef EE_SHADERS_SUPPORTED
	glDisableVertexAttribArray( Location );
	#endif
}

}}
