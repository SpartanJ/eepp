#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( FontManager )

FontManager::FontManager() {}

FontManager::~FontManager() {}

Graphics::Font* FontManager::add( Graphics::Font* Font ) {
	eeASSERT( NULL != Font );
	return ResourceManager<Graphics::Font>::add( Font );
}

void FontManager::setColorEmojiFont( Font* font ) {
	mColorEmojiFont = font;
}

Graphics::Font* FontManager::getColorEmojiFont() const {
	return mColorEmojiFont;
}

Graphics::Font* FontManager::getEmojiFont() const {
	return mEmojiFont;
}

void FontManager::setEmojiFont( Graphics::Font* newEmojiFont ) {
	mEmojiFont = newEmojiFont;
}

}} // namespace EE::Graphics
