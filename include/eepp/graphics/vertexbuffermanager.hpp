#ifndef EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP
#define EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

#include <eepp/system/container.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics { namespace Private {

/** Non-owning registry of vertex buffers visible to the active graphics context. */
class EE_API VertexBufferRegistry : public Container<VertexBuffer> {
	SINGLETON_DECLARE_HEADERS( VertexBufferRegistry )

  public:
	virtual ~VertexBufferRegistry();

	void reload();

  protected:
	VertexBufferRegistry();
};

}}} // namespace EE::Graphics::Private

#endif
