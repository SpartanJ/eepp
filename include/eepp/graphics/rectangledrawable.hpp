#ifndef EE_GRAPHICS_RECTANGLEDRAWABLE
#define EE_GRAPHICS_RECTANGLEDRAWABLE

#include <eepp/graphics/primitivedrawable.hpp>
#include <eepp/math/rect.hpp>

namespace EE { namespace Graphics {

class EE_API RectangleDrawable : public PrimitiveDrawable {
	public:
		RectangleDrawable();

		RectangleDrawable( const Vector2f& position, const Sizef& size );

		virtual Sizef getSize();

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		Float getRotation() const;

		void setRotation(const Float & rotation);

		Vector2f getScale() const;

		void setScale(const Vector2f & scale);

		void setSize(const Sizef & size);

		Uint32 getCorners() const;

		void setCorners(const Uint32 & corners);

		RectColors getRectColors() const;

		void setRectColors(const RectColors & rectColors);
	protected:
		Sizef mSize;
		Float mRotation;
		Vector2f mScale;
		Uint32 mCorners;
		RectColors mRectColors;
		bool mUsingRectColors;

		void drawRectangle( const Rectf& R, const ColorA& TopLeft, const ColorA& BottomLeft, const ColorA& BottomRight, const ColorA& TopRight, const Float& Angle, const Vector2f& Scale );

		void updateVertex();

		virtual void onColorFilterChange();
};

}}

#endif
