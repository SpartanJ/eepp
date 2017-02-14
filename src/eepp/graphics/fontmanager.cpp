#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(FontManager)

FontManager::FontManager() {
}

FontManager::~FontManager() {
}

Graphics::Font * FontManager::add( Graphics::Font * Font ) {
	eeASSERT( NULL != Font );
	return ResourceManager<Graphics::Font>::add( Font );
}

}}
