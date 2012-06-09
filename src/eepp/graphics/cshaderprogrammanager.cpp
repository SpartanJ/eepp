#include <eepp/graphics/cshaderprogrammanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cShaderProgramManager)

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
