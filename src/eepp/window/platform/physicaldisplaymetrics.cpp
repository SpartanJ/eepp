#include "physicaldisplaymetrics.hpp"
#include <cmath>
#include <eepp/core/string.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/sys.hpp>
#include <memory>

#if ( EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD ) && \
	__has_include( <X11/Xlib.h> ) && __has_include( <X11/extensions/Xrandr.h> )
#define EE_HAS_PHYSICAL_X11_METRICS
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#undef None
#undef Status
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include <shellscalingapi.h>
#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#endif

#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window { namespace Private {

using namespace EE::System;

#if defined( EE_HAS_PHYSICAL_X11_METRICS )
static std::string getX11MonitorId( void* x11, void* randr, ::Display* connection, RROutput id,
									const XRROutputInfo* output ) {
	if ( !output )
		return {};
	const auto atom = reinterpret_cast<decltype( &::XInternAtom )>(
		Sys::loadFunction( x11, "XInternAtom", false ) );
	const auto free =
		reinterpret_cast<decltype( &::XFree )>( Sys::loadFunction( x11, "XFree", false ) );
	const auto property = reinterpret_cast<decltype( &::XRRGetOutputProperty )>(
		Sys::loadFunction( randr, "XRRGetOutputProperty", false ) );
	if ( !atom || !free || !property )
		return {};
	const Atom edid = atom( connection, "EDID", True );
	if ( !edid )
		return {};
	Atom type{};
	int format = 0;
	unsigned long count = 0, remaining = 0;
	unsigned char* bytes = nullptr;
	const int status = property( connection, id, edid, 0, 32, False, False, XA_INTEGER, &type,
								 &format, &count, &remaining, &bytes );
	std::unique_ptr<unsigned char, decltype( free )> data( bytes, free );
	if ( status != Success || type != XA_INTEGER || format != 8 || count != 128 || !data )
		return {};
	// Include the connector to distinguish displays with identical EDID blocks.
	std::string identity( output->name, output->nameLen );
	identity.append( reinterpret_cast<const char*>( data.get() ), count );
	return "x11-" + MD5::fromString( identity ).toHexString();
}

static Float getX11DisplayMetrics( const Math::Rect& bounds, std::string* identity ) {
	// Keep the optional X11 dependencies local to this rare query. Never borrow or close SDL's
	// connection, and do not introduce a link-time dependency on Xlib/XRandR.
	using Library = std::unique_ptr<void, decltype( &Sys::unloadObject )>;
	Library x11( Sys::loadObject( "libX11.so.6", false ), Sys::unloadObject );
	Library randr( Sys::loadObject( "libXrandr.so.2", false ), Sys::unloadObject );
	if ( !x11 || !randr )
		return 0.f;
	const auto open = reinterpret_cast<decltype( &::XOpenDisplay )>(
		Sys::loadFunction( x11.get(), "XOpenDisplay", false ) );
	const auto close = reinterpret_cast<decltype( &::XCloseDisplay )>(
		Sys::loadFunction( x11.get(), "XCloseDisplay", false ) );
	const auto root = reinterpret_cast<decltype( &::XDefaultRootWindow )>(
		Sys::loadFunction( x11.get(), "XDefaultRootWindow", false ) );
	const auto resources = reinterpret_cast<decltype( &::XRRGetScreenResourcesCurrent )>(
		Sys::loadFunction( randr.get(), "XRRGetScreenResourcesCurrent", false ) );
	const auto freeResources = reinterpret_cast<decltype( &::XRRFreeScreenResources )>(
		Sys::loadFunction( randr.get(), "XRRFreeScreenResources", false ) );
	const auto outputInfo = reinterpret_cast<decltype( &::XRRGetOutputInfo )>(
		Sys::loadFunction( randr.get(), "XRRGetOutputInfo", false ) );
	const auto freeOutput = reinterpret_cast<decltype( &::XRRFreeOutputInfo )>(
		Sys::loadFunction( randr.get(), "XRRFreeOutputInfo", false ) );
	const auto crtcInfo = reinterpret_cast<decltype( &::XRRGetCrtcInfo )>(
		Sys::loadFunction( randr.get(), "XRRGetCrtcInfo", false ) );
	const auto freeCrtc = reinterpret_cast<decltype( &::XRRFreeCrtcInfo )>(
		Sys::loadFunction( randr.get(), "XRRFreeCrtcInfo", false ) );
	if ( !open || !close || !root || !resources || !freeResources || !outputInfo || !freeOutput ||
		 !crtcInfo || !freeCrtc )
		return 0.f;
	std::unique_ptr<::Display, decltype( close )> connection( open( nullptr ), close );
	if ( !connection )
		return 0.f;
	std::unique_ptr<XRRScreenResources, decltype( freeResources )> screen(
		resources( connection.get(), root( connection.get() ) ), freeResources );
	if ( !screen )
		return 0.f;
	const int x = bounds.Left + bounds.getWidth() / 2;
	const int y = bounds.Top + bounds.getHeight() / 2;
	bool foundOutput = false;
	for ( int i = 0; i < screen->noutput; ++i ) {
		std::unique_ptr<XRROutputInfo, decltype( freeOutput )> output(
			outputInfo( connection.get(), screen.get(), screen->outputs[i] ), freeOutput );
		if ( !output || output->connection != RR_Connected || !output->crtc )
			continue;
		if ( !identity && ( output->mm_width == 0 || output->mm_height == 0 ) )
			continue;
		std::unique_ptr<XRRCrtcInfo, decltype( freeCrtc )> crtc(
			crtcInfo( connection.get(), screen.get(), output->crtc ), freeCrtc );
		if ( !crtc || x < crtc->x || y < crtc->y ||
			 x >= crtc->x + static_cast<int>( crtc->width ) ||
			 y >= crtc->y + static_cast<int>( crtc->height ) )
			continue;
		if ( identity ) {
			if ( foundOutput ) {
				// Mirrored outputs can cover the same rectangle. The display bounds are
				// insufficient to select the output that owns the saved preference.
				identity->clear();
				break;
			}
			foundOutput = true;
			*identity = getX11MonitorId( x11.get(), randr.get(), connection.get(),
										 screen->outputs[i], output.get() );
			continue;
		}
		return 25.4f *
			   std::hypot( static_cast<Float>( crtc->width ), static_cast<Float>( crtc->height ) ) /
			   std::hypot( static_cast<Float>( output->mm_width ),
						   static_cast<Float>( output->mm_height ) );
	}
	return 0.f;
}
#elif EE_PLATFORM == EE_PLATFORM_WIN
static HMONITOR getMonitor( const Math::Rect& bounds ) {
	RECT rect{ bounds.Left, bounds.Top, bounds.Right, bounds.Bottom };
	return MonitorFromRect( &rect, MONITOR_DEFAULTTONULL );
}

static bool getMonitorDPI( const Math::Rect& bounds, MONITOR_DPI_TYPE type, UINT& x, UINT& y ) {
	const HMONITOR monitor = getMonitor( bounds );
	if ( !monitor )
		return false;
	std::unique_ptr<void, decltype( &Sys::unloadObject )> library(
		Sys::loadObject( "shcore.dll", false ), Sys::unloadObject );
	if ( !library )
		return false;
	// The SDK declaration is gated on targeting Windows 8.1 or later.
	using GetDpiForMonitorFn = HRESULT( WINAPI* )( HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT* );
	const auto getDpi = reinterpret_cast<GetDpiForMonitorFn>(
		Sys::loadFunction( library.get(), "GetDpiForMonitor", false ) );
	return getDpi && SUCCEEDED( getDpi( monitor, type, &x, &y ) );
}
#endif

Float PhysicalDisplayMetrics::getDPI( const Math::Rect& bounds ) {
	if ( bounds.getWidth() <= 0 || bounds.getHeight() <= 0 )
		return 0.f;
#if defined( EE_HAS_PHYSICAL_X11_METRICS )
	return getX11DisplayMetrics( bounds, nullptr );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	UINT x = 0, y = 0;
	return getMonitorDPI( bounds, MDT_RAW_DPI, x, y ) && x && y
			   ? std::sqrt( static_cast<Float>( x ) * y )
			   : 0.f;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return getPhysicalDisplayDPI( bounds );
#else
	(void)bounds;
	return 0.f;
#endif
}

#if EE_PLATFORM == EE_PLATFORM_WIN
Float PhysicalDisplayMetrics::getScale( const Math::Rect& bounds ) {
	UINT x = 0, y = 0;
	return getMonitorDPI( bounds, MDT_EFFECTIVE_DPI, x, y ) ? x / 96.f : 0.f;
}

std::string PhysicalDisplayMetrics::getPersistentId( eeWindowHandle window ) {
	return getPersistentMonitorId( window ? MonitorFromWindow( window, MONITOR_DEFAULTTONULL )
										  : nullptr );
}

std::string PhysicalDisplayMetrics::getPersistentMonitorId( void* handle ) {
	const auto monitor = static_cast<HMONITOR>( handle );
	MONITORINFOEXW info{};
	info.cbSize = sizeof( info );
	if ( !monitor || !GetMonitorInfoW( monitor, reinterpret_cast<MONITORINFO*>( &info ) ) )
		return {};
	DISPLAY_DEVICEW device{};
	device.cb = sizeof( device );
	if ( !EnumDisplayDevicesW( info.szDevice, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME ) ||
		 !device.DeviceID[0] )
		return {};
	return "win-" + MD5::fromString( String::fromWide( device.DeviceID ).toUtf8() ).toHexString();
}
#endif

std::string PhysicalDisplayMetrics::getPersistentId( const Math::Rect& bounds ) {
	if ( bounds.getWidth() <= 0 || bounds.getHeight() <= 0 )
		return {};
#if defined( EE_HAS_PHYSICAL_X11_METRICS )
	std::string identity;
	getX11DisplayMetrics( bounds, &identity );
	return identity;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	// SDL2 keeps the desktop origin in native pixels even with DPI-scaled dimensions.
	return getPersistentMonitorId(
		MonitorFromPoint( { bounds.Left, bounds.Top }, MONITOR_DEFAULTTONULL ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return getPersistentDisplayId( bounds );
#else
	return {};
#endif
}

std::string PhysicalDisplayMetrics::getWaylandPersistentId( const Display* display,
															DisplayManager* manager ) {
	if ( !display || !manager || display->getIndex() < 0 )
		return {};

	const auto name = String::trim( display->getName(), " \t\r\n" );
	// SDL generates numeric names from display indices or instance IDs when no label is given.
	if ( name.empty() || String::isNumber( name ) || String::iequals( name, "unknown" ) )
		return {};

	const int count = manager->getDisplayCount();
	for ( int index = 0; index < count; ++index ) {
		auto* other = manager->getDisplayIndex( index );
		if ( other && other != display && String::trim( other->getName(), " \t\r\n" ) == name )
			return {};
	}
	return "wayland-" + MD5::fromString( name ).toHexString();
}

}}} // namespace EE::Window::Private
