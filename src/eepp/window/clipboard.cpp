#include <eepp/window/clipboard.hpp>
#include <memory>

namespace EE { namespace Window {

Clipboard::Clipboard( EE::Window::Window* window ) : mWindow( window ) {}

Clipboard::~Clipboard() {}

EE::Window::Window* Clipboard::getWindow() const {
	return mWindow;
}

bool Clipboard::setMimeData( std::vector<std::pair<std::string, std::string>> dataByMimeType ) {
	if ( dataByMimeType.empty() )
		return false;
	std::vector<std::string> mimeTypes;
	mimeTypes.reserve( dataByMimeType.size() );
	for ( const auto& entry : dataByMimeType )
		mimeTypes.push_back( entry.first );
	// Owned by the callback: released when the backend drops the offer.
	auto data = std::make_shared<std::vector<std::pair<std::string, std::string>>>(
		std::move( dataByMimeType ) );
	return setData(
		[data]( const char* mimeType, size_t* size ) -> const void* {
			if ( mimeType ) {
				for ( const auto& entry : *data ) {
					if ( entry.first == mimeType ) {
						*size = entry.second.size();
						return entry.second.data();
					}
				}
			}
			*size = 0;
			return nullptr;
		},
		nullptr, mimeTypes );
}

std::string Clipboard::getDataString( const std::string& mimeType ) {
	Data data = getData( mimeType );
	return std::string( data.begin(), data.end() );
}

const std::vector<std::string>& Clipboard::getTextMimeTypes() {
	// UTF-8 text first; the X11 target names are what X11 clients request text as.
	static const std::vector<std::string> types{ "text/plain;charset=utf-8", "UTF8_STRING",
												 "text/plain", "TEXT", "STRING" };
	return types;
}

}} // namespace EE::Window
