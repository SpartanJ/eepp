#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>

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

const std::vector<Font*>& FontManager::getFallbackFonts() const {
	return mFallbackFonts;
}

bool FontManager::hasFallbackFonts() const {
	return !mFallbackFonts.empty();
}

bool FontManager::addFallbackFont( Font* fallbackFont ) {
	if ( fallbackFont && std::find( mFallbackFonts.begin(), mFallbackFonts.end(), fallbackFont ) ==
							 mFallbackFonts.end() ) {
		mFallbackFonts.emplace_back( fallbackFont );
		return true;
	}
	return false;
}

bool FontManager::removeFallbackFont( Font* fallbackFont ) {
	auto fallbackFontIt = std::find( mFallbackFonts.begin(), mFallbackFonts.end(), fallbackFont );
	if ( fallbackFontIt != mFallbackFonts.end() ) {
		mFallbackFonts.erase( fallbackFontIt );
		return true;
	}
	return false;
}

FontHinting FontManager::getHinting() const {
	return mHinting;
}

void FontManager::setHinting( FontHinting hinting ) {
	mHinting = hinting;

	for ( auto [_, font] : mResources ) {
		if ( font->getType() == FontType::TTF ) {
			auto ttf = static_cast<FontTrueType*>( font );
			if ( !ttf->isEmojiFont() )
				ttf->setHinting( hinting );
		}
	}
}

FontAntialiasing FontManager::getAntialiasing() const {
	return mAntialiasing;
}

void FontManager::setAntialiasing( FontAntialiasing antialiasing ) {
	mAntialiasing = antialiasing;

	for ( auto [_, font] : mResources ) {
		if ( font->getType() == FontType::TTF ) {
			auto ttf = static_cast<FontTrueType*>( font );
			if ( !ttf->isEmojiFont() )
				ttf->setAntialiasing( antialiasing );
		}
	}
}

}} // namespace EE::Graphics
