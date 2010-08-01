#include "cshaderprogrammanager.hpp"

namespace EE { namespace Graphics {

cShaderProgramManager::cShaderProgramManager()
{
}

cShaderProgramManager::~cShaderProgramManager()
{
}

void cShaderProgramManager::Reload() {
	std::list<cShaderProgram*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->Reload();
}

}}
