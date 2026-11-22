#include <eepp/window/platformhelper.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string_view>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
#include <eepp/window/engine.hpp>
#include <jni.h>
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/sys.hpp>
#include <memory>
#if __has_include( <X11/Xlib.h> )
#include <X11/Xlib.h>
#define EE_HAS_X11_FONT_SETTINGS
#endif
#endif

namespace EE { namespace Window {

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
using namespace System;

#ifdef EE_HAS_X11_FONT_SETTINGS
static std::string getXSettingsFont() {
	std::unique_ptr<void, decltype( &Sys::unloadObject )> library(
		Sys::loadObject( "libX11.so.6", false ), Sys::unloadObject );
	if ( !library )
		return {};
	const auto open = reinterpret_cast<decltype( &::XOpenDisplay )>(
		Sys::loadFunction( library.get(), "XOpenDisplay", false ) );
	const auto close = reinterpret_cast<decltype( &::XCloseDisplay )>(
		Sys::loadFunction( library.get(), "XCloseDisplay", false ) );
	const auto atom = reinterpret_cast<decltype( &::XInternAtom )>(
		Sys::loadFunction( library.get(), "XInternAtom", false ) );
	const auto selectionOwner = reinterpret_cast<decltype( &::XGetSelectionOwner )>(
		Sys::loadFunction( library.get(), "XGetSelectionOwner", false ) );
	const auto property = reinterpret_cast<decltype( &::XGetWindowProperty )>(
		Sys::loadFunction( library.get(), "XGetWindowProperty", false ) );
	const auto free = reinterpret_cast<decltype( &::XFree )>(
		Sys::loadFunction( library.get(), "XFree", false ) );
	const auto screen = reinterpret_cast<decltype( &::XDefaultScreen )>(
		Sys::loadFunction( library.get(), "XDefaultScreen", false ) );
	if ( !open || !close || !atom || !selectionOwner || !property || !free || !screen )
		return {};
	std::unique_ptr<::Display, decltype( close )> display( open( nullptr ), close );
	if ( !display )
		return {};
	const auto selection = "_XSETTINGS_S" + std::to_string( screen( display.get() ) );
	const Atom manager = atom( display.get(), selection.c_str(), True );
	const Atom settings = atom( display.get(), "_XSETTINGS_SETTINGS", True );
	if ( manager == None || settings == None )
		return {};
	// Read both the owner and its property while it is still registered.
	const auto grab = reinterpret_cast<decltype( &::XGrabServer )>(
		Sys::loadFunction( library.get(), "XGrabServer", false ) );
	const auto ungrab = reinterpret_cast<decltype( &::XUngrabServer )>(
		Sys::loadFunction( library.get(), "XUngrabServer", false ) );
	if ( !grab || !ungrab )
		return {};
	Atom type = None;
	int format = 0;
	unsigned long count = 0, remaining = 0;
	unsigned char* bytes = nullptr;
	grab( display.get() );
	const auto owner = selectionOwner( display.get(), manager );
	const int status = owner == None
						   ? BadWindow
						   : property( display.get(), owner, settings, 0, 64 * 1024, False,
									   settings, &type, &format, &count, &remaining, &bytes );
	ungrab( display.get() );
	std::unique_ptr<unsigned char, decltype( free )> data( bytes, free );
	if ( status != Success || type != settings || format != 8 || !data || count < 12 ||
		 bytes[0] > 1 )
		return {};
	const bool littleEndian = bytes[0] == 0;
	const auto read = [bytes, littleEndian]( size_t offset, size_t length ) {
		Uint32 value = 0;
		for ( size_t i = 0; i < length; ++i ) {
			const size_t shift = littleEndian ? i : length - 1 - i;
			value |= static_cast<Uint32>( bytes[offset + i] ) << ( shift * 8 );
		}
		return value;
	};
	const auto aligned = []( size_t length ) { return ( length + 3 ) & ~size_t{ 3 }; };
	const Uint32 settingsCount = read( 8, 4 );
	size_t offset = 12;
	for ( Uint32 i = 0; i < settingsCount && count - offset >= 4; ++i ) {
		const auto entryType = bytes[offset];
		const auto nameLength = read( offset + 2, 2 );
		offset += 4;
		if ( count - offset < aligned( nameLength ) + 4 )
			break;
		const std::string_view name( reinterpret_cast<const char*>( bytes + offset ), nameLength );
		offset += aligned( nameLength ) + 4; // Name and last-change serial.
		size_t size = entryType == 2 ? 8 : 4;
		if ( entryType > 2 || count - offset < size )
			break;
		if ( entryType == 1 ) {
			const auto textLength = read( offset, 4 );
			offset += 4;
			if ( textLength > count - offset || aligned( textLength ) > count - offset )
				break;
			if ( name == "Gtk/FontName" )
				return std::string( reinterpret_cast<const char*>( bytes + offset ), textLength );
			size = aligned( textLength );
		}
		offset += size;
	}
	return {};
}
#endif

static Float parseFontSize( const std::string& value ) {
	char* end = nullptr;
	const Float size = std::strtof( value.c_str(), &end );
	if ( end == value.c_str() || ( *end && std::string_view( end ) != "px" ) )
		return 0.f;
	// Seed eepp's logical font size with the desktop's nominal size. The window
	// applies its own UI scale when it renders the font.
	return size;
}

static Float getDesktopUIFontSize() {
	auto config = Sys::getEnv( "XDG_CONFIG_HOME" );
	if ( config.empty() )
		config = Sys::getUserDirectory() + "/.config";
	FileSystem::dirAddSlashAtEnd( config );
	const auto desktop = String::split( Sys::getEnv( "XDG_CURRENT_DESKTOP" ), ':' );
	if ( std::find( desktop.begin(), desktop.end(), "KDE" ) != desktop.end() ) {
		IniFile kde( config + "kdeglobals" );
		const auto font = String::split( kde.getValue( "General", "font" ), ',' );
		const Float pixels = font.size() > 2 ? parseFontSize( font[2] ) : 0.f;
		const Float size = pixels > 0.f ? pixels : font.size() > 1 ? parseFontSize( font[1] ) : 0.f;
		if ( size > 0.f )
			return size;
	}
	std::string font;
#ifdef EE_HAS_X11_FONT_SETTINGS
	if ( !Sys::getEnv( "DISPLAY" ).empty() )
		font = getXSettingsFont();
#endif
	if ( font.empty() ) {
		IniFile gtk( config + "gtk-3.0/settings.ini" );
		font = gtk.getValue( "Settings", "gtk-font-name" );
	}
	String::trimInPlace( font );
	const auto separator = font.find_last_of( " 	" );
	return separator == std::string::npos ? 0.f : parseFontSize( font.substr( separator + 1 ) );
}
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
static Float getAndroidUIFontSize( Float baseline ) {
	auto* platform = Engine::instance()->getPlatformHelper();
	auto* env = platform ? static_cast<JNIEnv*>( platform->getJNIEnv() ) : nullptr;
	if ( !env )
		return 0.f;
	if ( env->PushLocalFrame( 16 ) < 0 ) {
		env->ExceptionClear();
		return 0.f;
	}
	const auto getObject = [env]( jobject object, const char* name, const char* signature ) {
		if ( !object || env->ExceptionCheck() )
			return jobject{};
		const auto method = env->GetMethodID( env->GetObjectClass( object ), name, signature );
		return method ? env->CallObjectMethod( object, method ) : jobject{};
	};
	const auto activity = static_cast<jobject>( platform->getActivity() );
	const auto resources =
		getObject( activity, "getResources", "()Landroid/content/res/Resources;" );
	const auto metrics =
		getObject( resources, "getDisplayMetrics", "()Landroid/util/DisplayMetrics;" );
	Float result = 0.f;
	if ( metrics && !env->ExceptionCheck() ) {
		const auto densityField = env->GetFieldID( env->GetObjectClass( metrics ), "density", "F" );
		const Float density = densityField ? env->GetFloatField( metrics, densityField ) : 0.f;
		const auto typedValue =
			density > 0.f ? env->FindClass( "android/util/TypedValue" ) : nullptr;
		const auto apply = typedValue
							   ? env->GetStaticMethodID( typedValue, "applyDimension",
														 "(IFLandroid/util/DisplayMetrics;)F" )
							   : nullptr;
		if ( apply ) {
			// COMPLEX_UNIT_SP converts using the user's font scaling curve on recent Android.
			result =
				env->CallStaticFloatMethod( typedValue, apply, 2, baseline, metrics ) / density;
		}
	}
	if ( env->ExceptionCheck() ) {
		env->ExceptionClear();
		result = 0.f;
	}
	env->PopLocalFrame( nullptr );
	return result;
}
#endif

Float PlatformHelper::getSystemUIFontSize( Float fallback ) {
	const char* useSystemSize = std::getenv( "EEPP_USE_SYSTEM_UI_FONT_SIZE" );
	if ( !useSystemSize || std::string_view( useSystemSize ) != "1" )
		return fallback;

	Float size = 0.f;
#if EE_PLATFORM == EE_PLATFORM_MACOS
	size = Private::getSystemUIFontSize();
#elif EE_PLATFORM == EE_PLATFORM_WIN
	using Query = BOOL( WINAPI* )( UINT, UINT, PVOID, UINT, UINT );
	const auto query = reinterpret_cast<Query>(
		GetProcAddress( GetModuleHandleW( L"user32.dll" ), "SystemParametersInfoForDpi" ) );
	NONCLIENTMETRICSW metrics{};
	metrics.cbSize = sizeof( metrics );
	if ( query && query( SPI_GETNONCLIENTMETRICS, sizeof( metrics ), &metrics, 0, 96 ) )
		size = std::abs( static_cast<Float>( metrics.lfMessageFont.lfHeight ) );
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	size = getDesktopUIFontSize();
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	const Float baseline = fallback > 0.f ? fallback : 12.f;
	size = getAndroidUIFontSize( baseline );
	if ( std::abs( size - baseline ) < 0.01f )
		return fallback;
#endif
	return std::isfinite( size ) && size >= 6.f && size <= 72.f ? size : fallback;
}

bool PlatformHelper::setApplicationReopenCallback( std::function<bool()> callback ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::ApplicationReopenHandler::setCallback( std::move( callback ) );
#else
	(void)callback;
	return false;
#endif
}

bool PlatformHelper::setNativeScrollMomentumEnabled( bool enabled ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::setNativeScrollMomentumEnabled( enabled );
#else
	(void)enabled;
	return false;
#endif
}

bool PlatformHelper::setWindowTitleBarSeparatorVisible( void* window, bool visible ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::setWindowTitleBarSeparatorVisible( window, visible );
#else
	(void)window;
	(void)visible;
	return false;
#endif
}

bool PlatformHelper::setWindowTitleBarColor( void* window, Uint8 red, Uint8 green, Uint8 blue ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::setWindowTitleBarColor( window, red, green, blue );
#else
	(void)window;
	(void)red;
	(void)green;
	(void)blue;
	return false;
#endif
}

}} // namespace EE::Window
