#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/window/engine.hpp>

#include <bitset>

namespace EE { namespace Graphics {

// Maximum Unicode code point for emojis (covering up to U+1FAFF)
constexpr size_t MAX_EMOJI_CODE_POINT = 0x1FAFF + 1; // 129791

// Static bitset to store emoji code points
static const std::bitset<MAX_EMOJI_CODE_POINT> emojiLookup = []() {
	std::bitset<MAX_EMOJI_CODE_POINT> bits;

	// Define emoji ranges (Unicode 15.1, tailored for Noto Sans Color Emoji)
	static constexpr struct {
		uint32_t min, max;
	} ranges[] = {
		{ 0x1F300, 0x1F5FF }, // Miscellaneous Symbols and Pictographs
		{ 0x1F600, 0x1F64F }, // Emoticons
		{ 0x1F680, 0x1F6FF }, // Transport and Map Symbols
		{ 0x1F900, 0x1F9FF }, // Supplemental Symbols and Pictographs
		{ 0x1FA70, 0x1FAFF }, // Symbols and Pictographs Extended-A
		{ 0x2600, 0x26FF },	  // Miscellaneous Symbols
		{ 0x2700, 0x27BF },	  // Dingbats
		{ 0x1F1E6, 0x1F1FF }, // Regional Indicator Symbols
		{ 0x1F000, 0x1F02F }, // Mahjong Tiles, Domino Tiles
		{ 0x1F0A0, 0x1F0FF }, // Playing Cards
		{ 0x1F100, 0x1F1FF }, // Enclosed Alphanumeric Supplement (partial)
		{ 0x1F200, 0x1F2FF }, // Enclosed Ideographic Supplement (partial)
	};

	// Define single emoji code points
	static constexpr uint32_t singles[] = {
		0x203C, 0x2049, 0x2139, 0x231A, 0x231B, 0x2328, 0x23CF, 0x23E9,
		0x23F0, 0x23F3, 0x25AA, 0x25AB, 0x25B6, 0x25C0, 0x25FB, 0x25FC,
		0x25FD, 0x25FE, 0x2B50, 0x2B55, 0x3030, 0x303D, 0x3297, 0x3299,
	};

	// Set bits for ranges
	for ( const auto& range : ranges )
		for ( uint32_t i = range.min; i <= range.max; ++i )
			bits.set( i );

	// Set bits for single code points
	for ( const auto& code : singles )
		bits.set( code );

	// Set bits for emoji variation selector and ZWJ
	bits.set( 0xFE0F ); // Variation Selector-16
	bits.set( 0x200D ); // Zero Width Joiner

	return bits;
}();

bool Font::isEmojiCodePoint( const Uint32& codePoint ) {
	// Check if code point is within valid range and is an emoji
	return codePoint < MAX_EMOJI_CODE_POINT && emojiLookup[codePoint];
}

bool Font::containsEmojiCodePoint( const String& string ) {
	for ( const auto& codePoint : string ) {
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
