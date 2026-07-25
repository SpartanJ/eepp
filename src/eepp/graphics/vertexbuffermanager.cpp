#include <eepp/graphics/vertexbuffermanager.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION( VertexBufferRegistry )

VertexBufferRegistry::VertexBufferRegistry() {}

VertexBufferRegistry::~VertexBufferRegistry() {}

void VertexBufferRegistry::reload() {
	for ( auto& vb : mResources )
		vb->reload();
}

}}} // namespace EE::Graphics::Private
