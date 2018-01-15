#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>

namespace EE { namespace Graphics {

Uint32 factorToGlConstant(BlendMode::Factor blendFactor) {
	switch (blendFactor) {
		case BlendMode::Zero:             return GL_ZERO;
		case BlendMode::One:              return GL_ONE;
		case BlendMode::SrcColor:         return GL_SRC_COLOR;
		case BlendMode::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case BlendMode::DstColor:         return GL_DST_COLOR;
		case BlendMode::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
		case BlendMode::SrcAlpha:         return GL_SRC_ALPHA;
		case BlendMode::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case BlendMode::DstAlpha:         return GL_DST_ALPHA;
		case BlendMode::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
	}

	eePRINTL( "Invalid value for BlendMode::Factor! Fallback to BlendMode::Zero." );

	return GL_ZERO;
}

Uint32 equationToGlConstant(BlendMode::Equation blendEquation) {
	switch (blendEquation) {
		case BlendMode::Add:             return GL_FUNC_ADD;
		case BlendMode::Subtract:        return GL_FUNC_SUBTRACT;
		case BlendMode::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
	}

	eePRINTL( "Invalid value for BlendMode::Equation! Fallback to BlendMode::Add." );

	return GL_FUNC_ADD;
}

const BlendMode BlendAlpha(BlendMode::SrcAlpha, BlendMode::OneMinusSrcAlpha, BlendMode::Add, BlendMode::One, BlendMode::OneMinusSrcAlpha, BlendMode::Add);
const BlendMode BlendAdd(BlendMode::SrcAlpha, BlendMode::One, BlendMode::Add, BlendMode::One, BlendMode::One, BlendMode::Add);
const BlendMode BlendMultiply(BlendMode::DstColor, BlendMode::Zero);
const BlendMode BlendNone(BlendMode::One, BlendMode::Zero);

BlendMode BlendMode::sLastBlend = BlendAlpha;

BlendMode::BlendMode() :
	colorSrcFactor(BlendMode::SrcAlpha),
	colorDstFactor(BlendMode::OneMinusSrcAlpha),
	colorEquation (BlendMode::Add),
	alphaSrcFactor(BlendMode::One),
	alphaDstFactor(BlendMode::OneMinusSrcAlpha),
	alphaEquation (BlendMode::Add)
{}

BlendMode::BlendMode(Factor sourceFactor, Factor destinationFactor, Equation blendEquation) :
	colorSrcFactor(sourceFactor),
	colorDstFactor(destinationFactor),
	colorEquation (blendEquation),
	alphaSrcFactor(sourceFactor),
	alphaDstFactor(destinationFactor),
	alphaEquation (blendEquation)
{}

BlendMode::BlendMode(Factor colorSourceFactor, Factor colorDestinationFactor, Equation colorBlendEquation, Factor alphaSourceFactor, Factor alphaDestinationFactor, Equation alphaBlendEquation) :
	colorSrcFactor(colorSourceFactor),
	colorDstFactor(colorDestinationFactor),
	colorEquation (colorBlendEquation),
	alphaSrcFactor(alphaSourceFactor),
	alphaDstFactor(alphaDestinationFactor),
	alphaEquation (alphaBlendEquation)
{}

bool operator ==(const BlendMode& left, const BlendMode& right) {
	return	(left.colorSrcFactor == right.colorSrcFactor) &&
			(left.colorDstFactor == right.colorDstFactor) &&
			(left.colorEquation  == right.colorEquation)  &&
			(left.alphaSrcFactor == right.alphaSrcFactor) &&
			(left.alphaDstFactor == right.alphaDstFactor) &&
			(left.alphaEquation  == right.alphaEquation);
}

bool operator !=(const BlendMode& left, const BlendMode& right) {
	return !(left == right);
}

void BlendMode::setMode( const BlendMode & mode, bool force ) {
	if ( sLastBlend != mode || force ) {
		GLi->enable( GL_BLEND );

		if ( GLi->isExtension( EEGL_EXT_blend_func_separate ) ) {
			GLi->blendFuncSeparate( factorToGlConstant(mode.colorSrcFactor), factorToGlConstant(mode.colorDstFactor),
									factorToGlConstant(mode.alphaSrcFactor), factorToGlConstant(mode.alphaDstFactor) );
		} else {
			GLi->blendFunc( factorToGlConstant(mode.colorSrcFactor), factorToGlConstant(mode.colorDstFactor) );
		}

		if ( GLi->isExtension( EEGL_EXT_blend_minmax ) && GLi->isExtension( EEGL_EXT_blend_subtract ) ) {
			GLi->blendEquationSeparate( equationToGlConstant(mode.colorEquation), equationToGlConstant(mode.alphaEquation) );
		}

		sLastBlend= mode;
	}
}

BlendMode BlendMode::getPreBlendFunc() {
	return sLastBlend;
}

}}

