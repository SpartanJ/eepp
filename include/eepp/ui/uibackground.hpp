#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBackground {
	public:
		UIBackground();
		UIBackground( const ColorA& Color, const unsigned int& Corners = 0, const EE_BLEND_MODE& BlendMode = ALPHA_NORMAL );
		UIBackground( const UIBackground& Back );
		UIBackground( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor, const unsigned int& Corners, const EE_BLEND_MODE& BlendMode );

		ColorA& Color( const unsigned int& index = 0 );

		void Color( const ColorA& Col );

		const std::vector<ColorA>& Colors();

		void Colors( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor );

		void ColorsTo( const ColorA& Color );

		const EE_BLEND_MODE& Blend() const;
		void Blend( const EE_BLEND_MODE& blend );

		const unsigned int& Corners() const;
		void Corners( const unsigned int& corners );
	protected:
		std::vector<ColorA>	mColor;

		EE_BLEND_MODE		mBlendMode;
		unsigned int					mCorners;
};

}}

#endif
