#include <eepp/graphics/framebuffermanager.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION(FrameBufferManager)

FrameBufferManager::FrameBufferManager()
{
}

FrameBufferManager::~FrameBufferManager()
{
}

void FrameBufferManager::Reload() {
	std::list<FrameBuffer*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->Reload();
}

}}}
