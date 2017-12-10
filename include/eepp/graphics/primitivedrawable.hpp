#ifndef EE_GRAPHICS_PRIMITIVEDRAWABLE
#define EE_GRAPHICS_PRIMITIVEDRAWABLE

#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

class VertexBuffer;

class EE_API PrimitiveDrawable : public Drawable {
	public:
		virtual ~PrimitiveDrawable();

		virtual void draw( const Vector2f& position, const Sizef& size );

		/** Set the fill mode used to draw primitives */
		virtual void setFillMode( const EE_FILL_MODE& Mode );

		/** @return The fill mode used to draw primitives */
		const EE_FILL_MODE& getFillMode() const;

		/** Set the blend mode used to draw primitives */
		virtual void setBlendMode( const BlendMode& Mode );

		/** @return The blend mode used to draw primitives */
		const BlendMode& getBlendMode() const;

		/** Set the line width to draw primitives */
		virtual void setLineWidth( const Float& width );

		/** @return The line with to draw primitives */
		const Float& getLineWidth() const;
	protected:
		PrimitiveDrawable( EE_DRAWABLE_TYPE drawableType );

		EE_FILL_MODE			mFillMode;
		BlendMode				mBlendMode;
		Float					mLineWidth;
		bool mNeedsUpdate;
		bool mRecreateVertexBuffer;
		VertexBuffer * mVertexBuffer;

		virtual void onAlphaChange();

		virtual void onColorFilterChange();

		virtual void onPositionChange();

		void prepareVertexBuffer( const EE_DRAW_MODE& drawableType );

		virtual void updateVertex() = 0;
};

}}

#endif
