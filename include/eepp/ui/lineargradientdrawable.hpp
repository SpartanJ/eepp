#ifndef EE_UI_LINEARGRADIENTDRAWABLE_HPP
#define EE_UI_LINEARGRADIENTDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/system/color.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <vector>

namespace EE { namespace UI {

class EE_API LinearGradientDrawable : public Graphics::Drawable {
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

		Float getNormalized( Float gradientLineLength ) const {
			if ( unit == CSS::StyleSheetLength::Percentage )
				return eemax( 0.f, value ) / 100.f;
			if ( gradientLineLength > 0.0001f )
				return eemax( 0.f, value ) / gradientLineLength;
			return 0.f;
		}
	};

	static LinearGradientDrawable* New();
	static LinearGradientDrawable* NewRepeating();

	LinearGradientDrawable(
		Graphics::Drawable::Type drawableType = Graphics::Drawable::LINEARGRADIENT );

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	const std::vector<ColorStop>& getColorStops() const;

	void setColorStops( std::vector<ColorStop> stops );

	Float getAngle() const;

	void setAngle( Float angleDegrees );

	void setSize( const Sizef& size );

	bool isRepeating() const;

  protected:
	std::vector<ColorStop> mColorStops;
	Float mAngle{ 180.f };
	Sizef mSize;
};

}} // namespace EE::UI

#endif
