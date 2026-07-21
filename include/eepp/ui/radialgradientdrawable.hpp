#ifndef EE_UI_RADIALGRADIENTDRAWABLE_HPP
#define EE_UI_RADIALGRADIENTDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/system/color.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/lineargradientdrawable.hpp>
#include <vector>

namespace EE { namespace UI {

class EE_API RadialGradientDrawable : public Graphics::Drawable {
  public:
	using Length = CSS::StyleSheetLength;
	using GradientUnit = CSS::StyleSheetLength::Unit;

	struct ColorStop {
		Color color;
		Float value{ 0 };
		GradientUnit unit{ CSS::StyleSheetLength::Percentage };

		ColorStop() : color( Color::White ) {}
		ColorStop( Float val, const Color& col,
				   GradientUnit u = CSS::StyleSheetLength::Percentage ) :
			color( col ), value( val ), unit( u ) {}

		Float getNormalized( Float gradientRayLength ) const {
			if ( unit == CSS::StyleSheetLength::Percentage )
				return eemax( 0.f, value ) / 100.f;
			if ( gradientRayLength > 0.0001f )
				return eemax( 0.f, value ) / gradientRayLength;
			return 0.f;
		}
	};

	enum ShapeType { CIRCLE, ELLIPSE };
	enum Extent { CLOSEST_SIDE, FARTHEST_SIDE, CLOSEST_CORNER, FARTHEST_CORNER };

	static RadialGradientDrawable* New();
	static RadialGradientDrawable* NewRepeating();

	RadialGradientDrawable(
		Graphics::Drawable::Type drawableType = Graphics::Drawable::RADIALGRADIENT );

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	const std::vector<ColorStop>& getColorStops() const;

	void setColorStops( std::vector<ColorStop> stops );

	ShapeType getShape() const;

	void setShape( ShapeType shape );

	Extent getExtent() const;

	void setExtent( Extent extent );

	const Vector2f& getCenter() const;

	void setCenter( const Vector2f& centerNormalized );

	void setSize( const Sizef& size );

	bool isRepeating() const;

  protected:
	std::vector<ColorStop> mColorStops;
	ShapeType mShape{ CIRCLE };
	Extent mExtent{ FARTHEST_CORNER };
	Vector2f mCenter{ 0.5f, 0.5f };
	Sizef mSize;
};

}} // namespace EE::UI

#endif
