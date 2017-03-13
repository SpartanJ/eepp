#ifndef EE_PIXELDENSITY_HPP
#define EE_PIXELDENSITY_HPP

#include <eepp/config.hpp>
#include <string>
#include <eepp/math/size.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/vector2.hpp>

using namespace EE::Math;

namespace EE { namespace Graphics {

enum EE_PIXEL_DENSITY {
	PD_MDPI = 1,	//!< 1dp = 1px
	PD_HDPI = 2,	//!< 1dp = 1.5px
	PD_XHDPI = 3,	//!< 1dp = 2px
	PD_XXHDPI = 4,	//!< 1dp = 3px
	PD_XXXHDPI = 5	//!< 1dp = 4px
};

class EE_API PixelDensity {
	public:
		static Float toFloat( EE_PIXEL_DENSITY pd ) {
			switch ( pd )
			{
				case PD_MDPI: return 1.f;
				case PD_HDPI: return 1.5f;
				case PD_XHDPI: return 2.f;
				case PD_XXHDPI: return 3.f;
				case PD_XXXHDPI: return 4.f;
				default: return 1.f;
			}
		}

		static Float toFloat( Uint32 pd ) {
			return toFloat( (EE_PIXEL_DENSITY)pd );
		}

		static const Float& getPixelDensity();

		static void setPixelDensity( const Float& pixelDensity );

		static void setPixelDensity( const EE_PIXEL_DENSITY& pixelDensity );

		static Float pxToDp( Float px );

		static Int32 pxToDpI( Float px );

		static Float dpToPx( Float dp );

		static Int32 dpToPxI( Float dp );

		static Sizei dpToPxI(Sizei size);

		static Sizei pxToDpI( Sizei size );

		static Recti dpToPxI( Recti size);

		static Recti pxToDpI( Recti size );

		static Rectf dpToPx( Rectf size);

		static Rectf pxToDp( Rectf size );

		static Sizef dpToPx( Sizef size);

		static Sizef pxToDp( Sizef size );

		static Sizei dpToPxI( Sizef size);

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

}}

#endif
