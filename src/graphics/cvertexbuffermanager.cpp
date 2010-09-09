#include "cvertexbuffermanager.hpp"

namespace EE { namespace Graphics { namespace Private {

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
