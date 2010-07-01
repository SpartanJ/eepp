#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include "base.hpp"

namespace EE { namespace UI {

class cUIBackground {
	public:
		cUIBackground();
		cUIBackground( const eeColorA& Color, const eeUint& Corners = 0, const EE_RENDERALPHAS& BlendMode = ALPHA_NORMAL );
		cUIBackground( const cUIBackground& Back );
		cUIBackground( const eeColorA& TopLeftColor, const eeColorA& BottomLeftColor, const eeColorA& BottomRightColor, const eeColorA& TopRightColor, const eeUint& Corners, const EE_RENDERALPHAS& BlendMode );

		eeColorA& Color( const eeUint& index = 0 );

		void Color( const eeColorA& Col );

		const std::vector<eeColorA>& Colors();

		void Colors( const eeColorA& TopLeftColor, const eeColorA& BottomLeftColor, const eeColorA& BottomRightColor, const eeColorA& TopRightColor );

		void ColorsTo( const eeColorA& Color );

		const EE_RENDERALPHAS& Blend() const;
		void Blend( const EE_RENDERALPHAS& blend );

		const eeUint& Corners() const;
		void Corners( const eeUint& corners );
	protected:
		std::vector<eeColorA>	mColor;

		EE_RENDERALPHAS			mBlendMode;
		eeUint					mCorners;
};

}}

#endif
