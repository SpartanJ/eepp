#ifndef EE_GRAPHICS_PRIVATE_RENDERERHELPER_HPP
#define EE_GRAPHICS_PRIVATE_RENDERERHELPER_HPP

#include <eepp/graphics/renderer/base.hpp>

#ifdef EE_GL3_ENABLED

#include <stack>
#include <eepp/helper/glm/gtx/transform.hpp>

namespace EE { namespace Graphics { namespace Private {

static void fromGLMmat4( glm::mat4 from, float * to ) {
	memcpy( to, &from[0][0], sizeof(glm::mat4) );
}

static glm::mat4 toGLMmat4( const float * m ) {
	glm::mat4 Result;
	memcpy( &Result[0][0], m, sizeof(glm::mat4));
	return Result;
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
