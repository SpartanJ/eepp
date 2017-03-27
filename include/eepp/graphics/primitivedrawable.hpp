#ifndef EE_GRAPHICS_PRIMITIVEDRAWABLE
#define EE_GRAPHICS_PRIMITIVEDRAWABLE

#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

class PrimitiveDrawable : public Drawable {
	public:
		PrimitiveDrawable( EE_DRAWABLE_TYPE drawableType );

		/** Set the fill mode used to draw primitives */
		void setFillMode( const EE_FILL_MODE& Mode );

		/** @return The fill mode used to draw primitives */
		const EE_FILL_MODE& getFillMode() const;

		/** Set the blend mode used to draw primitives */
		void setBlendMode( const EE_BLEND_MODE& Mode );

		/** @return The blend mode used to draw primitives */
		const EE_BLEND_MODE& getBlendMode() const;

		/** Set the line width to draw primitives */
		void setLineWidth( const Float& width );

		/** @return The line with to draw primitives */
		const Float& getLineWidth() const;
	protected:
		EE_FILL_MODE			mFillMode;
		EE_BLEND_MODE			mBlendMode;
		Float					mLineWidth;
};

}}

#endif
