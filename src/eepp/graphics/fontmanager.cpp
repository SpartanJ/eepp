#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(FontManager)

FontManager::FontManager() {
}

FontManager::~FontManager() {
}

Graphics::Font * FontManager::Add( Graphics::Font * Font ) {
	eeASSERT( NULL != Font );
	return ResourceManager<Graphics::Font>::Add( Font );
}

}}
