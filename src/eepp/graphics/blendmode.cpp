#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/log.hpp>

namespace EE { namespace Graphics {

Uint32 factorToGlConstant( BlendMode::Factor blendFactor ) {
	switch ( blendFactor ) {
		case BlendMode::Factor::Zero:
			return GL_ZERO;
		case BlendMode::Factor::One:
			return GL_ONE;
		case BlendMode::Factor::SrcColor:
			return GL_SRC_COLOR;
		case BlendMode::Factor::OneMinusSrcColor:
			return GL_ONE_MINUS_SRC_COLOR;
		case BlendMode::Factor::DstColor:
			return GL_DST_COLOR;
		case BlendMode::Factor::OneMinusDstColor:
			return GL_ONE_MINUS_DST_COLOR;
		case BlendMode::Factor::SrcAlpha:
			return GL_SRC_ALPHA;
		case BlendMode::Factor::OneMinusSrcAlpha:
			return GL_ONE_MINUS_SRC_ALPHA;
		case BlendMode::Factor::DstAlpha:
			return GL_DST_ALPHA;
		case BlendMode::Factor::OneMinusDstAlpha:
			return GL_ONE_MINUS_DST_ALPHA;
	}

	Log::warning( "Invalid value for BlendMode::Factor! Fallback to BlendMode::Zero." );

	return GL_ZERO;
}

Uint32 equationToGlConstant( BlendMode::Equation blendEquation ) {
	switch ( blendEquation ) {
		case BlendMode::Equation::Add:
			return GL_FUNC_ADD;
		case BlendMode::Equation::Subtract:
			return GL_FUNC_SUBTRACT;
		case BlendMode::Equation::ReverseSubtract:
			return GL_FUNC_REVERSE_SUBTRACT;
	}

	Log::warning( "Invalid value for BlendMode::Equation! Fallback to BlendMode::Add." );

	return GL_FUNC_ADD;
}

BlendMode BlendMode::Alpha() {
	static const BlendMode BlendAlpha{ BlendMode::Factor::SrcAlpha,
									   BlendMode::Factor::OneMinusSrcAlpha,
									   BlendMode::Equation::Add,
									   BlendMode::Factor::One,
									   BlendMode::Factor::OneMinusSrcAlpha,
									   BlendMode::Equation::Add };
	return BlendAlpha;
}

BlendMode BlendMode::Add() {
	static const BlendMode BlendAdd{ BlendMode::Factor::SrcAlpha, BlendMode::Factor::One,
									 BlendMode::Equation::Add,	  BlendMode::Factor::One,
									 BlendMode::Factor::One,	  BlendMode::Equation::Add };
	return BlendAdd;
}

BlendMode BlendMode::Multiply() {
	static const BlendMode BlendMultiply{ BlendMode::Factor::DstColor, BlendMode::Factor::Zero };
	return BlendMultiply;
}

BlendMode BlendMode::None() {
	static const BlendMode BlendNone{ BlendMode::Factor::One, BlendMode::Factor::Zero };
	return BlendNone;
}

BlendMode BlendMode::sLastBlend = BlendMode::Add();

std::string BlendMode::equationToString( const Equation& eq ) {
	switch ( eq ) {
		case BlendMode::Equation::Add:
			return "Add";
		case BlendMode::Equation::Subtract:
			return "Substract";
		case BlendMode::Equation::ReverseSubtract:
			return "ReverseSubtract";
	}
	return "";
}

std::string BlendMode::factorToString( const Factor& fc ) {
	switch ( fc ) {
		case BlendMode::Factor::Zero:
			return "Zero";
		case BlendMode::Factor::One:
			return "One";
		case BlendMode::Factor::SrcColor:
			return "SrcColor";
		case BlendMode::Factor::OneMinusSrcColor:
			return "OneMinusSrcColor";
		case BlendMode::Factor::DstColor:
			return "DstColor";
		case BlendMode::Factor::OneMinusDstColor:
			return "OneMinusDstColor";
		case BlendMode::Factor::SrcAlpha:
			return "SrcAlpha";
		case BlendMode::Factor::OneMinusSrcAlpha:
			return "OneMinusSrcAlpha";
		case BlendMode::Factor::DstAlpha:
			return "DstAlpha";
		case BlendMode::Factor::OneMinusDstAlpha:
			return "OneMinusDstAlpha";
	}
	return "";
}

BlendMode::BlendMode() :
	colorSrcFactor( BlendMode::Factor::SrcAlpha ),
	colorDstFactor( BlendMode::Factor::OneMinusSrcAlpha ),
	colorEquation( BlendMode::Equation::Add ),
	alphaSrcFactor( BlendMode::Factor::One ),
	alphaDstFactor( BlendMode::Factor::OneMinusSrcAlpha ),
	alphaEquation( BlendMode::Equation::Add ) {}

BlendMode::BlendMode( Factor sourceFactor, Factor destinationFactor, Equation blendEquation ) :
	colorSrcFactor( sourceFactor ),
	colorDstFactor( destinationFactor ),
	colorEquation( blendEquation ),
	alphaSrcFactor( sourceFactor ),
	alphaDstFactor( destinationFactor ),
	alphaEquation( blendEquation ) {}

BlendMode::BlendMode( Factor colorSourceFactor, Factor colorDestinationFactor,
					  Equation colorBlendEquation, Factor alphaSourceFactor,
					  Factor alphaDestinationFactor, Equation alphaBlendEquation ) :
	colorSrcFactor( colorSourceFactor ),
	colorDstFactor( colorDestinationFactor ),
	colorEquation( colorBlendEquation ),
	alphaSrcFactor( alphaSourceFactor ),
	alphaDstFactor( alphaDestinationFactor ),
	alphaEquation( alphaBlendEquation ) {}

bool operator==( const BlendMode& left, const BlendMode& right ) {
	return ( left.colorSrcFactor == right.colorSrcFactor ) &&
		   ( left.colorDstFactor == right.colorDstFactor ) &&
		   ( left.colorEquation == right.colorEquation ) &&
		   ( left.alphaSrcFactor == right.alphaSrcFactor ) &&
		   ( left.alphaDstFactor == right.alphaDstFactor ) &&
		   ( left.alphaEquation == right.alphaEquation );
}

bool operator!=( const BlendMode& left, const BlendMode& right ) {
	return !( left == right );
}

void BlendMode::setMode( const BlendMode& mode, bool force ) {
	if ( sLastBlend != mode || force ) {
		GLi->enable( GL_BLEND );

		if ( GLi->isExtension( EEGL_EXT_blend_func_separate ) ) {
			GLi->blendFuncSeparate( factorToGlConstant( mode.colorSrcFactor ),
									factorToGlConstant( mode.colorDstFactor ),
									factorToGlConstant( mode.alphaSrcFactor ),
									factorToGlConstant( mode.alphaDstFactor ) );
		} else {
			GLi->blendFunc( factorToGlConstant( mode.colorSrcFactor ),
							factorToGlConstant( mode.colorDstFactor ) );
		}

		if ( GLi->isExtension( EEGL_EXT_blend_minmax ) &&
			 GLi->isExtension( EEGL_EXT_blend_subtract ) ) {
			GLi->blendEquationSeparate( equationToGlConstant( mode.colorEquation ),
										equationToGlConstant( mode.alphaEquation ) );
		}

		sLastBlend = mode;
	}
}

BlendMode BlendMode::getPreBlendFunc() {
	return sLastBlend;
}

std::string BlendMode::toString() const {
	return factorToString( colorSrcFactor ) + " " + factorToString( colorDstFactor ) + " " +
		   equationToString( colorEquation ) + " " + factorToString( alphaSrcFactor ) + " " +
		   factorToString( alphaDstFactor ) + " " + equationToString( alphaEquation );
}

}} // namespace EE::Graphics
