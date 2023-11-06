#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Graphics {

bool Font::isEmojiCodePoint( const Uint32& codePoint ) {
	const Uint32 rangeMin = 127744;
	const Uint32 rangeMax = 131069;
	const Uint32 rangeMin2 = 126980;
	const Uint32 rangeMax2 = 127569;
	const Uint32 rangeMin3 = 8986;
	const Uint32 rangeMax3 = 12953;
	return codePoint >= 8986 && ( ( rangeMin <= codePoint && codePoint <= rangeMax ) ||
								  ( rangeMin2 <= codePoint && codePoint <= rangeMax2 ) ||
								  ( rangeMin3 <= codePoint && codePoint <= rangeMax3 ) );
}

bool Font::containsEmojiCodePoint( const String& string ) {
	for ( auto& codePoint : string ) {
		if ( Font::isEmojiCodePoint( codePoint ) )
			return true;
	}
	return false;
}

std::vector<std::size_t> Font::emojiCodePointsPositions( const String& string ) {
	std::vector<std::size_t> positions;
	for ( size_t i = 0; i < string.size(); i++ ) {
		if ( Font::isEmojiCodePoint( string[i] ) )
			positions.emplace_back( i );
	}
	return positions;
}

Font::Font( const FontType& Type, const std::string& Name ) : mType( Type ), mNumCallBacks( 0 ) {
	this->setName( Name );
	FontManager::instance()->add( this );
}

Font::~Font() {
	if ( !FontManager::instance()->isDestroying() ) {
		FontManager::instance()->remove( this, false );
	}
}

const FontType& Font::getType() const {
	return mType;
}

const std::string& Font::getName() const {
	return mFontName;
}

void Font::setName( const std::string& name ) {
	mFontName = name;
	mFontHash = String::hash( mFontName );
}

const String::HashType& Font::getId() {
	return mFontHash;
}

Uint32 Font::getFontStyle() const {
	Uint32 style = 0;
	if ( isBold() )
		style |= Text::Bold;
	if ( isItalic() )
		style |= Text::Italic;
	if ( isBoldItalic() )
		style |= Text::Bold | Text::Italic;
	return style;
}

Uint32 Font::pushFontEventCallback( const FontEventCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[mNumCallBacks] = cb;
	return mNumCallBacks;
}

void Font::popFontEventCallback( const Uint32& callbackId ) {
	mCallbacks[callbackId] = 0;
	mCallbacks.erase( mCallbacks.find( callbackId ) );
}

void Font::sendEvent( const Event& event ) {
	for ( const auto& cb : mCallbacks ) {
		cb.second( cb.first, event, this );
	}
}

}} // namespace EE::Graphics
