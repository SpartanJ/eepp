#ifndef EE_UICUIBACKGROUND_HPP
#define EE_UICUIBACKGROUND_HPP

#include <eepp/ui/base.hpp>
#include <eepp/graphics/drawable.hpp>

namespace EE { namespace UI {

class EE_API UIBackground {
	public:
		static UIBackground * New();

		UIBackground();

		~UIBackground();

		Color& getColor( const unsigned int& index = 0 );

		UIBackground * setColor( const Color& Col );

		const std::vector<Color>& getColors();

		UIBackground * setColors( const Color& TopLeftColor, const Color& BottomLeftColor, const Color& BottomRightColor, const Color& TopRightColor );

		UIBackground * setColorsTo( const Color& color );

		const EE_BLEND_MODE& getBlendMode() const;

		UIBackground * setBlendMode( const EE_BLEND_MODE& blend );

		const unsigned int& getCorners() const;

		UIBackground * setCorners( const unsigned int& corners );

		void draw( Rectf R );

		Drawable * getDrawable() const;

		void setDrawable( Drawable * drawable, bool ownIt );

	protected:
		std::vector<Color>	mColor;

		EE_BLEND_MODE		mBlendMode;
		unsigned int		mCorners;
		Drawable *			mDrawable;
		bool				mOwnIt;
};

}}

#endif
