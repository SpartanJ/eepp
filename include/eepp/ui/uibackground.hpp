#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBackground {
	public:
		static UIBackground * New();

		UIBackground();

		ColorA& getColor( const unsigned int& index = 0 );

		UIBackground * setColor( const ColorA& Col );

		const std::vector<ColorA>& getColors();

		UIBackground * setColors( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor );

		UIBackground * setColorsTo( const ColorA& color );

		const EE_BLEND_MODE& getBlendMode() const;

		UIBackground * setBlendMode( const EE_BLEND_MODE& blend );

		const unsigned int& getCorners() const;

		UIBackground * setCorners( const unsigned int& corners );
	protected:
		std::vector<ColorA>	mColor;

		EE_BLEND_MODE		mBlendMode;
		unsigned int		mCorners;
};

}}

#endif
