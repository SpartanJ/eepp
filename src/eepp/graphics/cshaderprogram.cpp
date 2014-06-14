#include <eepp/graphics/cshaderprogram.hpp>
#include <eepp/graphics/cshaderprogrammanager.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics {

cShaderProgram * cShaderProgram::New( const std::string& name ) {
	return eeNew( cShaderProgram, ( name ) );
}

cShaderProgram * cShaderProgram::New( const std::vector<cShader*>& Shaders, const std::string& name ) {
	return eeNew( cShaderProgram, ( Shaders, name ) );
}

cShaderProgram * cShaderProgram::New( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name ) {
	return eeNew( cShaderProgram, ( VertexShaderFile, FragmentShaderFile, name ) );
}

cShaderProgram * cShaderProgram::New( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& name ) {
	return eeNew( cShaderProgram, ( VertexShaderData, VertexShaderDataSize, FragmentShaderData, FragmentShaderDataSize, name ) );
}

cShaderProgram * cShaderProgram::New( cPack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& name ) {
	return eeNew( cShaderProgram, ( Pack, VertexShaderPath, FragmentShaderPath, name ) );
}

cShaderProgram * cShaderProgram::New( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& name ) {
	return eeNew( cShaderProgram, ( VertexShaderData, NumLinesVS, FragmentShaderData, NumLinesFS, name ) );
}

cShaderProgram::cShaderProgram( const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();
}

cShaderProgram::cShaderProgram( const std::vector<cShader*>& Shaders, const std::string& name ) :
	mHandler(0),
	mId(0)
{
	AddToManager( name );
	Init();

	AddShaders( Shaders );

	Link();
}

cShaderProgram::cShaderProgram( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name ) :
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

cShaderProgram::cShaderProgram( cPack * Pack, const std::string& VertexShaderPath, const std::string& FragmentShaderPath, const std::string& name ) :
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

cShaderProgram::cShaderProgram( const char * VertexShaderData, const Uint32& VertexShaderDataSize, const char * FragmentShaderData, const Uint32& FragmentShaderDataSize, const std::string& name ) :
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

cShaderProgram::cShaderProgram( const char ** VertexShaderData, const Uint32& NumLinesVS, const char ** FragmentShaderData, const Uint32& NumLinesFS, const std::string& name ) :
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

cShaderProgram::~cShaderProgram() {
	if ( Handler() > 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glDeleteProgram( Handler() );
		#endif
	}

	mUniformLocations.clear();
	mAttributeLocations.clear();

	for ( unsigned int i = 0; i < mShaders.size(); i++ )
		eeSAFE_DELETE( mShaders[i] );

	if ( !cShaderProgramManager::instance()->IsDestroying() ) {
		RemoveFromManager();
	}
}

void cShaderProgram::AddToManager( const std::string& name ) {
	Name( name );

	cShaderProgramManager::instance()->Add( this );
}

void cShaderProgram::RemoveFromManager() {
	cShaderProgramManager::instance()->Remove( this, false );
}

void cShaderProgram::Init() {
	if ( GLi->ShadersSupported() && 0 == Handler() ) {
		#ifdef EE_SHADERS_SUPPORTED
		mHandler = glCreateProgram();
		#endif
		mValid = false;
		mUniformLocations.clear();
		mAttributeLocations.clear();
	} else {
		eePRINTL( "cShaderProgram::Init() %s: Couldn't create program.", mName.c_str() );
	}
}

void cShaderProgram::Reload() {
	mHandler = 0;

	Init();

	std::vector<cShader*> tmpShader = mShaders;

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

void cShaderProgram::AddShader( cShader* Shader ) {
	if ( !Shader->IsValid() ) {
		eePRINTL( "cShaderProgram::AddShader() %s: Cannot add invalid shader", mName.c_str() );
		return;
	}

	if ( 0 != Handler() ) {
		#ifdef EE_SHADERS_SUPPORTED
		glAttachShader( Handler(), Shader->GetId() );
		#endif

		mShaders.push_back( Shader );
	}
}

void cShaderProgram::AddShaders( const std::vector<cShader*>& Shaders ) {
	for ( Uint32 i = 0; i < Shaders.size(); i++ )
		AddShader( Shaders[i] );
}

bool cShaderProgram::Link() {
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
		eePRINTL( "cShaderProgram::Link(): %s: Couldn't link program. Log follows:\n%s", mName.c_str(), mLinkLog.c_str() );
	} else {
		if ( mLinkLog.size() > 1 ) {
			eePRINTL( "cShaderProgram::Link() %s: Program linked, but received some log:\n%s", mName.c_str(), mLinkLog.c_str() );
		}

		mUniformLocations.clear();
		mAttributeLocations.clear();
	}

	return mValid;
}

void cShaderProgram::Bind() const {
	cGlobalBatchRenderer::instance()->Draw();

	GLi->SetShader( const_cast<cShaderProgram*>( this ) );
}

void cShaderProgram::Unbind() const {
	cGlobalBatchRenderer::instance()->Draw();

	GLi->SetShader( NULL );
}

Int32 cShaderProgram::UniformLocation( const std::string& Name ) {
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

Int32 cShaderProgram::AttributeLocation( const std::string& Name ) {
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

void cShaderProgram::InvalidateLocations() {
	mUniformLocations.clear();
	mAttributeLocations.clear();
}

bool cShaderProgram::SetUniform( const std::string& Name, float Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1f( Location, Value );
		#endif
	}

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, eeVector2ff Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform2fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif
	}

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, eeVector3ff Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform3fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif
	}

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, float x, float y, float z, float w ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform4f( Location, x, y, z, w );
		#endif
	}

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, Int32 Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1i( Location, Value );
		#endif
	}

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const Int32& Location, Int32 Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1i( Location, Value );
		#endif

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, float Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform1f( Location, Value );
		#endif

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, eeVector2ff Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform2fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, eeVector3ff Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform3fv( Location, 1, reinterpret_cast<float*>( &Value ) );
		#endif

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, float x, float y, float z, float w ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniform4f( Location, x, y, z, w );
		#endif

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniformMatrix( const Int32& Location, const float * Value ) {
	if ( -1 != Location ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniformMatrix4fv( Location, 1, false, Value );
		#endif

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniformMatrix( const std::string Name, const float * Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 ) {
		#ifdef EE_SHADERS_SUPPORTED
		glUniformMatrix4fv( Location, 1, false, Value );
		#endif
	}

	return ( Location >= 0 );
}

const std::string& cShaderProgram::Name() const {
	return mName;
}

void cShaderProgram::Name( const std::string& name ) {
	mName = name;
	mId = String::Hash( mName );

	Uint32 NameCount = cShaderProgramManager::instance()->Exists( mName );

	if ( 0 != NameCount || 0 == name.size() ) {
		Name( name + String::ToStr( NameCount + 1 ) );
	}
}

void cShaderProgram::SetReloadCb( ShaderProgramReloadCb Cb ) {
	mReloadCb = Cb;
}

void cShaderProgram::EnableVertexAttribArray( const std::string& Name ) {
	EnableVertexAttribArray( AttributeLocation( Name ) );
}

void cShaderProgram::EnableVertexAttribArray( const Int32& Location ) {
	#ifdef EE_SHADERS_SUPPORTED
	glEnableVertexAttribArray( Location );
	#endif
}

void cShaderProgram::DisableVertexAttribArray( const std::string& Name ) {
	DisableVertexAttribArray( AttributeLocation( Name ) );
}

void cShaderProgram::DisableVertexAttribArray( const Int32& Location ) {
	#ifdef EE_SHADERS_SUPPORTED
	glDisableVertexAttribArray( Location );
	#endif
}

}}
