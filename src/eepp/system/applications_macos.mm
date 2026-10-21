#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS

#include <eepp/system/applications.hpp>

#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <algorithm>

namespace EE { namespace System {

namespace {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
/** Older macOS uses the same Launch Services registrations through its C API. */
NSArray<NSURL*>* legacyApplications( NSURL* file, NSString* extension, NSString* mime ) {
	NSMutableArray<NSURL*>* urls = [NSMutableArray array];
	if ( file ) {
		CFURLRef preferred =
			LSCopyDefaultApplicationURLForURL( (CFURLRef)file, kLSRolesAll, nullptr );
		if ( preferred ) {
			[urls addObject:(NSURL*)preferred];
			CFRelease( preferred );
		}
		CFArrayRef handlers = LSCopyApplicationURLsForURL( (CFURLRef)file, kLSRolesAll );
		if ( handlers ) {
			[urls addObjectsFromArray:(NSArray*)handlers];
			CFRelease( handlers );
		}
		if ( urls.count )
			return urls;
	}
	CFStringRef type =
		extension.length ? UTTypeCreatePreferredIdentifierForTag( kUTTagClassFilenameExtension,
																  (CFStringRef)extension, nullptr )
						 : nullptr;
	if ( !type && mime.length )
		type = UTTypeCreatePreferredIdentifierForTag( kUTTagClassMIMEType, (CFStringRef)mime,
													  nullptr );
	if ( !type )
		return urls;
	NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
	CFStringRef preferred = LSCopyDefaultRoleHandlerForContentType( type, kLSRolesAll );
	if ( preferred ) {
		NSURL* url = [workspace URLForApplicationWithBundleIdentifier:(NSString*)preferred];
		if ( url )
			[urls addObject:url];
		CFRelease( preferred );
	}
	CFArrayRef handlers = LSCopyAllRoleHandlersForContentType( type, kLSRolesAll );
	if ( handlers ) {
		for ( NSString* identifier in (NSArray*)handlers ) {
			NSURL* url = [workspace URLForApplicationWithBundleIdentifier:identifier];
			if ( url )
				[urls addObject:url];
		}
		CFRelease( handlers );
	}
	CFRelease( type );
	return urls;
}
#pragma clang diagnostic pop

} // namespace

std::vector<Application> Applications::forFile( const URI& uri,
												const std::vector<std::string>& mimeTypes ) {
	@autoreleasepool {
		NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
		NSString* path = [NSString stringWithUTF8String:uri.getPath().c_str()];
		if ( !path )
			return {};
		NSURL* file = nil;
		if ( uri.getScheme() == "file" )
			file = [NSURL fileURLWithPath:[NSString stringWithUTF8String:uri.getFSPath().c_str()]];
		NSString* mimeType =
			mimeTypes.empty() ? nil : [NSString stringWithUTF8String:mimeTypes.front().c_str()];
		NSArray<NSURL*>* handlers;
		NSURL* preferred = nil;
		if ( @available( macOS 12.0, * ) ) {
			handlers = file ? [workspace URLsForApplicationsToOpenURL:file] : @[];
			preferred = file ? [workspace URLForApplicationToOpenURL:file] : nil;
			// Packed and remote files do not exist locally yet. Resolve their filename's UTI
			// without downloading them or treating sftp/archive as an application URL scheme.
			if ( !handlers.count ) {
				UTType* type = path.pathExtension.length
								   ? [UTType typeWithFilenameExtension:path.pathExtension]
								   : nil;
				if ( !type && mimeType.length )
					type = [UTType typeWithMIMEType:mimeType];
				if ( type ) {
					handlers = [workspace URLsForApplicationsToOpenContentType:type];
					if ( !preferred )
						preferred = [workspace URLForApplicationToOpenContentType:type];
				}
			}
		} else {
			handlers = legacyApplications( file, path.pathExtension, mimeType );
		}
		std::vector<Application> out;
		out.reserve( handlers.count + ( preferred ? 1 : 0 ) );
		auto add = [&out]( NSURL* url ) {
			if ( !url )
				return;
			NSBundle* bundle = [NSBundle bundleWithURL:url];
			NSString* identifier = bundle.bundleIdentifier;
			std::string id( ( identifier.length ? identifier : url.path ).UTF8String );
			if ( std::any_of( out.begin(), out.end(),
							  [&id]( const Application& app ) { return app.id == id; } ) )
				return;
			NSString* name = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
			if ( !name.length )
				name = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
			if ( !name.length )
				name = url.lastPathComponent.stringByDeletingPathExtension;
			out.push_back( { std::move( id ), name.UTF8String, url.path.UTF8String, {}, false } );
		};
		add( preferred );
		for ( NSURL* url in handlers )
			add( url );
		return out;
	}
}

}} // namespace EE::System

#endif
