#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics {

EE_BLEND_MODE BlendMode::sLastBlend = ALPHA_NORMAL;

void BlendMode::SetMode( const EE_BLEND_MODE& blend, bool force ) {
	if ( sLastBlend != blend || force ) {
		if (blend == ALPHA_NONE) {
			GLi->Disable( GL_BLEND );
		} else {
			GLi->Enable( GL_BLEND );

			switch (blend) {
				case ALPHA_NORMAL:
					if ( GLi->IsExtension( EEGL_EXT_blend_func_separate ) )
						glBlendFuncSeparateEXT( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
					else
						GLi->BlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);
					break;
				case ALPHA_BLENDONE:
					GLi->BlendFunc(GL_SRC_ALPHA , GL_ONE);
					break;
				case ALPHA_BLENDTWO:
					GLi->BlendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA);
					GLi->BlendFunc(GL_DST_ALPHA , GL_ONE);
					break;
				case ALPHA_BLENDTHREE:
					GLi->BlendFunc(GL_SRC_ALPHA , GL_ONE);
					GLi->BlendFunc(GL_DST_ALPHA , GL_SRC_ALPHA);
					break;
				case ALPHA_ALPHACHANNELS:
					GLi->BlendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA);
					break;
				case ALPHA_DESTALPHA:
					GLi->BlendFunc(GL_SRC_ALPHA , GL_DST_ALPHA);
					break;
				case ALPHA_MULTIPLY:
					GLi->BlendFunc(GL_DST_COLOR,GL_ZERO);
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
	GLi->Enable( GL_BLEND );

	GLi->BlendFunc( (GLenum)SrcFactor, (GLenum)DestFactor );

	sLastBlend = ALPHA_CUSTOM;
}

EE_BLEND_MODE BlendMode::GetPreBlendFunc() {
	return sLastBlend;
}

}}

