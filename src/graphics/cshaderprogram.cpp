#include "cshaderprogram.hpp"
#include "../window/cengine.hpp"
#include "cshaderprogrammanager.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

cShaderProgram::cShaderProgram( const std::string& name ) :
	mGLId(0)
{
	AddToManager( name );
	Init();
}

cShaderProgram::cShaderProgram( const std::vector<cShader*>& Shaders, const std::string& name ) :
	mGLId(0)
{
	AddToManager( name );
	Init();

	AddShaders( Shaders );

	Link();
}

cShaderProgram::cShaderProgram( const std::string& VertexShaderFile, const std::string& FragmentShaderFile, const std::string& name ) :
	mGLId(0)
{
	AddToManager( name );
	Init();

	cVertexShader * vs = new cVertexShader( VertexShaderFile );
	cFragmentShader * fs = new cFragmentShader( FragmentShaderFile );

	if ( !vs->IsValid() || !fs->IsValid() ) {
		delete vs;
		delete fs;
		return;
	}

	AddShader( vs );
	AddShader( fs );

	Link();
}

cShaderProgram::~cShaderProgram() {
	if ( GetId() > 0 )
    	glDeleteProgram( GetId() );

    mUniformLocations.clear();
    mAttributeLocations.clear();

	for ( eeUint i = 0; i < mShaders.size(); i++ )
		delete mShaders[i];
}

void cShaderProgram::AddToManager( const std::string& name ) {
	Name( name );

	cShaderProgramManager::instance()->Add( this );
}

void cShaderProgram::RemoveFromManager() {
	cShaderProgramManager::instance()->Remove( this );
}

void cShaderProgram::Init() {
	if ( cEngine::instance()->ShadersSupported() && 0 == GetId() ) {
		mGLId = glCreateProgram();
		mValid = false;
		mUniformLocations.clear();
		mAttributeLocations.clear();
	}
}

void cShaderProgram::Reload() {
	mGLId = 0;

	Init();

	std::vector<cShader*> tmpShader = mShaders;

	mShaders.clear();

	for ( eeUint i = 0; i < tmpShader.size(); i++ )
		AddShader( tmpShader[i] );

	Link();
}

void cShaderProgram::AddShader( cShader* Shader ) {
	if ( !Shader->IsValid() ) {
		cLog::instance()->Write( "cShaderProgram::AddShader(): Cannot add invalid shader" );
		return;
	}

	if ( 0 != GetId() ) {
		glAttachShader( GetId(), Shader->GetId() );

		mShaders.push_back( Shader );
	}
}

void cShaderProgram::AddShaders( const std::vector<cShader*>& Shaders ) {
	for ( Uint32 i = 0; i < Shaders.size(); i++ )
		AddShader( Shaders[i] );
}

bool cShaderProgram::Link() {
	glLinkProgram( GetId() );

	Int32 linked;
	glGetProgramiv( GetId(), GL_LINK_STATUS, &linked );
	mValid = 0 != linked;

	GLsizei logsize, logarraysize;
	glGetProgramiv( GetId(), GL_INFO_LOG_LENGTH, &logarraysize );
	mLinkLog.resize(logarraysize);

	glGetProgramInfoLog( GetId(), logarraysize, &logsize, reinterpret_cast<GLchar*>( &mLinkLog[0] ) );

	if ( !mValid ) {
		cLog::instance()->Write( "cShaderProgram::Link(): Couldn't link program. Log follows:" + mLinkLog );
	} else {
		mUniformLocations.clear();
		mAttributeLocations.clear();
	}

	return mValid;
}

void cShaderProgram::Bind() const {
	glUseProgram( GetId() );
}

void cShaderProgram::Unbind() const {
	glUseProgram( 0 );
}

Int32 cShaderProgram::UniformLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mUniformLocations.find( Name );
	if ( it == mUniformLocations.end() ) {
		Int32 Location = glGetUniformLocation( GetId(), Name.c_str() );
		mUniformLocations[Name] = Location;
	}
	return mUniformLocations[Name];
}

Int32 cShaderProgram::AttributeLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mAttributeLocations.find( Name );
	if ( it == mAttributeLocations.end() ) {
		Int32 Location = glGetAttribLocation( GetId(), Name.c_str() );
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

const std::string& cShaderProgram::Name() const {
	return mName;
}

void cShaderProgram::Name( const std::string& name ) {
	mName = name;

	Uint32 NameCount = cShaderProgramManager::instance()->ExistsName( mName );

	if ( 0 != NameCount || 0 == name.size() ) {
		Name( name + intToStr( NameCount + 1 ) );
	}
}

}}
