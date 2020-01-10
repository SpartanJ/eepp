#ifndef EE_PIXELDENSITY_HPP
#define EE_PIXELDENSITY_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>
#include <string>

using namespace EE::Math;

namespace EE { namespace Graphics {

enum class PixelDensitySize : Uint32 {
	MDPI = 1,	//!< 1dp = 1px
	HDPI = 2,	//!< 1dp = 1.5px
	XHDPI = 3,	//!< 1dp = 2px
	XXHDPI = 4, //!< 1dp = 3px
	XXXHDPI = 5 //!< 1dp = 4px
};

class EE_API PixelDensity {
  public:
	static Float toFloat( PixelDensitySize pd );

	static Float toFloat( Uint32 pd );

	static PixelDensitySize fromString( std::string str );

	static PixelDensitySize fromString( String str );

	static PixelDensitySize fromDPI( Float dpi );

	static const Float& getPixelDensity();

	static void setPixelDensity( const Float& pixelDensity );

	static void setPixelDensity( const PixelDensitySize& pixelDensity );

	static Float pxToDp( Float px );

	static Int32 pxToDpI( Float px );

	static Float dpToPx( Float dp );

	static Int32 dpToPxI( Float dp );

	static Sizei dpToPxI( Sizei size );

	static Sizei pxToDpI( Sizei size );

	static Rect dpToPxI( Rect size );

	static Rect pxToDpI( Rect size );

	static Rectf dpToPx( Rectf size );

	static Rectf pxToDp( Rectf size );

	static Sizef dpToPx( Sizef size );

	static Sizef pxToDp( Sizef size );

	static Sizei dpToPxI( Sizef size );

	static Sizei pxToDpI( Sizef size );

	static Vector2i dpToPxI( Vector2i pos );

	static Vector2i pxToDpI( Vector2i pos );

	static Vector2f dpToPx( Vector2f pos );

	static Vector2f pxToDp( Vector2f pos );

	static Float toDpFromString( const std::string& str );

	static Float toDpFromStringI( const std::string& str );

  protected:
	static Float sPixelDensity;
};

}} // namespace EE::Graphics

#endif
