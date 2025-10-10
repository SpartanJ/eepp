#ifndef EE_GRAPHICS_BLENDMODE_HPP
#define EE_GRAPHICS_BLENDMODE_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace Graphics {

class EE_API BlendMode {
  public:
	enum class Factor : Uint8 {
		Zero,			  /// (0, 0, 0, 0)
		One,			  /// (1, 1, 1, 1)
		SrcColor,		  /// (src.r, src.g, src.b, src.a)
		OneMinusSrcColor, /// (1, 1, 1, 1) - (src.r, src.g, src.b, src.a)
		DstColor,		  /// (dst.r, dst.g, dst.b, dst.a)
		OneMinusDstColor, /// (1, 1, 1, 1) - (dst.r, dst.g, dst.b, dst.a)
		SrcAlpha,		  /// (src.a, src.a, src.a, src.a)
		OneMinusSrcAlpha, /// (1, 1, 1, 1) - (src.a, src.a, src.a, src.a)
		DstAlpha,		  /// (dst.a, dst.a, dst.a, dst.a)
		OneMinusDstAlpha  /// (1, 1, 1, 1) - (dst.a, dst.a, dst.a, dst.a)
	};

	enum class Equation : Uint8 {
		Add,			/// Pixel = Src * SrcFactor + Dst * DstFactor
		Subtract,		/// Pixel = Src * SrcFactor - Dst * DstFactor
		ReverseSubtract /// Pixel = Dst * DstFactor - Src * SrcFactor
	};

	/// Blend source and dest according to dest alpha
	static constexpr BlendMode Alpha() {
		return { Factor::SrcAlpha, Factor::OneMinusSrcAlpha, Equation::Add,
				 Factor::One,	   Factor::OneMinusSrcAlpha, Equation::Add };
	}

	/// Add source to dest
	static constexpr BlendMode Add() {
		return { Factor::SrcAlpha, Factor::One, Equation::Add,
				 Factor::One,	   Factor::One, Equation::Add };
	}

	/// Multiply source and dest
	static constexpr BlendMode Multiply() { return { Factor::DstColor, Factor::Zero }; }

	/// Overwrite dest with source
	static constexpr BlendMode None() { return { Factor::One, Factor::Zero }; }

	static std::string equationToString( const Equation& eq );

	static std::string factorToString( const Factor& fc );

	constexpr BlendMode() :
		colorSrcFactor( Factor::SrcAlpha ),
		colorDstFactor( Factor::OneMinusSrcAlpha ),
		colorEquation( Equation::Add ),
		alphaSrcFactor( Factor::One ),
		alphaDstFactor( Factor::OneMinusSrcAlpha ),
		alphaEquation( Equation::Add ) {}

	constexpr BlendMode( Factor sourceFactor, Factor destinationFactor,
						 Equation blendEquation = Equation::Add ) :
		colorSrcFactor( sourceFactor ),
		colorDstFactor( destinationFactor ),
		colorEquation( blendEquation ),
		alphaSrcFactor( sourceFactor ),
		alphaDstFactor( destinationFactor ),
		alphaEquation( blendEquation ) {}

	constexpr BlendMode( Factor colorSourceFactor, Factor colorDestinationFactor,
						 Equation colorBlendEquation, Factor alphaSourceFactor,
						 Factor alphaDestinationFactor, Equation alphaBlendEquation ) :
		colorSrcFactor( colorSourceFactor ),
		colorDstFactor( colorDestinationFactor ),
		colorEquation( colorBlendEquation ),
		alphaSrcFactor( alphaSourceFactor ),
		alphaDstFactor( alphaDestinationFactor ),
		alphaEquation( alphaBlendEquation ) {}

	/** Set a Predefined Blend Function
	 * @param mode The Blend Mode
	 * @param force If force to apply the blend ( no matters if the last blend was the same blend )
	 */
	static void setMode( const BlendMode& mode, bool force = false );

	/** @return The last used predefined blend func */
	static BlendMode getPreBlendFunc();

	Factor colorSrcFactor;	/// Source blending factor for the color channels
	Factor colorDstFactor;	/// Destination blending factor for the color channels
	Equation colorEquation; /// Blending equation for the color channels
	Factor alphaSrcFactor;	/// Source blending factor for the alpha channel
	Factor alphaDstFactor;	/// Destination blending factor for the alpha channel
	Equation alphaEquation; /// Blending equation for the alpha channel

	std::string toString() const;

  protected:
	static BlendMode sLastBlend;
};

constexpr bool operator==( const BlendMode& left, const BlendMode& right ) {
	return ( left.colorSrcFactor == right.colorSrcFactor ) &&
		   ( left.colorDstFactor == right.colorDstFactor ) &&
		   ( left.colorEquation == right.colorEquation ) &&
		   ( left.alphaSrcFactor == right.alphaSrcFactor ) &&
		   ( left.alphaDstFactor == right.alphaDstFactor ) &&
		   ( left.alphaEquation == right.alphaEquation );
}

constexpr bool operator!=( const BlendMode& left, const BlendMode& right ) {
	return !( left == right );
}

}} // namespace EE::Graphics

#endif
