#ifndef EE_GRAPHICS_PRIVATE_RENDERERHELPER_HPP
#define EE_GRAPHICS_PRIVATE_RENDERERHELPER_HPP

#include <eepp/graphics/renderer/base.hpp>

#ifdef EE_GL3_ENABLED

#include <eepp/helper/glm/gtx/transform.hpp>

namespace EE { namespace Graphics { namespace Private {

static void fromGLMmat4( glm::mat4 from, GLfloat * to ) {
	Int32 i,p;

	for ( i = 0; i < 4; i++ ) {
		glm::vec4 v = from[i];
		p		= i * 4;
		to[p  ]	= v.x;
		to[p+1]	= v.y;
		to[p+2]	= v.z;
		to[p+3]	= v.w;
	}
}

static glm::mat4 toGLMmat4( const GLfloat * m ) {
	return glm::mat4( m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15] );
}

class cMatrixStack
{
	public:
		std::stack<glm::mat4>	mProjectionMatrix;		// cpu-side
		std::stack<glm::mat4>	mModelViewMatrix;		// cpu-side
		std::stack<glm::mat4>*	mCurMatrix;
};

}}}

using namespace EE::Graphics::Private;

#endif

#endif

