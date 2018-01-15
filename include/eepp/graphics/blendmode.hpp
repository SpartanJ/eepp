#ifndef EE_GRAPHICS_BLENDMODE_HPP
#define EE_GRAPHICS_BLENDMODE_HPP

#include <eepp/config.hpp>

namespace EE { namespace Graphics {

class EE_API BlendMode {
	public:
		enum Factor
		{
			Zero,             /// (0, 0, 0, 0)
			One,              /// (1, 1, 1, 1)
			SrcColor,         /// (src.r, src.g, src.b, src.a)
			OneMinusSrcColor, /// (1, 1, 1, 1) - (src.r, src.g, src.b, src.a)
			DstColor,         /// (dst.r, dst.g, dst.b, dst.a)
			OneMinusDstColor, /// (1, 1, 1, 1) - (dst.r, dst.g, dst.b, dst.a)
			SrcAlpha,         /// (src.a, src.a, src.a, src.a)
			OneMinusSrcAlpha, /// (1, 1, 1, 1) - (src.a, src.a, src.a, src.a)
			DstAlpha,         /// (dst.a, dst.a, dst.a, dst.a)
			OneMinusDstAlpha  /// (1, 1, 1, 1) - (dst.a, dst.a, dst.a, dst.a)
		};

		enum Equation
		{
			Add,            /// Pixel = Src * SrcFactor + Dst * DstFactor
			Subtract,       /// Pixel = Src * SrcFactor - Dst * DstFactor
			ReverseSubtract /// Pixel = Dst * DstFactor - Src * SrcFactor
		};

		BlendMode();

		BlendMode(Factor sourceFactor, Factor destinationFactor, Equation blendEquation = Add);

		BlendMode(Factor colorSourceFactor, Factor colorDestinationFactor,
				  Equation colorBlendEquation, Factor alphaSourceFactor,
				  Factor alphaDestinationFactor, Equation alphaBlendEquation);

		/** Set a Predefined Blend Function
		* @param blend The Blend Mode
		* @param force If force to apply the blend ( no matters if the last blend was the same blend )
		*/
		static void setMode( const BlendMode& mode, bool force = false );

		/** @return The last used predefined blend func */
		static BlendMode getPreBlendFunc();

		Factor   colorSrcFactor; /// Source blending factor for the color channels
		Factor   colorDstFactor; /// Destination blending factor for the color channels
		Equation colorEquation;  /// Blending equation for the color channels
		Factor   alphaSrcFactor; /// Source blending factor for the alpha channel
		Factor   alphaDstFactor; /// Destination blending factor for the alpha channel
		Equation alphaEquation;  /// Blending equation for the alpha channel

	protected:
		static BlendMode sLastBlend;

};

EE_API bool operator ==(const BlendMode& left, const BlendMode& right);
EE_API bool operator !=(const BlendMode& left, const BlendMode& right);

EE_API extern const BlendMode BlendAlpha;    /// Blend source and dest according to dest alpha
EE_API extern const BlendMode BlendAdd;      /// Add source to dest
EE_API extern const BlendMode BlendMultiply; /// Multiply source and dest
EE_API extern const BlendMode BlendNone;     /// Overwrite dest with source

}}

#endif
