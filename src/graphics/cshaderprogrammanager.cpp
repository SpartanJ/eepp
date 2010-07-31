#include "cshaderprogrammanager.hpp"

namespace EE { namespace Graphics {

cShaderProgramManager::cShaderProgramManager()
{
}

cShaderProgramManager::~cShaderProgramManager()
{
}

void cShaderProgramManager::Reload() {
	std::map<std::string, cShaderProgram*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		cShaderProgram * sp = reinterpret_cast< cShaderProgram* > ( it->second );

		sp->Reload();
	}
}

}}
