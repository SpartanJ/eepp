#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS

#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

extern "C" CFStringRef eeppFileAssociationTypeIdentifierForExtension( const char* extension ) {
	if ( extension == nullptr )
		return nullptr;

	NSString* extensionString = [NSString stringWithUTF8String:extension];
	if ( extensionString == nil )
		return nullptr;

	if ( @available( macOS 11.0, * ) ) {
		UTType* type = [UTType typeWithFilenameExtension:extensionString];
		if ( type == nil || type.identifier == nil )
			return nullptr;
		return CFStringCreateWithCString( kCFAllocatorDefault, type.identifier.UTF8String,
										  kCFStringEncodingUTF8 );
	}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	CFStringRef type = UTTypeCreatePreferredIdentifierForTag(
		kUTTagClassFilenameExtension, static_cast<CFStringRef>( extensionString ), nullptr );
#pragma clang diagnostic pop
	return type;
}

#endif
