#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include "base.hpp"

namespace EE { namespace UI {
	
class cUIBackground {
	public:
		cUIBackground();
		cUIBackground( const cUIBackground& Back );

		const eeColorA& Color() const;
		void Color( const eeColorA& Col );

		const EE_RENDERALPHAS& Blend() const;
		void Blend( const EE_RENDERALPHAS& blend );
	protected:
		eeColorA			mColor;
		EE_RENDERALPHAS		mBlendMode;
};

}}

#endif 
