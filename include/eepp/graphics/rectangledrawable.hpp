#ifndef EE_GRAPHICS_RECTANGLEDRAWABLE
#define EE_GRAPHICS_RECTANGLEDRAWABLE

#include <eepp/graphics/primitivedrawable.hpp>

namespace EE { namespace Graphics {

class RectangleDrawable : PrimitiveDrawable {
	public:
		RectangleDrawable();

		~RectangleDrawable();

		virtual Sizef getSize();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

};

}}

#endif
