#ifndef EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP
#define EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

#include <eepp/system/container.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics { namespace Private {

class EE_API VertexBufferManager : public Container<VertexBuffer> {
	SINGLETON_DECLARE_HEADERS( VertexBufferManager )

  public:
	virtual ~VertexBufferManager();

	void reload();

  protected:
	VertexBufferManager();
};

}}} // namespace EE::Graphics::Private

#endif
