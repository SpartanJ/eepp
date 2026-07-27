#include <eepp/graphics/framebufferregistry.hpp>
#include <eepp/graphics/renderer/openglext.hpp>

namespace EE { namespace Graphics { namespace Private {

SINGLETON_DECLARE_IMPLEMENTATION( FrameBufferRegistry )

FrameBufferRegistry::FrameBufferRegistry() {}

FrameBufferRegistry::~FrameBufferRegistry() {}

FrameBuffer* FrameBufferRegistry::getCurrentlyBound() {
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

FrameBuffer* FrameBufferRegistry::getFromName( const std::string& name ) {
	return getFromId( String::hash( name ) );
}

FrameBuffer* FrameBufferRegistry::getFromId( const String::HashType& id ) {
	for ( auto& fb : mResources ) {
		if ( fb->getId() == id ) {
			return fb;
		}
	}

	return NULL;
}

}}} // namespace EE::Graphics::Private
