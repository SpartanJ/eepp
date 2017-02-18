#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBackground {
	public:
		UIBackground();
		UIBackground( const ColorA& color, const unsigned int& corners = 0, const EE_BLEND_MODE& BlendMode = ALPHA_NORMAL );
		UIBackground( const UIBackground& Back );
		UIBackground( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor, const unsigned int& corners, const EE_BLEND_MODE& BlendMode );

		ColorA& getColor( const unsigned int& index = 0 );

		void setColor( const ColorA& Col );

		const std::vector<ColorA>& getColors();

		void setColors( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor );

		void setColorsTo( const ColorA& color );

		const EE_BLEND_MODE& getBlendMode() const;

		void setBlendMode( const EE_BLEND_MODE& blend );

		const unsigned int& getCorners() const;

		void setCorners( const unsigned int& corners );
	protected:
		std::vector<ColorA>	mColor;

		EE_BLEND_MODE		mBlendMode;
		unsigned int		mCorners;
};

}}

#endif
