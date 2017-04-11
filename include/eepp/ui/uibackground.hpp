#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class EE_API UIBackground {
	public:
		static UIBackground * New();

		UIBackground();

		Color& getColor( const unsigned int& index = 0 );

		UIBackground * setColor( const Color& Col );

		const std::vector<Color>& getColors();

		UIBackground * setColors( const Color& TopLeftColor, const Color& BottomLeftColor, const Color& BottomRightColor, const Color& TopRightColor );

		UIBackground * setColorsTo( const Color& color );

		const EE_BLEND_MODE& getBlendMode() const;

		UIBackground * setBlendMode( const EE_BLEND_MODE& blend );

		const unsigned int& getCorners() const;

		UIBackground * setCorners( const unsigned int& corners );
	protected:
		std::vector<Color>	mColor;

		EE_BLEND_MODE		mBlendMode;
		unsigned int		mCorners;
};

}}

#endif
