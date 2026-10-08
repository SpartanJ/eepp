#include <eepp/system/log.hpp>
#include <eepp/window/backend/SDL3/clipboardsdl3.hpp>
#include <eepp/window/backend/SDL3/windowsdl3.hpp>

#include <memory>

#ifdef EE_BACKEND_SDL3

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

namespace {

struct ClipboardDataContext {
	Clipboard::DataCallback callback;
	Clipboard::CleanupCallback cleanup;
};

const void* SDLCALL clipboardDataCallback( void* userdata, const char* mimeType, size_t* size ) {
	auto context = static_cast<ClipboardDataContext*>( userdata );
	return context->callback( mimeType, size );
}

void SDLCALL clipboardCleanupCallback( void* userdata ) {
	std::unique_ptr<ClipboardDataContext> context( static_cast<ClipboardDataContext*>( userdata ) );
	if ( context->cleanup )
		context->cleanup();
}

} // namespace

ClipboardSDL::ClipboardSDL( EE::Window::Window* window ) : Clipboard( window ) {}

ClipboardSDL::~ClipboardSDL() {}

void ClipboardSDL::init() {}

void ClipboardSDL::setText( const std::string& text ) {
	SDL_SetClipboardText( text.c_str() );
}

std::string ClipboardSDL::getText() {
	char* text = SDL_GetClipboardText();
	std::string str( text ? text : "" );
	SDL_free( text );
	return str;
}

bool ClipboardSDL::hasPrimarySelection() const {
	// SDL3 may not have primary selection support
	return false;
}

std::string ClipboardSDL::getPrimarySelectionText() {
	return getText();
}

void ClipboardSDL::setPrimarySelectionText( const std::string& text ) {
	// No-op
}

String ClipboardSDL::getWideText() {
	return String::fromUtf8( getText() );
}

bool ClipboardSDL::setData( DataCallback callback, CleanupCallback cleanup,
							const std::vector<std::string>& mimeTypes ) {
	if ( !callback || mimeTypes.empty() )
		return false;

	std::vector<const char*> mimeTypePointers;
	mimeTypePointers.reserve( mimeTypes.size() );
	for ( const auto& mimeType : mimeTypes )
		mimeTypePointers.emplace_back( mimeType.c_str() );

	auto context = std::make_unique<ClipboardDataContext>(
		ClipboardDataContext{ std::move( callback ), std::move( cleanup ) } );
	if ( !SDL_SetClipboardData( clipboardDataCallback, clipboardCleanupCallback, context.get(),
								mimeTypePointers.data(), mimeTypePointers.size() ) )
		return false;

	context.release();
	return true;
}

bool ClipboardSDL::clearData() {
	return SDL_ClearClipboardData();
}

Clipboard::Data ClipboardSDL::getData( const std::string& mimeType ) {
	size_t size = 0;
	void* data = SDL_GetClipboardData( mimeType.c_str(), &size );
	if ( !data )
		return {};

	const auto bytes = static_cast<const Uint8*>( data );
	Data result( bytes, bytes + size );
	SDL_free( data );
	return result;
}

std::vector<std::string> ClipboardSDL::getMimeTypes() {
	size_t count = 0;
	char** mimeTypes = SDL_GetClipboardMimeTypes( &count );
	if ( !mimeTypes )
		return {};

	std::vector<std::string> result;
	result.reserve( count );
	for ( size_t i = 0; i < count; ++i )
		result.emplace_back( mimeTypes[i] );
	SDL_free( mimeTypes );
	return result;
}

bool ClipboardSDL::hasData( const std::string& mimeType ) const {
	return SDL_HasClipboardData( mimeType.c_str() );
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
