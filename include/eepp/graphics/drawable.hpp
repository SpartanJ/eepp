#ifndef EE_GRAPHICS_DRAWABLE_HPP
#define EE_GRAPHICS_DRAWABLE_HPP

#include <eepp/math/size.hpp>
#include <eepp/system/color.hpp>
using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace Graphics {

class EE_API Drawable {
	public:
		Drawable();

		virtual Sizef getSize() = 0;

		virtual void draw( const Vector2f& position ) = 0;

		virtual void draw( const Vector2f& position, const Sizef& size ) = 0;

		void setAlpha( Uint8 alpha );

		const Uint8& getAlpha();

		void setColorFilter( Color color );

		const Color& getColorFilter();

		void clearColorFilter();

		void resetAlpha();

	protected:
		Color mColorFilter;
		Uint8 mAlpha;

		virtual void onAlphaChange();

		virtual void onColorFilterChange();

		ColorA getColorFilterAlpha();
};

}}

#endif
