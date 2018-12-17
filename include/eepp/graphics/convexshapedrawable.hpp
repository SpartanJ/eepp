#ifndef EE_GRAPHICS_CONVEXSHAPEDRAWABLE_HPP
#define EE_GRAPHICS_CONVEXSHAPEDRAWABLE_HPP

#include <eepp/graphics/primitivedrawable.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

class EE_API ConvexShapeDrawable : public PrimitiveDrawable {
	public:
		static ConvexShapeDrawable * New();

		ConvexShapeDrawable();

		virtual Sizef getSize();

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		virtual bool isStateful() { return false; }

		void setPolygon( const Polygon2f& polygon );

		void addPoint( const Vector2f& point );

		void resetPoints();
	protected:
		Polygon2f mPolygon;

		void updateVertex();
};

}}

#endif
