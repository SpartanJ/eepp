#ifndef EE_GRAPHICS_ARCDRAWABLE 
#define EE_GRAPHICS_ARCDRAWABLE

#include <eepp/core.hpp>
#include <eepp/graphics/primitivedrawable.hpp>

namespace EE { namespace Graphics {

class ArcDrawable : public PrimitiveDrawable {
	public:
		ArcDrawable();

		ArcDrawable( const Float& radius, Uint32 segmentsCount = 64, const Float& arcAngle = 360.f , const Float& arcStartAngle = 0.f );

		virtual Sizef getSize();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		Float getRadius() const;

		void setRadius(const Float & radius);

		Float getArcAngle() const;

		void setArcAngle(const Float & arcAngle);

		Float getArcStartAngle() const;

		void setArcStartAngle(const Float & arcStartAngle);

		Uint32 getSegmentsCount() const;

		void setSegmentsCount(const Uint32 & segmentsCount);
	protected:
		Float mRadius;
		Uint32 mSegmentsCount;
		Float mArcAngle;
		Float mArcStartAngle;

		void updateVertex();
};

}}

#endif
