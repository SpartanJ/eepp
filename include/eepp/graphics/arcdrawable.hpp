#ifndef EE_GRAPHICS_ARCDRAWABLE 
#define EE_GRAPHICS_ARCDRAWABLE

#include <eepp/graphics/primitivedrawable.hpp>

namespace EE { namespace Graphics {

class ArcDrawable : public PrimitiveDrawable {
	public:
		ArcDrawable( const Float& radius, Uint32 segmentsCount = 0, const Float& arcAngle = 360.f , const Float& arcStartAngle = 0.f );

		virtual Sizef getSize();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );
	protected:
		Float mRadius;
		Uint32 mSegmentsCount;
		Float mArcAngle;
		Float mArcStartAngle;
};

}}

#endif
