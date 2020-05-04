#include <eepp/graphics/vertexbuffermanager.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION( VertexBufferManager )

VertexBufferManager::VertexBufferManager() {}

VertexBufferManager::~VertexBufferManager() {}

void VertexBufferManager::reload() {
	for ( auto& vb : mResources )
		vb->reload();
}

}}} // namespace EE::Graphics::Private
