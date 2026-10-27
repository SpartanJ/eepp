#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS

#include "devicemonitor.hpp"
#include "places.hpp"

#import <AppKit/AppKit.h>

namespace eefm {

DeviceMonitor::DeviceMonitor() : mChanged( std::make_shared<std::atomic<bool>>( false ) ) {
	@autoreleasepool {
		NSNotificationCenter* center = [[NSWorkspace sharedWorkspace] notificationCenter];
		NSNotificationName names[] = { NSWorkspaceDidMountNotification,
									   NSWorkspaceDidUnmountNotification,
									   NSWorkspaceDidRenameVolumeNotification };
		auto changed = mChanged;
		for ( size_t i = 0; i < 3; ++i ) {
			mObservers[i] = [[center addObserverForName:names[i]
												 object:nil
												  queue:nil
											 usingBlock:^( NSNotification* ) {
											   changed->store( true, std::memory_order_relaxed );
											 }] retain];
		}
	}
}

DeviceMonitor::~DeviceMonitor() {
	stop();
}

bool DeviceMonitor::changed() {
	return mChanged->exchange( false, std::memory_order_relaxed );
}

void DeviceMonitor::stop() {
	@autoreleasepool {
		NSNotificationCenter* center = [[NSWorkspace sharedWorkspace] notificationCenter];
		for ( auto& observer : mObservers ) {
			if ( observer ) {
				[center removeObserver:(id)observer];
				[(id)observer release];
				observer = nullptr;
			}
		}
	}
}

void DeviceMonitor::updateCapabilities( std::vector<Place>& devices,
										const CancellationToken& token ) {
	for ( auto& device : devices ) {
		if ( token.isCancellationRequested() )
			return;
		if ( device.section == PlaceSection::Devices && device.uri.getPath() != "/" )
			device.ejectable = canEject( device.uri.getFSPath() );
	}
}

bool DeviceMonitor::canEject( const std::string& path ) {
	@autoreleasepool {
		NSString* nativePath = [NSString stringWithUTF8String:path.c_str()];
		if ( !nativePath )
			return false;
		NSURL* url = [NSURL fileURLWithPath:nativePath isDirectory:YES];
		NSNumber* ejectable = nil;
		return [url getResourceValue:&ejectable forKey:NSURLVolumeIsEjectableKey error:nil] &&
			   ejectable.boolValue;
	}
}

std::string DeviceMonitor::eject( const std::string& path, const CancellationToken& token ) {
	if ( token.isCancellationRequested() )
		return "Device operation cancelled.";
	@autoreleasepool {
		if ( !canEject( path ) )
			return "This volume is no longer available or cannot be ejected.";
		NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]
								isDirectory:YES];
		NSError* error = nil;
		if ( [[NSWorkspace sharedWorkspace] unmountAndEjectDeviceAtURL:url error:&error] )
			return {};
		const char* description = error.localizedDescription.UTF8String;
		return description ? description : "Unable to eject the volume.";
	}
}

} // namespace eefm

#endif
