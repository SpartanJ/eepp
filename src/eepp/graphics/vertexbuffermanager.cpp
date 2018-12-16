#include <eepp/graphics/vertexbuffermanager.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION(VertexBufferManager)

VertexBufferManager::VertexBufferManager()
{
}

VertexBufferManager::~VertexBufferManager()
{
}

void VertexBufferManager::reload() {
	std::list<VertexBuffer*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); ++it )
		(*it)->reload();
}

}}}
