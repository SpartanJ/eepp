#ifndef EE_GRAPHICS_DRAWABLE_HPP
#define EE_GRAPHICS_DRAWABLE_HPP

#include <eepp/math/size.hpp>
#include <eepp/system/color.hpp>
#include <eepp/graphics/graphicshelper.hpp>
using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace Graphics {

class EE_API Drawable {
	public:
		virtual Sizef getSize() = 0;

		virtual void draw() = 0;

		virtual void draw( const Vector2f& position ) = 0;

		virtual void draw( const Vector2f& position, const Sizef& size ) = 0;

		void setAlpha( Uint8 alpha );

		const Uint8& getAlpha();

		void setColor( const Color& color );

		const Color& getColor() const;

		void setColorFilter( const Color& color );

		RGB getColorFilter();

		void clearColor();

		void clearColorFilter();

		void resetAlpha();

		EE_DRAWABLE_TYPE getDrawableType() const;

		const Vector2f& getPosition() const;

		void setPosition( const Vector2f& position );
	protected:
		EE_DRAWABLE_TYPE mDrawableType;
		Color mColor;
		Vector2f mPosition;

		Drawable( EE_DRAWABLE_TYPE drawableType );

		virtual void onAlphaChange();

		virtual void onColorFilterChange();

		virtual void onPositionChange();
};

}}

#endif
