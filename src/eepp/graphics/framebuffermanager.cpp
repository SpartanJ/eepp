#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/renderer/openglext.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION(FrameBufferManager)

FrameBufferManager::FrameBufferManager()
{
}

FrameBufferManager::~FrameBufferManager()
{
}

void FrameBufferManager::reload() {
	std::list<FrameBuffer*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->reload();
}

FrameBuffer * FrameBufferManager::getCurrentlyBound() {
	int curFB;

	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	if ( 0 != curFB ) {
		std::list<FrameBuffer*>::iterator it;

		for ( it = mResources.begin(); it != mResources.end(); it++ ) {
			if ( (*it)->getFrameBufferId() == curFB ) {
				return (*it);
			}
		}
	}

	return NULL;
}

}}}
