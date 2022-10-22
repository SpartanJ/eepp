#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/renderer/openglext.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION( FrameBufferManager )

FrameBufferManager::FrameBufferManager() {}

FrameBufferManager::~FrameBufferManager() {}

void FrameBufferManager::reload() {
	for ( auto& fb : mResources )
		fb->reload();
}

FrameBuffer* FrameBufferManager::getCurrentlyBound() {
	int curFB;

	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	if ( 0 != curFB ) {
		for ( auto& fb : mResources ) {
			if ( fb->getFrameBufferId() == curFB ) {
				return fb;
			}
		}
	}

	return NULL;
}

FrameBuffer* FrameBufferManager::getFromName( const std::string& name ) {
	return getFromId( String::hash( name ) );
}

FrameBuffer* FrameBufferManager::getFromId( const String::HashType& id ) {
	for ( auto& fb : mResources ) {
		if ( fb->getId() == id ) {
			return fb;
		}
	}

	return NULL;
}

}}} // namespace EE::Graphics::Private
