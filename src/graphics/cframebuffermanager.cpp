#include "cframebuffermanager.hpp"

namespace EE { namespace Graphics { namespace Private {

cFrameBufferManager::cFrameBufferManager()
{
}

cFrameBufferManager::~cFrameBufferManager()
{
}

void cFrameBufferManager::Reload() {
	std::list<cFrameBuffer*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->Reload();
}

}}}
