#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/graphics/shaderprogramregistry.hpp>
#include <eepp/system/log.hpp>

namespace EE { namespace Graphics {

ShaderProgramPtr ShaderProgram::New( const std::string& name ) {
	return ShaderProgramPtr( eeNew( ShaderProgram, ( name ) ), ResourceDeleter<ShaderProgram>() );
}

ShaderProgramPtr ShaderProgram::New( const std::vector<ShaderPtr>& shaders,
									 const std::string& name ) {
	return ShaderProgramPtr( eeNew( ShaderProgram, ( shaders, name ) ),
							 ResourceDeleter<ShaderProgram>() );
}

ShaderProgramPtr ShaderProgram::New( const std::string& VertexShaderFile,
									 const std::string& FragmentShaderFile,
									 const std::string& name ) {
	return ShaderProgramPtr( eeNew( ShaderProgram, ( VertexShaderFile, FragmentShaderFile, name ) ),
							 ResourceDeleter<ShaderProgram>() );
}

ShaderProgramPtr ShaderProgram::New( const char* VertexShaderData,
									 const Uint32& VertexShaderDataSize,
									 const char* FragmentShaderData,
									 const Uint32& FragmentShaderDataSize,
									 const std::string& name ) {
	return ShaderProgramPtr(
		eeNew( ShaderProgram, ( VertexShaderData, VertexShaderDataSize, FragmentShaderData,
								FragmentShaderDataSize, name ) ),
		ResourceDeleter<ShaderProgram>() );
}

ShaderProgramPtr ShaderProgram::New( Pack* Pack, const std::string& VertexShaderPath,
									 const std::string& FragmentShaderPath,
									 const std::string& name ) {
	return ShaderProgramPtr(
		eeNew( ShaderProgram, ( Pack, VertexShaderPath, FragmentShaderPath, name ) ),
		ResourceDeleter<ShaderProgram>() );
}

ShaderProgramPtr ShaderProgram::New( const char** VertexShaderData, const Uint32& NumLinesVS,
									 const char** FragmentShaderData, const Uint32& NumLinesFS,
									 const std::string& name ) {
	return ShaderProgramPtr( eeNew( ShaderProgram, ( VertexShaderData, NumLinesVS,
													 FragmentShaderData, NumLinesFS, name ) ),
							 ResourceDeleter<ShaderProgram>() );
}

ShaderProgram::ShaderProgram( const std::string& name ) : mHandler( 0 ), mId( 0 ) {
	addToRegistry( name );
	init();
}

ShaderProgram::ShaderProgram( const std::vector<ShaderPtr>& Shaders, const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToRegistry( name );
	init();

	addShaders( Shaders );

	link();
}

ShaderProgram::ShaderProgram( const std::string& VertexShaderFile,
							  const std::string& FragmentShaderFile, const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToRegistry( name );
	init();

	ShaderPtr vs( eeNew( VertexShader, ( VertexShaderFile ) ), ResourceDeleter<Shader>() );
	ShaderPtr fs( eeNew( FragmentShader, ( FragmentShaderFile ) ), ResourceDeleter<Shader>() );

	if ( !vs->isValid() || !fs->isValid() ) {
		return;
	}

	addShader( vs );
	addShader( fs );

	link();
}

ShaderProgram::ShaderProgram( Pack* Pack, const std::string& VertexShaderPath,
							  const std::string& FragmentShaderPath, const std::string& name ) :
	mHandler( 0 ), mId( 0 ) {
	addToRegistry( name );
	init();

	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( VertexShaderPath ) &&
		 -1 != Pack->exists( FragmentShaderPath ) ) {
		ShaderPtr vs( eeNew( VertexShader, ( Pack, VertexShaderPath ) ),
					  ResourceDeleter<Shader>() );
		ShaderPtr fs( eeNew( FragmentShader, ( Pack, FragmentShaderPath ) ),
					  ResourceDeleter<Shader>() );

		if ( !vs->isValid() || !fs->isValid() ) {
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
	addToRegistry( name );
	init();

	ShaderPtr vs( eeNew( VertexShader, ( VertexShaderData, VertexShaderDataSize ) ),
				  ResourceDeleter<Shader>() );
	ShaderPtr fs( eeNew( FragmentShader, ( FragmentShaderData, FragmentShaderDataSize ) ),
				  ResourceDeleter<Shader>() );

	if ( !vs->isValid() || !fs->isValid() ) {
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
	addToRegistry( name );
	init();

	ShaderPtr vs( eeNew( VertexShader, ( VertexShaderData, NumLinesVS ) ),
				  ResourceDeleter<Shader>() );
	ShaderPtr fs( eeNew( FragmentShader, ( FragmentShaderData, NumLinesFS ) ),
				  ResourceDeleter<Shader>() );

	if ( !vs->isValid() || !fs->isValid() ) {
		return;
	}

	addShader( vs );
	addShader( fs );

	link();
}

ShaderProgram::~ShaderProgram() {
	if ( getHandler() > 0 ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->deleteProgram( getHandler() );
#endif
	}

	mUniformLocations.clear();
	mAttributeLocations.clear();

	removeFromRegistry();
}

void ShaderProgram::addToRegistry( const std::string& name ) {
	setName( name );

	ShaderProgramRegistry::instance()->add( this );
}

void ShaderProgram::removeFromRegistry() {
	if ( ShaderProgramRegistry::existsSingleton() )
		ShaderProgramRegistry::instance()->remove( this );
}

void ShaderProgram::init() {
	if ( GLi->shadersSupported() && 0 == getHandler() ) {
#ifdef EE_SHADERS_SUPPORTED
		mHandler = GLi->createProgram();
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

	std::vector<ShaderPtr> tmpShader = mShaders;

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

void ShaderProgram::addShader( ShaderPtr shader ) {
	if ( !shader || !shader->isValid() ) {
		Log::error( "ShaderProgram::addShader() %s: Cannot add invalid shader", mName.c_str() );
		return;
	}

	if ( 0 != getHandler() ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->attachShader( getHandler(), shader->getId() );
#endif

		mShaders.emplace_back( std::move( shader ) );
	}
}

void ShaderProgram::addShaders( const std::vector<ShaderPtr>& shaders ) {
	for ( const auto& shader : shaders )
		addShader( shader );
}

bool ShaderProgram::bindFragDataLocationIndexed( Uint32 colorNumber, Uint32 index,
												 const char* name ) {
	if ( !GLi->bindFragDataLocationIndexed( getHandler(), colorNumber, index, name ) )
		return false;
	mFragmentOutputBindings.push_back( { colorNumber, index, name } );
	return true;
}

bool ShaderProgram::link() {
#ifdef EE_SHADERS_SUPPORTED
	for ( const auto& binding : mFragmentOutputBindings )
		GLi->bindFragDataLocationIndexed( getHandler(), binding.colorNumber, binding.index,
										  binding.name.c_str() );
	GLi->linkProgram( getHandler() );

	Int32 linked;
	GLi->getProgramiv( getHandler(), GL_LINK_STATUS, &linked );
	mValid = 0 != linked;

	int logsize = 0, logarraysize = 0;
	GLi->getProgramiv( getHandler(), GL_INFO_LOG_LENGTH, &logarraysize );

	if ( logarraysize > 0 ) {
		mLinkLog.resize( logarraysize );

		GLi->getProgramInfoLog( getHandler(), logarraysize, &logsize, &mLinkLog[0] );

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
		Int32 Location = GLi->getUniformLocation( getHandler(), Name.c_str() );
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
		Int32 Location = GLi->getAttribLocation( getHandler(), Name.c_str() );
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
		GLi->uniform1i( Location, Value );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, float Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->uniform1f( Location, Value );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, Vector2ff Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->uniform2fv( Location, 1, reinterpret_cast<float*>( &Value ) );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, Vector3ff Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->uniform3fv( Location, 1, reinterpret_cast<float*>( &Value ) );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniform( const Int32& Location, float x, float y, float z, float w ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->uniform4f( Location, x, y, z, w );
#endif

		return true;
	}

	return false;
}

bool ShaderProgram::setUniformMatrix( const Int32& Location, const float* Value ) {
	if ( -1 != Location ) {
#ifdef EE_SHADERS_SUPPORTED
		GLi->uniformMatrix4fv( Location, 1, false, Value );
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
}

void ShaderProgram::setReloadCb( ShaderProgramReloadCb Cb ) {
	mReloadCb = Cb;
}

void ShaderProgram::enableVertexAttribArray( const std::string& Name ) {
	enableVertexAttribArray( getAttributeLocation( Name ) );
}

void ShaderProgram::enableVertexAttribArray( const Int32& Location ) {
#ifdef EE_SHADERS_SUPPORTED
	GLi->enableVertexAttribArray( Location );
#endif
}

void ShaderProgram::disableVertexAttribArray( const std::string& Name ) {
	disableVertexAttribArray( getAttributeLocation( Name ) );
}

void ShaderProgram::disableVertexAttribArray( const Int32& Location ) {
#ifdef EE_SHADERS_SUPPORTED
	GLi->disableVertexAttribArray( Location );
#endif
}

}} // namespace EE::Graphics
