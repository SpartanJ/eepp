#ifndef EE_GRAPHICS_BLENDMODE_HPP
#define EE_GRAPHICS_BLENDMODE_HPP

#include <eepp/declares.hpp>
#include <eepp/graphics/renders.hpp>

namespace EE { namespace Graphics {

class EE_API BlendMode {
	public:
		/** Set a blend function.
		* @param SrcFactor Source Factor
		* @param DestFactor Destination Factor
		*/
		static void SetBlendFunc( const EE_BLEND_FUNC& SrcFactor, const EE_BLEND_FUNC& DestFactor );

		/** Set a Predefined Blend Function
		* @param blend The Blend Mode
		* @param force If force to apply the blend ( no matters if the last blend was the same blend )
		*/
		static void SetMode( const EE_BLEND_MODE& blend, bool force = false );

		/** @return The last used predefined blend func */
		static EE_BLEND_MODE GetPreBlendFunc();
	protected:
		static EE_BLEND_MODE sLastBlend;
};

}}

#endif
