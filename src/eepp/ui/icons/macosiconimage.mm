#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {

NSString* homePath( NSString* component ) {
	return component ? [NSHomeDirectory() stringByAppendingPathComponent:component]
					 : NSHomeDirectory();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

/** Icon for a file type identifier (UTI) or extension. */
NSImage* iconForType( NSString* type ) {
	return [[NSWorkspace sharedWorkspace] iconForFileType:type];
}

#pragma clang diagnostic pop

/** Icon for a freedesktop icon name, or nil. */
NSImage* iconForName( const char* name ) {
	NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
	struct Folder {
		const char* name;
		NSString* component;
	};
	const Folder folders[] = {
		{ "user-home", nil },
		{ "user-desktop", @"Desktop" },
		{ "folder-documents", @"Documents" },
		{ "folder-download", @"Downloads" },
		{ "folder-music", @"Music" },
		{ "folder-pictures", @"Pictures" },
		{ "folder-videos", @"Movies" },
	};
	for ( const auto& folder : folders ) {
		if ( std::strcmp( name, folder.name ) == 0 )
			return [workspace iconForFile:homePath( folder.component )];
	}
	if ( std::strcmp( name, "folder" ) == 0 )
		return [NSImage imageNamed:NSImageNameFolder];
	if ( std::strcmp( name, "user-trash" ) == 0 )
		return [NSImage imageNamed:NSImageNameTrashEmpty];
	if ( std::strcmp( name, "user-trash-full" ) == 0 )
		return [NSImage imageNamed:NSImageNameTrashFull];
	if ( std::strcmp( name, "drive-harddisk" ) == 0 ||
		 std::strcmp( name, "drive-harddisk-root" ) == 0 )
		return [workspace iconForFile:@"/"];
	if ( std::strcmp( name, "network-server" ) == 0 || std::strcmp( name, "folder-remote" ) == 0 ||
		 std::strcmp( name, "network-workgroup" ) == 0 )
		return [NSImage imageNamed:NSImageNameNetwork];
	if ( std::strcmp( name, "text-x-generic" ) == 0 )
		return iconForType( @"public.plain-text" );
	if ( std::strcmp( name, "application-x-executable" ) == 0 )
		return iconForType( @"public.unix-executable" );
	return nil;
}

/** Icon for a file name's type (by extension). */
NSImage* iconForFileName( const char* fileName, bool isDirectory ) {
	if ( isDirectory )
		return [NSImage imageNamed:NSImageNameFolder];
	NSString* extension = [[NSString stringWithUTF8String:fileName ? fileName : ""] pathExtension];
	if ( @available( macOS 11.0, * ) ) {
		UTType* type = extension.length ? [UTType typeWithFilenameExtension:extension] : nil;
		return [[NSWorkspace sharedWorkspace] iconForContentType:type ? type : UTTypeData];
	}
	return iconForType( extension.length ? extension : @"public.data" );
}

} // namespace

extern "C" unsigned char* eeppSystemIconRGBA( int kind, const char* value, int isDirectory,
											  const char* const* names, int nameCount, int size ) {
	if ( size <= 0 )
		return nullptr;
	@autoreleasepool {
		NSImage* image = nil;
		if ( kind == 2 && value && *value ) {
			image =
				[[NSWorkspace sharedWorkspace] iconForFile:[NSString stringWithUTF8String:value]];
		} else if ( kind == 1 ) {
			image = iconForFileName( value, isDirectory != 0 );
		}
		for ( int i = 0; !image && i < nameCount; ++i ) {
			if ( names[i] )
				image = iconForName( names[i] );
		}
		if ( !image )
			return nullptr;

		NSBitmapImageRep* bitmap =
			[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr
													pixelsWide:size
													pixelsHigh:size
												 bitsPerSample:8
											   samplesPerPixel:4
													  hasAlpha:YES
													  isPlanar:NO
												colorSpaceName:NSDeviceRGBColorSpace
												   bytesPerRow:size * 4
												  bitsPerPixel:32];
		if ( !bitmap )
			return nullptr;
		NSGraphicsContext* context = [NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap];
		[NSGraphicsContext saveGraphicsState];
		[NSGraphicsContext setCurrentContext:context];
		context.imageInterpolation = NSImageInterpolationHigh;
		// The icon picks its best representation for the target size.
		[image drawInRect:NSMakeRect( 0, 0, size, size )
				 fromRect:NSZeroRect
				operation:NSCompositingOperationCopy
				 fraction:1.0];
		[context flushGraphics];
		[NSGraphicsContext restoreGraphicsState];

		unsigned char* pixels = static_cast<unsigned char*>( std::malloc( size * size * 4 ) );
		const unsigned char* source = [bitmap bitmapData];
		NSInteger stride = [bitmap bytesPerRow];
		if ( pixels && source ) {
			for ( int y = 0; y < size; ++y )
				std::memcpy( pixels + y * size * 4, source + y * stride, size * 4 );
			// AppKit bitmaps are premultiplied.
			for ( int i = 0; i < size * size * 4; i += 4 ) {
				unsigned alpha = pixels[i + 3];
				if ( alpha > 0 && alpha < 255 ) {
					for ( int c = 0; c < 3; ++c ) {
						pixels[i + c] = static_cast<unsigned char>(
							std::min( 255u, ( pixels[i + c] * 255u + alpha / 2 ) / alpha ) );
					}
				}
			}
		} else if ( pixels ) {
			std::free( pixels );
			pixels = nullptr;
		}
		[bitmap release];
		return pixels;
	}
}

#endif
