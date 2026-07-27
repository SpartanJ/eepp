#ifndef EE_GRAPHICSSHADERPROGRAREGISTRY_HPP
#define EE_GRAPHICSSHADERPROGRAREGISTRY_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/system/container.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** Non-owning registry of shader programs associated with the active graphics context. */
class EE_API ShaderProgramRegistry : public Container<ShaderProgram> {
	SINGLETON_DECLARE_HEADERS( ShaderProgramRegistry )

  public:
	virtual ~ShaderProgramRegistry();

	void reload();

  protected:
	ShaderProgramRegistry();
};

}} // namespace EE::Graphics

#endif
