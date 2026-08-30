#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/inputmethod.hpp>
#include <eepp/window/window.hpp>

#include <algorithm>
#include <limits>

namespace EE { namespace Window {

static constexpr Uint32 DEFAULT_LOCATION_UPDATE_INTERVAL_MS = 50;

static Uint16 getLocationUpdateInterval() {
	static Uint16 interval = [] {
		std::string value = Sys::getEnv( "EEPP_IME_LOCATION_UPDATE_INTERVAL_MS" );
		Uint32 parsedValue;
		if ( value.empty() || !String::fromString( parsedValue, value ) )
			return static_cast<Uint16>( DEFAULT_LOCATION_UPDATE_INTERVAL_MS );
		return static_cast<Uint16>(
			std::min<Uint32>( parsedValue, std::numeric_limits<Uint16>::max() ) );
	}();
	return interval;
}

InputMethod::InputMethod( EE::Window::Window* window ) : mWindow( window ) {}

void InputMethod::sendTextEditing( const String& txt, Int32 start, Int32 length ) {
	for ( const auto& cb : mEditingCbs )
		cb.second( txt, start, length );
}

void InputMethod::setLocation( Rect rect ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
	rect = PixelDensity::pxToDpI( rect );
#endif
	if ( rect != mLastLocation ) {
		mLastLocation = std::move( rect );
		mLocationDirty = true;
	}
}

void InputMethod::updateLocation() {
	double elapsed = mWindow->getElapsed().asMilliseconds();
	if ( elapsed >= mLocationUpdateCooldown )
		mLocationUpdateCooldown = 0;
	else
		mLocationUpdateCooldown -= static_cast<Uint16>( elapsed );

	if ( !mLocationDirty )
		return;

	if ( mLocationUpdateCooldown > 0 )
		return;

	// SDL3 can synchronously notify the platform IME (including a DBus round-trip on Linux).
	// Apply only the latest requested location at a bounded rate and keep the backend call on the
	// main thread, as required by SDL.
	mLocationDirty = false;
	mLocationUpdateCooldown = getLocationUpdateInterval();
	mWindow->setTextInputRect( mLastLocation );
}

bool InputMethod::isEditing() const {
	return mEditing;
}

void InputMethod::reset() {
	mEditing = false;
	mState = {};
}

void InputMethod::stop() {
	if ( mEditing ) {
		mWindow->clearComposition();
		reset();
		sendTextEditing( "", 0, 0 );
	}
}

void InputMethod::onTextEditing( const String& text, const Int32& start, const Int32& length ) {
	if ( text.empty() ) {
		reset();
	} else {
		mEditing = true;
		mState = { text, start, length };
	}
}

const InputMethod::State& InputMethod::getState() const {
	return mState;
}

void InputMethod::draw( const Vector2f& screenPos, const Float& lineHeight,
						const FontStyleConfig& fontStyle, const Color& lineColor,
						const Color& backgroundColor, bool drawText ) {
	Float width = Text::getTextWidth( mState.text, fontStyle );
	Primitives p;

	if ( backgroundColor != Color::Transparent ) {
		p.setColor( backgroundColor );
		p.drawRectangle( { screenPos, { width, lineHeight } } );
	}

	if ( lineColor != Color::Transparent ) {
		Float lh = PixelDensity::dpToPx( 2 );
		p.setColor( lineColor );
		p.drawRectangle(
			Rectf( { screenPos.x, screenPos.y + lineHeight - lh * 0.5f }, { width, lh } ) );

		Float lineOffsetX =
			Text::getTextWidth( mState.text.view().substr( 0, mState.start ), fontStyle );
		p.drawRectangle( Rectf( { screenPos.x + lineOffsetX, screenPos.y },
								{ PixelDensity::dpToPx( 1.f ), lineHeight } ) );
	}

	if ( drawText )
		Text::draw( mState.text, screenPos, fontStyle );
}

Uint32 InputMethod::addTextEditingCb( TextEditingCb cb ) {
	mEditingCbs[++mLastCb] = std::move( cb );
	return mLastCb;
}

void InputMethod::removeTextEditingCb( Uint32 cbId ) {
	mEditingCbs.erase( cbId );
}

}} // namespace EE::Window
