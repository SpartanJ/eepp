#include <algorithm>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/systemfontresolver.hpp>

namespace EE { namespace Graphics {

FontService::FontService( ResourceScope& resourceScope ) : mResourceScope( resourceScope ) {}

ResourceScope& FontService::getResourceScope() const {
	return mResourceScope;
}

FontPtr FontService::findHandle( Font* font ) const {
	if ( !font )
		return {};
	FontPtr handle = mResourceScope.findFont( font->getId() );
	return handle.get() == font ? handle : FontPtr{};
}

void FontService::onFontRemoved( Font* font ) {
	if ( mColorEmojiFont.get() == font )
		mColorEmojiFont.reset();
	if ( mEmojiFont.get() == font )
		mEmojiFont.reset();
	removeFallbackFont( font );
	mSystemFallbackFonts.erase(
		std::remove_if( mSystemFallbackFonts.begin(), mSystemFallbackFonts.end(),
						[font]( const FontPtr& systemFont ) { return systemFont.get() == font; } ),
		mSystemFallbackFonts.end() );
}

void FontService::setColorEmojiFont( Font* font ) {
	mColorEmojiFont = findHandle( font );
}

Font* FontService::getColorEmojiFont() const {
	return mColorEmojiFont.get();
}

Font* FontService::getEmojiFont() const {
	return mEmojiFont.get();
}

void FontService::setEmojiFont( Font* font ) {
	mEmojiFont = findHandle( font );
}

const std::vector<FontPtr>& FontService::getFallbackFonts() const {
	return mFallbackFonts;
}

bool FontService::hasFallbackFonts() const {
	return !mFallbackFonts.empty();
}

bool FontService::addFallbackFont( FontPtr fallbackFont ) {
	if ( fallbackFont && std::find( mFallbackFonts.begin(), mFallbackFonts.end(), fallbackFont ) ==
							 mFallbackFonts.end() ) {
		mFallbackFonts.emplace_back( std::move( fallbackFont ) );
		return true;
	}
	return false;
}

bool FontService::addFallbackFont( Font* fallbackFont ) {
	return addFallbackFont( findHandle( fallbackFont ) );
}

bool FontService::removeFallbackFont( Font* fallbackFont ) {
	auto it = std::find_if(
		mFallbackFonts.begin(), mFallbackFonts.end(),
		[fallbackFont]( const FontPtr& font ) { return font.get() == fallbackFont; } );
	if ( it == mFallbackFonts.end() )
		return false;
	mFallbackFonts.erase( it );
	return true;
}

FontHinting FontService::getHinting() const {
	return mHinting;
}

void FontService::setHinting( FontHinting hinting ) {
	mHinting = hinting;
	for ( const FontPtr& fontHandle : mResourceScope.getFonts() ) {
		Font* font = fontHandle.get();
		if ( font->getType() == FontType::TTF ) {
			auto ttf = static_cast<FontTrueType*>( font );
			if ( ttf->getFontService() == this && !ttf->isEmojiFont() )
				ttf->setHinting( hinting );
		}
	}
}

FontAntialiasing FontService::getAntialiasing() const {
	return mAntialiasing;
}

void FontService::setAntialiasing( FontAntialiasing antialiasing ) {
	mAntialiasing = antialiasing;
	for ( const FontPtr& fontHandle : mResourceScope.getFonts() ) {
		Font* font = fontHandle.get();
		if ( font->getType() == FontType::TTF ) {
			auto ttf = static_cast<FontTrueType*>( font );
			if ( ttf->getFontService() == this && !ttf->isEmojiFont() )
				ttf->setAntialiasing( antialiasing );
		}
	}
}

Font* FontService::getByInternalId( Uint32 internalId ) const {
	for ( const FontPtr& fontHandle : mResourceScope.getFonts() ) {
		Font* font = fontHandle.get();
		if ( font->getType() == FontType::TTF &&
			 static_cast<FontTrueType*>( font )->getFontInternalId() == internalId )
			return font;
	}
	return nullptr;
}

ResourcePtr<FontTrueType> FontService::loadSystemFont( const FontDesc& desc ) {
	if ( desc.path.empty() )
		return {};

	FontTrueTypePtr ttf( eeNew( FontTrueType, ( desc.family, *this ) ),
						 ResourceDeleter<FontTrueType>() );
	if ( !ttf->loadFromFile( desc.path, desc.faceIndex ) )
		return {};

	ttf->setHinting( mHinting );
	ttf->setAntialiasing( mAntialiasing );
	// A standalone font can outlive this service, so it must not retain a borrowed service pointer.
	ttf->setFontService( nullptr );
	return ttf;
}

FontTrueType* FontService::getOrLoadSystemFallbackFont( const FontDesc& desc ) {
	if ( desc.path.empty() )
		return nullptr;

	for ( const FontPtr& fontHandle : mSystemFallbackFonts ) {
		auto* ttf = static_cast<FontTrueType*>( fontHandle.get() );
		if ( ttf->getInfo().fontpath + ttf->getInfo().filename == desc.path &&
			 ttf->getFaceIndex() == desc.faceIndex )
			return ttf;
	}

	FontTrueTypePtr ttf =
		FontTrueType::New( desc.family, desc.path, desc.faceIndex, mResourceScope );
	if ( !ttf || !ttf->loaded() ) {
		if ( ttf )
			mResourceScope.eraseLocalFont( ttf.get() );
		return nullptr;
	}

	mSystemFallbackFonts.emplace_back( ttf );
	return ttf.get();
}

}} // namespace EE::Graphics
