#include <eepp/graphics/shaderprogramregistry.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( ShaderProgramRegistry )

ShaderProgramRegistry::ShaderProgramRegistry() {}

ShaderProgramRegistry::~ShaderProgramRegistry() {}

void ShaderProgramRegistry::reload() {
	for ( auto* program : mResources )
		program->reload();
}

}} // namespace EE::Graphics
