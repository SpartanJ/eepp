#include "physicaldisplaymetrics.hpp"
#include <cmath>
#include <eepp/system/sys.hpp>
#include <memory>

#if ( EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD ) && \
	__has_include( <X11/Xlib.h> ) && __has_include( <X11/extensions/Xrandr.h> )
#define EE_HAS_PHYSICAL_X11_METRICS
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include <shellscalingapi.h>
#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#endif

namespace EE { namespace Window { namespace Private {

using System::Sys;

Float PhysicalDisplayMetrics::getDPI( const Math::Rect& bounds ) {
	if ( bounds.getWidth() <= 0 || bounds.getHeight() <= 0 )
		return 0.f;
#if defined( EE_HAS_PHYSICAL_X11_METRICS )
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
	for ( int i = 0; i < screen->noutput; ++i ) {
		std::unique_ptr<XRROutputInfo, decltype( freeOutput )> output(
			outputInfo( connection.get(), screen.get(), screen->outputs[i] ), freeOutput );
		if ( !output || output->connection != RR_Connected || !output->crtc ||
			 output->mm_width == 0 || output->mm_height == 0 )
			continue;
		std::unique_ptr<XRRCrtcInfo, decltype( freeCrtc )> crtc(
			crtcInfo( connection.get(), screen.get(), output->crtc ), freeCrtc );
		if ( !crtc || x < crtc->x || y < crtc->y ||
			 x >= crtc->x + static_cast<int>( crtc->width ) ||
			 y >= crtc->y + static_cast<int>( crtc->height ) )
			continue;
		return 25.4f *
			   std::hypot( static_cast<Float>( crtc->width ), static_cast<Float>( crtc->height ) ) /
			   std::hypot( static_cast<Float>( output->mm_width ),
						   static_cast<Float>( output->mm_height ) );
	}
	return 0.f;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	RECT rect{ bounds.Left, bounds.Top, bounds.Right, bounds.Bottom };
	const HMONITOR monitor = MonitorFromRect( &rect, MONITOR_DEFAULTTONULL );
	if ( !monitor )
		return 0.f;
	std::unique_ptr<void, decltype( &Sys::unloadObject )> library(
		Sys::loadObject( "shcore.dll", false ), Sys::unloadObject );
	if ( !library )
		return 0.f;
	const auto getDpi = reinterpret_cast<decltype( &::GetDpiForMonitor )>(
		Sys::loadFunction( library.get(), "GetDpiForMonitor", false ) );
	UINT x = 0, y = 0;
	return getDpi && SUCCEEDED( getDpi( monitor, MDT_RAW_DPI, &x, &y ) ) && x && y
			   ? std::sqrt( static_cast<Float>( x ) * y )
			   : 0.f;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return getPhysicalDisplayDPI( bounds );
#else
	(void)bounds;
	return 0.f;
#endif
}

}}} // namespace EE::Window::Private
