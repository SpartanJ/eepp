#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics {

EE_BLEND_MODE BlendMode::sLastBlend = ALPHA_NORMAL;

void BlendMode::SetMode( const EE_BLEND_MODE& blend, bool force ) {
	if ( sLastBlend != blend || force ) {
		if (blend == ALPHA_NONE) {
			GLi->disable( GL_BLEND );
		} else {
			GLi->enable( GL_BLEND );

			switch (blend) {
				case ALPHA_NORMAL:
					if ( GLi->isExtension( EEGL_EXT_blend_func_separate ) )
						glBlendFuncSeparateEXT( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
					else
						GLi->blendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);
					break;
				case ALPHA_BLENDONE:
					if ( GLi->isExtension( EEGL_EXT_blend_func_separate ) )
						glBlendFuncSeparateEXT( GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE );
					else
						GLi->blendFunc(GL_SRC_ALPHA , GL_ONE);
					break;
				case ALPHA_BLENDTWO:
					GLi->blendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA);
					GLi->blendFunc(GL_DST_ALPHA , GL_ONE);
					break;
				case ALPHA_BLENDTHREE:
					GLi->blendFunc(GL_SRC_ALPHA , GL_ONE);
					GLi->blendFunc(GL_DST_ALPHA , GL_SRC_ALPHA);
					break;
				case ALPHA_ALPHACHANNELS:
					GLi->blendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA);
					break;
				case ALPHA_DESTALPHA:
					GLi->blendFunc(GL_SRC_ALPHA , GL_DST_ALPHA);
					break;
				case ALPHA_MULTIPLY:
					GLi->blendFunc(GL_DST_COLOR,GL_ZERO);
					break;
				case ALPHA_NONE:
					// Avoid compiler warning
					break;
				case ALPHA_CUSTOM:
					break;
			}

		}

		sLastBlend= blend;
	}
}

void BlendMode::SetBlendFunc( const EE_BLEND_FUNC& SrcFactor, const EE_BLEND_FUNC& DestFactor ) {
	GLi->enable( GL_BLEND );

	GLi->blendFunc( (unsigned int)SrcFactor, (unsigned int)DestFactor );

	sLastBlend = ALPHA_CUSTOM;
}

EE_BLEND_MODE BlendMode::GetPreBlendFunc() {
	return sLastBlend;
}

}}

