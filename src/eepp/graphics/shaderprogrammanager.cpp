#include <eepp/graphics/shaderprogrammanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(ShaderProgramManager)

ShaderProgramManager::ShaderProgramManager()
{
}

ShaderProgramManager::~ShaderProgramManager()
{
}

void ShaderProgramManager::reload() {
	std::list<ShaderProgram*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->reload();
}

}}
