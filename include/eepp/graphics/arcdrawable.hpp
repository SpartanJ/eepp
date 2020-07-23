#ifndef EE_GRAPHICS_ARCDRAWABLE
#define EE_GRAPHICS_ARCDRAWABLE

#include <eepp/core.hpp>
#include <eepp/graphics/primitivedrawable.hpp>

namespace EE { namespace Graphics {

class EE_API ArcDrawable : public PrimitiveDrawable {
  public:
	static ArcDrawable* New();

	static ArcDrawable* New( const Float& radius, Uint32 segmentsCount = 64,
							 const Float& arcAngle = 360.f, const Float& arcStartAngle = 0.f );

	ArcDrawable();

	ArcDrawable( const Float& radius, Uint32 segmentsCount = 64, const Float& arcAngle = 360.f,
				 const Float& arcStartAngle = 0.f );

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	Float getRadius() const;

	void setRadius( const Float& radius );

	Float getArcAngle() const;

	void setArcAngle( const Float& arcAngle );

	Float getArcStartAngle() const;

	void setArcStartAngle( const Float& arcStartAngle );

	Uint32 getSegmentsCount() const;

	void setSegmentsCount( const Uint32& segmentsCount );

	const Vector2f& getOffset() const;

	void setOffset( const Vector2f& offset );

  protected:
	Float mRadius;
	Uint32 mSegmentsCount;
	Float mArcAngle;
	Float mArcStartAngle;
	Vector2f mOffset;

	void updateVertex();
};

}} // namespace EE::Graphics

#endif
