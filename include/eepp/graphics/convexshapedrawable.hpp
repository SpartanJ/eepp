#ifndef EE_GRAPHICS_CONVEXSHAPEDRAWABLE_HPP
#define EE_GRAPHICS_CONVEXSHAPEDRAWABLE_HPP

#include <eepp/graphics/primitivedrawable.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

class EE_API ConvexShapeDrawable : public PrimitiveDrawable {
  public:
	static ConvexShapeDrawable* New();

	ConvexShapeDrawable();

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	void setPolygon( const Polygon2f& polygon );

	void addPoint( const Vector2f& point );

	void addPoint( const Vector2f& point, const Color& color );

	void resetPoints();

  protected:
	Polygon2f mPolygon;
	std::vector<Color> mIndexColor;

	void updateVertex();
};

}} // namespace EE::Graphics

#endif
