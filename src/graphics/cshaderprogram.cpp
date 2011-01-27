#include "cshaderprogram.hpp"
#include "cshaderprogrammanager.hpp"
#include "cglobalbatchrenderer.hpp"
#include "glhelper.hpp"

namespace EE { namespace Graphics {

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
	if ( Handler() > 0 )
    	glDeleteProgram( Handler() );

    mUniformLocations.clear();
    mAttributeLocations.clear();

	for ( eeUint i = 0; i < mShaders.size(); i++ )
		eeSAFE_DELETE( mShaders[i] );
}

void cShaderProgram::AddToManager( const std::string& name ) {
	Name( name );

	cShaderProgramManager::instance()->Add( this );
}

void cShaderProgram::RemoveFromManager() {
	cShaderProgramManager::instance()->Remove( this );
}

void cShaderProgram::Init() {
	if ( GLi->ShadersSupported() && 0 == Handler() ) {
		mHandler = glCreateProgram();
		mValid = false;
		mUniformLocations.clear();
		mAttributeLocations.clear();
	}
}

void cShaderProgram::Reload() {
	mHandler = 0;

	Init();

	std::vector<cShader*> tmpShader = mShaders;

	mShaders.clear();

	for ( eeUint i = 0; i < tmpShader.size(); i++ ) {
	    tmpShader[i]->Reload();
		AddShader( tmpShader[i] );
    }

	Link();
}

void cShaderProgram::AddShader( cShader* Shader ) {
	if ( !Shader->IsValid() ) {
		cLog::instance()->Write( "cShaderProgram::AddShader(): Cannot add invalid shader" );
		return;
	}

	if ( 0 != Handler() ) {
		glAttachShader( Handler(), Shader->GetId() );

		mShaders.push_back( Shader );
	}
}

void cShaderProgram::AddShaders( const std::vector<cShader*>& Shaders ) {
	for ( Uint32 i = 0; i < Shaders.size(); i++ )
		AddShader( Shaders[i] );
}

bool cShaderProgram::Link() {
	glLinkProgram( Handler() );

	Int32 linked;
	glGetProgramiv( Handler(), GL_LINK_STATUS, &linked );
	mValid = 0 != linked;

	GLsizei logsize, logarraysize;
	glGetProgramiv( Handler(), GL_INFO_LOG_LENGTH, &logarraysize );
	mLinkLog.resize(logarraysize);

	glGetProgramInfoLog( Handler(), logarraysize, &logsize, reinterpret_cast<GLchar*>( &mLinkLog[0] ) );

	if ( !mValid ) {
		cLog::instance()->Write( "cShaderProgram::Link(): Couldn't link program. Log follows:" + mLinkLog );
	} else {
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
		Int32 Location = glGetUniformLocation( Handler(), Name.c_str() );
		mUniformLocations[Name] = Location;
	}
	return mUniformLocations[Name];
}

Int32 cShaderProgram::AttributeLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mAttributeLocations.find( Name );
	if ( it == mAttributeLocations.end() ) {
		Int32 Location = glGetAttribLocation( Handler(), Name.c_str() );
		mAttributeLocations[Name] = Location;
	}
	return mAttributeLocations[Name];
}

void cShaderProgram::InvalidateLocations() {
	mUniformLocations.clear();
	mAttributeLocations.clear();
}

bool cShaderProgram::SetUniform( const std::string& Name, eeFloat Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 )
		glUniform1f( Location, Value );

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, eeVector2f Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 )
		glUniform2fv( Location, 1, reinterpret_cast<eeFloat*>( &Value ) );

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, eeVector3f Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 )
		glUniform3fv( Location, 1, reinterpret_cast<eeFloat*>( &Value ) );

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const std::string& Name, Int32 Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 )
		glUniform1i( Location, Value );

	return ( Location >= 0 );
}

bool cShaderProgram::SetUniform( const Int32& Location, Int32 Value ) {
	if ( -1 != Location ) {
		glUniform1i( Location, Value );

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, eeFloat Value ) {
	if ( -1 != Location ) {
		glUniform1f( Location, Value );
		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, eeVector2f Value ) {
	if ( -1 != Location ) {
		glUniform2fv( Location, 1, reinterpret_cast<eeFloat*>( &Value ) );
		return true;
	}

	return false;
}

bool cShaderProgram::SetUniform( const Int32& Location, eeVector3f Value ) {
	if ( -1 != Location ) {
		glUniform3fv( Location, 1, reinterpret_cast<eeFloat*>( &Value ) );
		return true;
	}

	return false;
}

bool cShaderProgram::SetUniformMatrix( const Int32& Location, const float * Value ) {
	if ( -1 != Location ) {
		glUniformMatrix4fv( Location, 1, false, Value );

		return true;
	}

	return false;
}

bool cShaderProgram::SetUniformMatrix( const std::string Name, const float * Value ) {
	Int32 Location = UniformLocation( Name );

	if ( Location >= 0 )
		glUniformMatrix4fv( Location, 1, false, Value );

	return ( Location >= 0 );
}

const std::string& cShaderProgram::Name() const {
	return mName;
}

void cShaderProgram::Name( const std::string& name ) {
	mName = name;
	mId = MakeHash( mName );

	Uint32 NameCount = cShaderProgramManager::instance()->Exists( mName );

	if ( 0 != NameCount || 0 == name.size() ) {
		Name( name + intToStr( NameCount + 1 ) );
	}
}

}}
