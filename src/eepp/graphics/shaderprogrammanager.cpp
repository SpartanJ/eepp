#include <eepp/graphics/shaderprogrammanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( ShaderProgramManager )

ShaderProgramManager::ShaderProgramManager() {}

ShaderProgramManager::~ShaderProgramManager() {}

void ShaderProgramManager::reload() {
	for ( auto& res : mResources )
		res.second->reload();
}

}} // namespace EE::Graphics
