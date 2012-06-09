#include <eepp/graphics/cvertexbuffermanager.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION(cVertexBufferManager)

cVertexBufferManager::cVertexBufferManager()
{
}

cVertexBufferManager::~cVertexBufferManager()
{
}

void cVertexBufferManager::Reload() {
	std::list<cVertexBuffer*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->Reload();
}

}}}
