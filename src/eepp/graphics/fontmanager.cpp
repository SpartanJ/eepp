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

Font* FontManager::getFallbackFont() const {
	return mFallbackFont;
}

void FontManager::setFallbackFont( Font* fallbackFont ) {
	mFallbackFont = fallbackFont;
}

FontHinting FontManager::getHinting() const {
	return mHinting;
}

void FontManager::setHinting( FontHinting hinting ) {
	mHinting = hinting;
}

FontAntialiasing FontManager::getAntialiasing() const {
	return mAntialiasing;
}

void FontManager::setAntialiasing( FontAntialiasing antialiasing ) {
	mAntialiasing = antialiasing;
}

}} // namespace EE::Graphics
