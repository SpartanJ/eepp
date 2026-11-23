#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <eepp/core/string.hpp>
#include <eepp/graphics/pixeldensity.hpp>

namespace EE { namespace Graphics {

namespace {

thread_local PixelDensity::Context* sCurrentContext = nullptr;
thread_local Float sScopedPixelDensity = 0.f;

} // namespace

Float PixelDensity::sPixelDensity = 1.f;

PixelDensity::Context::Context( Float density, const Float* source, bool target ) :
	mPrevious( sCurrentContext ),
	mSource( source ),
	mDensity( std::isfinite( density ) && density > 0.f ? density : 0.f ),
	mTargetBinding( target ) {
	sCurrentContext = this;
	sScopedPixelDensity = mDensity;
}

PixelDensity::Context::~Context() {
	restore();
}

PixelDensity::Context::Context( Context&& other ) noexcept :
	mPrevious( other.mPrevious ),
	mSource( other.mSource ),
	mDensity( other.mDensity ),
	mActive( other.mActive ),
	mTargetBinding( other.mTargetBinding ) {
	if ( mActive )
		other.findIncomingLink() = this;
	other.mActive = false;
}

PixelDensity::Context*& PixelDensity::Context::findIncomingLink() {
	if ( sCurrentContext == this )
		return sCurrentContext;
	auto* newer = sCurrentContext;
	while ( newer->mPrevious != this )
		newer = newer->mPrevious;
	return newer->mPrevious;
}

void PixelDensity::Context::restore() {
	if ( mActive ) {
		const bool wasCurrent = sCurrentContext == this;
		findIncomingLink() = mPrevious;
		if ( wasCurrent )
			sScopedPixelDensity = mPrevious ? mPrevious->mDensity : 0.f;
		mActive = false;
	}
}

void PixelDensity::Context::restoreKeepingTarget( const Float* source ) {
	if ( mActive && sCurrentContext == this ) {
		for ( auto* previous = mPrevious; previous; previous = previous->mPrevious ) {
			if ( !previous->mTargetBinding )
				continue;
			// Keep the enclosing scene/window density on the live target. Explicit
			// density scopes keep their own values until they unwind.
			previous->mSource = source;
			previous->mDensity =
				source && std::isfinite( *source ) && *source > 0.f ? *source : 0.f;
			break;
		}
	}
	restore();
}

PixelDensity::Context PixelDensity::makeCurrent( Float density ) {
	return Context( density );
}

PixelDensity::Context PixelDensity::bindCurrent( const Float* density ) {
	return Context( density ? *density : 0.f, density );
}

void PixelDensity::snapshotBindings( const Float* source ) {
	// Suspended contexts must also stop borrowing the value before they become current again.
	for ( auto* context = sCurrentContext; context; context = context->mPrevious ) {
		if ( context->mSource == source ) {
			context->mDensity = std::isfinite( *source ) && *source > 0.f ? *source : 0.f;
			context->mSource = nullptr;
			if ( context == sCurrentContext )
				sScopedPixelDensity = context->mDensity;
		}
	}
}

Float PixelDensity::toFloat( PixelDensitySize pd ) {
	switch ( pd ) {
		case PixelDensitySize::MDPI:
			return 1.f;
		case PixelDensitySize::HDPI:
			return 1.5f;
		case PixelDensitySize::XHDPI:
			return 2.f;
		case PixelDensitySize::XXHDPI:
			return 2.5f;
		case PixelDensitySize::XXXHDPI:
			return 3.f;
		default:
			return 1.f;
	}
}

Float PixelDensity::toFloat( Uint32 pd ) {
	return toFloat( (PixelDensitySize)pd );
}

PixelDensitySize PixelDensity::fromString( std::string str ) {
	String::toLowerInPlace( str );
	if ( "mdpi" == str )
		return PixelDensitySize::MDPI;
	else if ( "hdpi" == str )
		return PixelDensitySize::HDPI;
	else if ( "xhdpi" == str )
		return PixelDensitySize::XHDPI;
	else if ( "xxhdpi" == str )
		return PixelDensitySize::XXHDPI;
	else if ( "xxxhdpi" == str )
		return PixelDensitySize::XXXHDPI;
	return PixelDensitySize::MDPI;
}

PixelDensitySize PixelDensity::fromString( String str ) {
	return fromString( str.toUtf8() );
}

PixelDensitySize PixelDensity::fromDPI( Float dpi ) {
	PixelDensitySize pd = PixelDensitySize::MDPI;

	if ( dpi > 105 && dpi < 160 ) {
		pd = PixelDensitySize::HDPI;
	} else if ( dpi >= 160 && dpi < 240 ) {
		pd = PixelDensitySize::XHDPI;
	} else if ( dpi > 240 && dpi < 320 ) {
		pd = PixelDensitySize::XXHDPI;
	} else if ( dpi >= 320 ) {
		pd = PixelDensitySize::XXXHDPI;
	}

	return pd;
}

const Float& PixelDensity::getPixelDensity() {
	if ( sCurrentContext ) {
		if ( const Float* source = sCurrentContext->mSource )
			return *source > 0.f && std::isfinite( *source ) ? *source : sPixelDensity;
	}
	return sScopedPixelDensity > 0.f ? sScopedPixelDensity : sPixelDensity;
}

Float PixelDensity::getEnvironmentPixelDensity() {
	const char* value = std::getenv( "EEPP_PIXEL_DENSITY" );
	if ( !value || !*value )
		return 0.f;
	char* end = nullptr;
	errno = 0;
	const Float density = std::strtof( value, &end );
	return errno == 0 && end != value && *end == '\0' && std::isfinite( density ) && density > 0.f
			   ? density
			   : 0.f;
}

void PixelDensity::setPixelDensity( const Float& pixelDensity ) {
	sPixelDensity = pixelDensity;
}

void PixelDensity::setPixelDensity( const PixelDensitySize& pixelDensity ) {
	sPixelDensity = toFloat( pixelDensity );
}

Float PixelDensity::dpToPx( Float dp ) {
	return dp * getPixelDensity();
}

Int32 PixelDensity::dpToPxI( Float dp ) {
	return (Int32)dpToPx( dp );
}

Float PixelDensity::pxToDp( Float px ) {
	return px / getPixelDensity();
}

Int32 PixelDensity::pxToDpI( Float px ) {
	return (Int32)pxToDp( px );
}

Sizei PixelDensity::dpToPxI( Sizei size ) {
	return Sizei( dpToPxI( size.x ), dpToPxI( size.y ) );
}

Sizei PixelDensity::pxToDpI( Sizei size ) {
	return Sizei( pxToDpI( size.x ), pxToDpI( size.y ) );
}

Rect PixelDensity::dpToPxI( Rect rect ) {
	return rect * getPixelDensity();
}

Rect PixelDensity::pxToDpI( Rect rect ) {
	return rect / getPixelDensity();
}

Rectf PixelDensity::dpToPx( Rectf rect ) {
	return rect * getPixelDensity();
}

Rectf PixelDensity::pxToDp( Rectf rect ) {
	return rect / getPixelDensity();
}

Sizef PixelDensity::dpToPx( Sizef size ) {
	return size * getPixelDensity();
}

Sizef PixelDensity::pxToDp( Sizef size ) {
	return size / getPixelDensity();
}

Sizei PixelDensity::dpToPxI( Sizef size ) {
	return Sizei( dpToPxI( size.x ), dpToPxI( size.y ) );
}

Sizei PixelDensity::pxToDpI( Sizef size ) {
	return Sizei( pxToDpI( size.x ), pxToDpI( size.y ) );
}

Vector2i PixelDensity::dpToPxI( Vector2i pos ) {
	return Sizei( dpToPxI( pos.x ), dpToPxI( pos.y ) );
}

Vector2i PixelDensity::pxToDpI( Vector2i pos ) {
	return Sizei( pxToDpI( pos.x ), pxToDpI( pos.y ) );
}

OriginPoint PixelDensity::dpToPx( OriginPoint point ) {
	point.x = dpToPx( point.x );
	point.y = dpToPx( point.y );
	return point;
}

Vector2f PixelDensity::dpToPx( Vector2f pos ) {
	return Vector2f( dpToPx( pos.x ), dpToPx( pos.y ) );
}

Vector2f PixelDensity::pxToDp( Vector2f pos ) {
	return Vector2f( pxToDp( pos.x ), pxToDp( pos.y ) );
}

Float PixelDensity::toDpFromString( const std::string& str ) {
	std::string num;
	std::string unit;

	for ( std::size_t i = 0; i < str.size(); i++ ) {
		if ( String::isNumber( str[i], true ) || ( '-' == str[i] && i == 0 ) ||
			 ( '+' == str[i] && i == 0 ) ) {
			num += str[i];
		} else {
			unit = str.substr( i );
			break;
		}
	}

	if ( !num.empty() ) {
		Float val = 0;
		bool res = String::fromString( val, num );

		if ( res ) {
			if ( unit == "dp" || unit == "dip" ) {
				return val;
			} else if ( unit == "px" ) {
				return pxToDp( val );
			} else {
				return val;
			}
		}
	}

	return 0;
}

Float PixelDensity::toDpFromStringI( const std::string& str ) {
	return (Int32)toDpFromString( str );
}

Float PixelDensity::toPxFromString( const std::string& str ) {
	std::string num;
	std::string unit;

	for ( std::size_t i = 0; i < str.size(); i++ ) {
		if ( String::isNumber( str[i], true ) || ( '-' == str[i] && i == 0 ) ||
			 ( '+' == str[i] && i == 0 ) ) {
			num += str[i];
		} else {
			unit = str.substr( i );
			break;
		}
	}

	if ( !num.empty() ) {
		Float val = 0;
		bool res = String::fromString( val, num );

		if ( res ) {
			if ( unit == "dp" || unit == "dip" ) {
				return dpToPx( val );
			} else {
				return val;
			}
		}
	}

	return 0;
}

Float PixelDensity::toPxFromStringI( const std::string& str ) {
	return (Int32)toPxFromString( str );
}

}} // namespace EE::Graphics
