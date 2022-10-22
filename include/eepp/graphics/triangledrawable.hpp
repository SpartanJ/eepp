#ifndef EE_GRAPHICS_TRIANGLEDRAWABLE_HPP
#define EE_GRAPHICS_TRIANGLEDRAWABLE_HPP

#include <eepp/graphics/primitivedrawable.hpp>
#include <eepp/math/triangle2.hpp>

namespace EE { namespace Graphics {

class EE_API TriangleDrawable : public PrimitiveDrawable {
  public:
	static TriangleDrawable* New();

	static TriangleDrawable* New( const Vector2f& position, const Sizef& size );

	TriangleDrawable();

	TriangleDrawable( const Vector2f& position, const Sizef& size );

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	void setSize( const Sizef& size );

	const Triangle2f& getTriangle() const;

	void setTriangle( const Triangle2f& triangle );

	void setTriangleColors( const Color& color1, const Color& color2, const Color& color3 );

  protected:
	Triangle2f mTriangle;
	Triangle2f mComputedTriangle;
	Sizef mSize;
	Color mColors[3];
	bool mCustomColors;

	virtual void onColorFilterChange();

	virtual void updateVertex();
};

}} // namespace EE::Graphics

#endif // EE_GRAPHICS_TRIANGLEDRAWABLE_HPP
