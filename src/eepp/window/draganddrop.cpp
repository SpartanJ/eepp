#include <eepp/window/draganddrop.hpp>

#include "draganddrop/dragaccess.hpp"
#include "draganddrop/dragmanager.hpp"
#include <eepp/network/uri.hpp>
#include <eepp/window/window.hpp>

#include <algorithm>
#include <utf8cpp/utf8.h>

namespace EE { namespace Window {

namespace {
bool isUriPathByteUnreserved( unsigned char c, [[maybe_unused]] size_t index ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( c == '\\' || ( c == ':' && index == 1 ) )
		return true;
#endif
	return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ||
		   c == '-' || c == '_' || c == '.' || c == '~' || c == '/';
}

bool validateOffer( const DragOffer& offer, size_t& capacity, std::string* error,
					bool calculateSize ) {
	if ( error )
		error->clear();
	auto fail = [&]( const char* message ) {
		if ( error )
			*error = message;
		return false;
	};
	if ( offer.files.empty() )
		return fail( "A file drag must contain at least one file" );
	if ( offer.actions != static_cast<DragActions>( DragAction::Copy ) )
		return fail( "Only the Copy action is currently supported" );
	capacity = 0;
	const size_t maximum = std::string{}.max_size();
	for ( const auto& file : offer.files ) {
		const auto& path = file.path;
		if ( path.empty() || path.find( '\0' ) != std::string::npos ||
			 !utf8::is_valid( path.begin(), path.end() ) )
			return fail( "File paths must be nonempty UTF-8 without NUL bytes" );
#if EE_PLATFORM == EE_PLATFORM_WIN
		const bool drive =
			path.size() >= 3 &&
			( ( path[0] >= 'A' && path[0] <= 'Z' ) || ( path[0] >= 'a' && path[0] <= 'z' ) ) &&
			path[1] == ':' && ( path[2] == '/' || path[2] == '\\' );
		const bool unc = path.size() > 2 && ( ( path[0] == '\\' && path[1] == '\\' ) ||
											  ( path[0] == '/' && path[1] == '/' ) );
		if ( !drive && !unc )
			return fail( "File paths must be absolute" );
		const auto shareSeparator = unc ? path.find_first_of( "/\\", 2 ) : std::string::npos;
		if ( unc && ( shareSeparator == 2 || shareSeparator == path.size() - 1 || path[2] == '?' ||
					  path[2] == '.' || shareSeparator == std::string::npos ||
					  path[shareSeparator + 1] == '/' || path[shareSeparator + 1] == '\\' ) )
			return fail(
				"UNC paths must contain a server and share; device paths are unsupported" );
#else
		if ( path[0] != '/' )
			return fail( "File paths must be absolute" );
#endif
		if ( !calculateSize )
			continue;
		size_t start = 0;
		size_t prefix = 9; // file:// plus CRLF
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( drive )
			++prefix;
		else
			start = 2;
#endif
		if ( capacity > maximum - prefix )
			return fail( "File offer is too large" );
		capacity += prefix;
		for ( size_t i = start; i < path.size(); ++i ) {
			const size_t bytes = isUriPathByteUnreserved( path[i], i ) ? 1 : 3;
			if ( capacity > maximum - bytes )
				return fail( "File offer is too large" );
			capacity += bytes;
		}
	}
	return true;
}

} // namespace

DragAndDrop::DragAndDrop( Window& window ) : mWindow( &window ) {}

DragAndDrop DragAndDrop::forWindow( Window& window ) {
	return DragAndDrop( window );
}

DragCapabilities DragAndDrop::getCapabilities() const {
	auto context = Private::NativeDragAccess::getNativeDragContext( *mWindow );
	const bool source = Private::DragManager::isFileDragSupported( context );
	return { source, false, Private::NativeDragAccess::isNativeFileTargetSupported( *mWindow ),
			 false, source };
}

DragStartResult DragAndDrop::startDrag( const DragOffer& offer,
										DragCompletionCallback completion ) {
	if ( Private::DragManager::hasSession() )
		return { "Another native drag is active or awaiting completion", DragStartStatus::Busy };
	auto context = Private::NativeDragAccess::getNativeDragContext( *mWindow );
	if ( !Private::DragManager::isFileDragSupported( context ) )
		return { "Native file dragging is unavailable for this video backend",
				 DragStartStatus::Unsupported };
	std::string payload;
	std::string error;
	if ( offer.preview.hotspot.x < -2048 || offer.preview.hotspot.x > 2048 ||
		 offer.preview.hotspot.y < -2048 || offer.preview.hotspot.y > 2048 ) {
		return { "Drag preview hotspots must be within 2048 pixels of the image origin",
				 DragStartStatus::InvalidOffer };
	}
	if ( const auto* image = offer.preview.image ) {
		const auto width = image->getWidth(), height = image->getHeight();
		if ( !image->getPixels() || !width || !height || width > 2048 || height > 2048 ||
			 static_cast<Uint64>( width ) * height > 1048576 ||
			 ( image->getChannels() != 3 && image->getChannels() != 4 ) ||
			 image->getMemSize() < static_cast<Uint64>( width ) * height * image->getChannels() ) {
			return { "Drag previews require an RGB/RGBA image of at most 1,048,576 pixels",
					 DragStartStatus::InvalidOffer };
		}
	}
	if ( context.kind == NativeWindowContext::Kind::Win32 ) {
		size_t unused;
		if ( !validateOffer( offer, unused, &error, false ) )
			return { std::move( error ), DragStartStatus::InvalidOffer };
	} else if ( !encodeFileUriList( offer, payload, &error ) ) {
		return { std::move( error ), DragStartStatus::InvalidOffer };
	}
	return Private::DragManager::startFileDrag( context, offer, std::move( payload ),
												std::move( completion ) );
}

void DragAndDrop::cancelDrag() {
	Private::DragManager::cancelDrag( *mWindow );
}

bool DragAndDrop::isDragging() const {
	return Private::DragManager::isDragging( *mWindow );
}

bool DragAndDrop::encodeFileUriList( const DragOffer& offer, std::string& output,
									 std::string* error ) {
	output.clear();
	size_t capacity;
	if ( !validateOffer( offer, capacity, error, true ) )
		return false;
	// The validation pass reserves a conservative bound for URI escaping, so encoding can
	// append directly without temporary URI objects or per-file output strings.
	output.reserve( capacity );
	const std::string reserved = "?#[]";
#if EE_PLATFORM == EE_PLATFORM_WIN
	// UNC server names occupy the authority, where ':' and '@' have structural meaning.
	const std::string reservedAuthority = "?#[]:@";
	std::string normalizedPath;
#endif
	for ( const auto& file : offer.files ) {
		output += "file://";
#if EE_PLATFORM == EE_PLATFORM_WIN
		const auto& path = file.path;
		const bool drive = path[1] == ':';
		const auto* uriPath = &path;
		if ( !drive || path.find( '\\' ) != std::string::npos ) {
			normalizedPath.assign( path, drive ? 0 : 2, std::string::npos );
			std::replace( normalizedPath.begin(), normalizedPath.end(), '\\', '/' );
			uriPath = &normalizedPath;
		}
		if ( drive )
			output += '/';
		Network::URI::encode( *uriPath, drive ? reserved : reservedAuthority, output );
#else
		Network::URI::encode( file.path, reserved, output );
#endif
		output += "\r\n";
	}
	return true;
}

}} // namespace EE::Window
