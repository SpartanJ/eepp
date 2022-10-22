#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/system/log.hpp>

namespace EE { namespace Graphics {

ShaderProgram* ShaderProgram::New( const std::string& name ) {
	return eeNew( ShaderProgram, ( name ) );
}

ShaderProgram* ShaderProgram::New( const std::vector<Shader*>& Shaders, const std::string& name ) {
	return eeNew( ShaderProgram, ( Shaders, name ) );
}

ShaderProgram* ShaderProgram::New( const std::string& VertexShaderFile,
								   const std::string& FragmentShaderFile,
								   const std::string& name ) {
	return eeNew( ShaderProgram, ( VertexShaderFile, FragmentShaderFile, name ) );
}

ShaderProgram* ShaderProgram::New( const char* VertexShaderData, const Uint32& VertexShaderDataSize,
								   const char* FragmentShaderData,
								   const Uint32& FragmentShaderDataSize, const std::string& name ) {
	return eeNew( ShaderProgram, ( VertexShaderData, VertexShaderDataSize, FragmentShaderData,
								   FragmentShaderDataSize, name ) );
}

ShaderProgram* ShaderProgram::New( Pack* Pack, const std::string& VertexShaderPath,
								   const std::string& FragmentShaderPath,
								   const std::string& name ) {
	return eeNew( ShaderProgram, ( Pack, VertexShaderPath, FragmentShaderPath, name ) );
}

ShaderProgram* ShaderProgram::New( const char** VertexShaderData, const Uint32& NumLinesVS,
								   const char** FragmentShaderData, const Uint32& NumLinesFS,
								   const std::string& name ) {
	return eeNew( ShaderProgram,
				  ( VertexShaderData, NumLinesVS, FragmentShaderData, NumLinesFS, name ) );
}

ShaderProgram::ShaderProgram( const std::string& name ) : mHandler( 0 ), mId( 0 ) {
	addToManager( name );
	init();
}

ShaderProgram::ShaderProgram( const std::vector<Shader*>& Shaders, const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToManager( name );
	init();

	addShaders( Shaders );

	link();
}

ShaderProgram::ShaderProgram( const std::string& VertexShaderFile,
							  const std::string& FragmentShaderFile, const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToManager( name );
	init();

	VertexShader* vs = eeNew( VertexShader, ( VertexShaderFile ) );
	FragmentShader* fs = eeNew( FragmentShader, ( FragmentShaderFile ) );

	if ( !vs->isValid() || !fs->isValid() ) {
		eeSAFE_DELETE( vs );
		eeSAFE_DELETE( fs );
		return;
	}

	addShader( vs );
	addShader( fs );

	link();
}

ShaderProgram::ShaderProgram( Pack* Pack, const std::string& VertexShaderPath,
							  const std::string& FragmentShaderPath, const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToManager( name );
	init();

	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( VertexShaderPath ) &&
		 -1 != Pack->exists( FragmentShaderPath ) ) {
		VertexShader* vs = eeNew( VertexShader, ( Pack, VertexShaderPath ) );
		FragmentShader* fs = eeNew( FragmentShader, ( Pack, FragmentShaderPath ) );

		if ( !vs->isValid() || !fs->isValid() ) {
			eeSAFE_DELETE( vs );
			eeSAFE_DELETE( fs );
			return;
		}

		addShader( vs );
		addShader( fs );

		link();
	}
}

ShaderProgram::ShaderProgram( const char* VertexShaderData, const Uint32& VertexShaderDataSize,
							  const char* FragmentShaderData, const Uint32& FragmentShaderDataSize,
							  const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToManager( name );
	init();

	VertexShader* vs = eeNew( VertexShader, ( VertexShaderData, VertexShaderDataSize ) );
	FragmentShader* fs = eeNew( FragmentShader, ( FragmentShaderData, FragmentShaderDataSize ) );

	if ( !vs->isValid() || !fs->isValid() ) {
		eeSAFE_DELETE( vs );
		eeSAFE_DELETE( fs );
		return;
	}

	addShader( vs );
	addShader( fs );

	link();
}

ShaderProgram::ShaderProgram( const char** VertexShaderData, const Uint32& NumLinesVS,
							  const char** FragmentShaderData, const Uint32& NumLinesFS,
							  const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToManager( name );
	init();

	VertexShader* vs = eeNew( VertexShader, ( VertexShaderData, NumLinesVS ) );
	FragmentShader* fs = eeNew( FragmentShader, ( FragmentShaderData, NumLinesFS ) );

	if ( !vs->isValid() || !fs->isValid() ) {
		eeSAFE_DELETE( vs );
		eeSAFE_DELETE( fs );
		return;
	}

	addShader( vs );
	addShader( fs );

	link();
}

ShaderProgram::~ShaderProgram() {
	if ( getHandler() > 0 ) {
#ifdef EE_SHADERS_SUPPORTED
		glDeleteProgram( getHandler() );
#endif
	}

	mUniformLocations.clear();
	mAttributeLocations.clear();

	for ( unsigned int i = 0; i < mShaders.size(); i++ )
		eeSAFE_DELETE( mShaders[i] );

	if ( !ShaderProgramManager::instance()->isDestroying() ) {
		removeFromManager();
	}
}

void ShaderProgram::addToManager( const std::string& name ) {
	setName( name );

	ShaderProgramManager::instance()->add( this );
}

void ShaderProgram::removeFromManager() {
	ShaderProgramManager::instance()->remove( this, false );
}

void ShaderProgram::init() {
	if ( GLi->shadersSupported() && 0 == getHandler() ) {
#ifdef EE_SHADERS_SUPPORTED
		mHandler = glCreateProgram();
#endif
		mValid = false;
		mUniformLocations.clear();
		mAttributeLocations.clear();
	} else {
		Log::error( "ShaderProgram::init() %s: Couldn't create program.", mName.c_str() );
	}
}

void ShaderProgram::reload() {
	mHandler = 0;

	init();

	std::vector<Shader*> tmpShader = mShaders;

	mShaders.clear();

	for ( unsigned int i = 0; i < tmpShader.size(); i++ ) {
		tmpShader[i]->reload();
		addShader( tmpShader[i] );
	}

	link();

	if ( mReloadCb ) {
		mReloadCb( this );
	}
}

void ShaderProgram::addShader( Shader* Shader ) {
	if ( !Shader->isValid() ) {
		Log::error( "ShaderProgram::addShader() %s: Cannot add invalid shader", mName.c_str() );
		return;
	}

	if ( 0 != getHandler() ) {
#ifdef EE_SHADERS_SUPPORTED
		glAttachShader( getHandler(), Shader->getId() );
#endif

		mShaders.push_back( Shader );
	}
}

void ShaderProgram::addShaders( const std::vector<Shader*>& Shaders ) {
	for ( Uint32 i = 0; i < Shaders.size(); i++ )
		addShader( Shaders[i] );
}

bool ShaderProgram::link() {
#ifdef EE_SHADERS_SUPPORTED
	glLinkProgram( getHandler() );

	Int32 linked;
	glGetProgramiv( getHandler(), GL_LINK_STATUS, &linked );
	mValid = 0 != linked;

	int logsize = 0, logarraysize = 0;
	glGetProgramiv( getHandler(), GL_INFO_LOG_LENGTH, &logarraysize );

	if ( logarraysize > 0 ) {
		mLinkLog.resize( logarraysize );

		glGetProgramInfoLog( getHandler(), logarraysize, &logsize,
							 reinterpret_cast<GLchar*>( &mLinkLog[0] ) );

		mLinkLog.resize( logsize );
	}
#endif

	if ( !mValid ) {
		Log::error( "ShaderProgram::Link(): %s: Couldn't link program. Log follows:\n%s",
					mName.c_str(), mLinkLog.c_str() );
	} else {
		if ( mLinkLog.size() > 1 ) {
			Log::warning( "ShaderProgram::Link() %s: Program linked, but received some log:\n%s",
						  mName.c_str(), mLinkLog.c_str() );
		}

		mUniformLocations.clear();
		mAttributeLocations.clear();
	}

	return mValid;
}

void ShaderProgram::bind() const {
	GlobalBatchRenderer::instance()->draw();

	GLi->setShader( const_cast<ShaderProgram*>( this ) );
}

void ShaderProgram::unbind() const {
	GlobalBatchRenderer::instance()->draw();

	GLi->setShader( NULL );
}

Int32 ShaderProgram::getUniformLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mUniformLocations.find( Name );
	if ( it == mUniformLocations.end() ) {
#ifdef EE_SHADERS_SUPPORTED
		Int32 Location = glGetUniformLocation( getHandler(), Name.c_str() );
		mUniformLocations[Name] = Location;
#endif
	}
	return mUniformLocations[Name];
}

Int32 ShaderProgram::getAttributeLocation( const std::string& Name ) {
	if ( !mValid )
		return -1;

	std::map<std::string, Int32>::iterator it = mAttributeLocations.find( Name );
	if ( it == mAttributeLocations.end() ) {
#ifdef EE_SHADERS_SUPPORTED
		Int32 Location = glGetAttribLocation( getHandler(), Name.c_str() );
		mAttributeLocations[Name] = Location;
#endif
	}
	return mAttributeLocations[Name];
}

void ShaderProgram::invalidateLocations() {
	mUniformLocations.clear();
	mAttributeLocations.clear();
}

bool ShaderProgram::setUniform( const std::string& Name, float Value ) {
	return setUniform( getUniformLocation( Name ), Value );
}

bool ShaderProgram::setUniform( const std::string& Name, Vector2ff Value ) {
	return setUniform( getUniformLocation( Name ), Value );
}

bool ShaderProgram::setUniform( const std::string& Name, Vector3ff Value ) {
	return setUniform( getUniformLocation( Name ), Value );
}

bool ShaderProgram::setUniform( const std::string& Name, float x, float y, float z, float w ) {
	return setUniform( getUniformLocation( Name ), x, y, z, w );
}

bool ShaderProgram::setUniform( const std::string& Name, Int32 Value ) {
	return setUniform( getUniformLocation( Name ), Value );
}

bool ShaderProgram::setUniform( const Int32& Location, Int32 Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		glUniform1i( Location, Value );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, float Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		glUniform1f( Location, Value );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, Vector2ff Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		glUniform2fv( Location, 1, reinterpret_cast<float*>( &Value ) );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, Vector3ff Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		glUniform3fv( Location, 1, reinterpret_cast<float*>( &Value ) );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, float x, float y, float z, float w ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		glUniform4f( Location, x, y, z, w );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniformMatrix( const Int32& Location, const float* Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		glUniformMatrix4fv( Location, 1, false, Value );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniformMatrix( const std::string Name, const float* Value ) {
	return setUniformMatrix( getUniformLocation( Name ), Value );
}

const std::string& ShaderProgram::getName() const {
	return mName;
}

void ShaderProgram::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );

	Uint32 NameCount = ShaderProgramManager::instance()->exists( mName );

	if ( 0 != NameCount || 0 == name.size() ) {
		setName( name + String::toString( NameCount + 1 ) );
	}
}

void ShaderProgram::setReloadCb( ShaderProgramReloadCb Cb ) {
	mReloadCb = Cb;
}

void ShaderProgram::enableVertexAttribArray( const std::string& Name ) {
	enableVertexAttribArray( getAttributeLocation( Name ) );
}

void ShaderProgram::enableVertexAttribArray( const Int32& Location ) {
#ifdef EE_SHADERS_SUPPORTED
	glEnableVertexAttribArray( Location );
#endif
}

void ShaderProgram::disableVertexAttribArray( const std::string& Name ) {
	disableVertexAttribArray( getAttributeLocation( Name ) );
}

void ShaderProgram::disableVertexAttribArray( const Int32& Location ) {
#ifdef EE_SHADERS_SUPPORTED
	glDisableVertexAttribArray( Location );
#endif
}

}} // namespace EE::Graphics
