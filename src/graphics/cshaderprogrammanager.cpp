#include "cshaderprogrammanager.hpp"

namespace EE { namespace Graphics {

cShaderProgramManager::cShaderProgramManager()
{
}

cShaderProgramManager::~cShaderProgramManager()
{
	std::map<std::string, cShaderProgram*>::iterator it;

	for ( it = mShaders.begin(); it != mShaders.end(); it++ )
		delete it->second;
}

void cShaderProgramManager::Add( cShaderProgram * ShaderProgram ) {
	Uint32 c = mShaders.count( ShaderProgram->Name() );

	if ( 0 == c ) {
		mShaders[ ShaderProgram->Name() ] = ShaderProgram;
	} else {
		ShaderProgram->Name( ShaderProgram->Name() + intToStr( c + 1 ) );

		Add( ShaderProgram );
	}
}

void cShaderProgramManager::Remove( cShaderProgram * ShaderProgram ) {
	mShaders.erase( ShaderProgram->Name() );
}

Uint32 cShaderProgramManager::ExistsName( const std::string& name ) {
	return mShaders.count( name );
}

cShaderProgram * cShaderProgramManager::GetByName( const std::string& Name ) {
	std::map<std::string, cShaderProgram*>::iterator it = mShaders.find( Name );

	if ( mShaders.end() != it ) {
		return it->second;
	}

	return NULL;
}

cShaderProgram * cShaderProgramManager::GetById( Uint32 id ) {
	std::map<std::string, cShaderProgram*>::iterator it;

	for ( it = mShaders.begin(); it != mShaders.end(); it++ ) {
		cShaderProgram * sp = reinterpret_cast< cShaderProgram* > ( it->second );

		if ( id == sp->GetId() )
			return sp;
	}

	return NULL;
}

eeUint cShaderProgramManager::Count() {
	return mShaders.size();
}

void cShaderProgramManager::Reload() {
	std::map<std::string, cShaderProgram*>::iterator it;

	for ( it = mShaders.begin(); it != mShaders.end(); it++ ) {
		cShaderProgram * sp = reinterpret_cast< cShaderProgram* > ( it->second );

		sp->Reload();
	}
}

}}
