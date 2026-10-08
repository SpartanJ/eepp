#include "process_icon_resolver_macos.hpp"

#include <CoreFoundation/CoreFoundation.h>

namespace eproc {

const std::string& ProcessIconResolverMacOS::resolve( const std::string& executablePath ) {
	const size_t bundleEnd = executablePath.find( ".app/Contents/MacOS/" );
	if ( bundleEnd == std::string::npos ) {
		static const std::string empty;
		return empty;
	}
	const std::string bundlePath = executablePath.substr( 0, bundleEnd + 4 );
	auto found = mIcons.find( bundlePath );
	if ( found != mIcons.end() )
		return found->second;

	std::string iconPath;
	CFURLRef bundleURL = CFURLCreateFromFileSystemRepresentation(
		kCFAllocatorDefault, reinterpret_cast<const UInt8*>( bundlePath.data() ),
		static_cast<CFIndex>( bundlePath.size() ), true );
	if ( bundleURL ) {
		CFBundleRef bundle = CFBundleCreate( kCFAllocatorDefault, bundleURL );
		if ( bundle ) {
			CFTypeRef iconName =
				CFBundleGetValueForInfoDictionaryKey( bundle, CFSTR( "CFBundleIconFile" ) );
			if ( iconName && CFGetTypeID( iconName ) == CFStringGetTypeID() ) {
				CFURLRef iconURL = CFBundleCopyResourceURL(
					bundle, static_cast<CFStringRef>( iconName ), nullptr, nullptr );
				if ( !iconURL )
					iconURL = CFBundleCopyResourceURL( bundle, static_cast<CFStringRef>( iconName ),
													   CFSTR( "icns" ), nullptr );
				if ( iconURL ) {
					char path[4096]{};
					if ( CFURLGetFileSystemRepresentation(
							 iconURL, true, reinterpret_cast<UInt8*>( path ), sizeof( path ) ) )
						iconPath = path;
					CFRelease( iconURL );
				}
			}
			CFRelease( bundle );
		}
		CFRelease( bundleURL );
	}
	return mIcons.emplace( bundlePath, std::move( iconPath ) ).first->second;
}

} // namespace eproc
