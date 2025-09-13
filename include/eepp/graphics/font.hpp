#ifndef EE_GRAPHICSCFONT_H
#define EE_GRAPHICSCFONT_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureregion.hpp>

using namespace std::literals;

namespace EE { namespace Graphics {

class Font;

struct EE_API Glyph {
	Float advance{ 0 }; ///< Offset to move horizontally to the next character
	Rectf bounds;	   ///< Bounding rectangle of the glyph, in coordinates relative to the baseline
	Rect textureRect;  ///< Texture coordinates of the glyph inside the font's texture
	Sizef size;		   ///< The glyph bitmap size on screen
	int lsbDelta{ 0 }; //!< Left offset after forced autohint. Internally used by getKerning()
	int rsbDelta{ 0 }; //!< Right offset after forced autohint. Internally used by getKerning()
	Font* font{ nullptr }; ///< The glyph font
};

enum class FontType { TTF, BMF, Sprite };

enum FontHorizontalAlign : Uint32 {
	TEXT_ALIGN_LEFT = ( 0 << 0 ),
	TEXT_ALIGN_RIGHT = ( 1 << 0 ),
	TEXT_ALIGN_CENTER = ( 2 << 0 ),
	TEXT_HALIGN_MASK = ( 3 << 0 )
};

enum FontVerticalAlign : Uint32 {
	TEXT_ALIGN_TOP = ( 0 << 2 ),
	TEXT_ALIGN_BOTTOM = ( 1 << 2 ),
	TEXT_ALIGN_MIDDLE = ( 2 << 2 ),
	TEXT_VALIGN_MASK = ( 3 << 2 )
};

enum class FontHinting { None, Slight, Full };

enum class FontAntialiasing { None, Grayscale, Subpixel };

/** @brief Font interface class. */
class EE_API Font {
  public:
	enum Event { Load, Unload };

	typedef std::function<void( Uint32, Event, Font* )> FontEventCallback;

	struct Info {
		std::string family;	  ///< The font family
		std::string fontpath; ///< The directory path of the font
		std::string filename; ///< The file name
	};

	static std::string_view fontHintingToString( FontHinting hint ) {
		switch ( hint ) {
			case FontHinting::None:
				return "none"sv;
			case FontHinting::Slight:
				return "slight"sv;
			case FontHinting::Full:
				break;
		}
		return "full"sv;
	}

	static FontHinting fontHintingFromString( std::string_view str ) {
		if ( str == "none"sv )
			return FontHinting::None;
		if ( str == "slight"sv )
			return FontHinting::Slight;
		return FontHinting::Full;
	}

	static std::string_view fontAntialiasingToString( FontAntialiasing aa ) {
		switch ( aa ) {
			case FontAntialiasing::None:
				return "none"sv;
			case FontAntialiasing::Grayscale:
				return "grayscale"sv;
			case FontAntialiasing::Subpixel:
				break;
		}
		return "subpixel"sv;
	}

	static FontAntialiasing fontAntialiasingFromString( std::string_view str ) {
		if ( str == "none"sv )
			return FontAntialiasing::None;
		if ( str == "subpixel"sv )
			return FontAntialiasing::Subpixel;
		return FontAntialiasing::Grayscale;
	}

	static inline Uint32 getHorizontalAlign( const Uint32& flags ) {
		return flags & TEXT_HALIGN_MASK;
	}

	static inline Uint32 getVerticalAlign( const Uint32& flags ) {
		return flags & TEXT_VALIGN_MASK;
	}

	static bool isEmojiCodePoint( const Uint32& codePoint );

	static bool containsEmojiCodePoint( const String& string );

	static std::vector<std::size_t> emojiCodePointsPositions( const String& string );

	virtual ~Font();

	/** @return The current font height */
	virtual Uint32 getFontHeight( const Uint32& characterSize ) const = 0;

	/** @return The type of the instance of the font */
	const FontType& getType() const;

	/** @return The font name */
	const std::string& getName() const;

	/** Change the font name ( and id, because it's the font name hash ) */
	void setName( const std::string& setName );

	/** @return The font id */
	const String::HashType& getId();

	virtual bool isMonospace() const = 0;

	virtual bool isScalable() const = 0;

	virtual const Info& getInfo() const = 0;

	virtual Glyph getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
							Float outlineThickness = 0 ) const = 0;

	/** @return The glyph drawable that represents the glyph in a texture. The glyph drawable
	 * allocation is managed by the font. */
	virtual GlyphDrawable* getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
											 bool bold = false, bool italic = false,
											 Float outlineThickness = 0 ) const = 0;

	virtual Float getKerning( Uint32 first, Uint32 second, unsigned int characterSize, bool bold,
							  bool italic, Float outlineThickness = 0 ) const = 0;

	virtual Float getAscent( unsigned int characterSize ) const {
		return getLineSpacing( characterSize );
	}

	virtual Float getDescent( unsigned int ) const { return 0.f; }

	virtual Float getLineSpacing( unsigned int characterSize ) const = 0;

	virtual Float getUnderlinePosition( unsigned int characterSize ) const = 0;

	virtual Float getUnderlineThickness( unsigned int characterSize ) const = 0;

	virtual Texture* getTexture( unsigned int characterSize ) const = 0;

	virtual Uint32 getFontStyle() const;

	virtual bool isRegular() const { return !isBold() && !isItalic(); }

	virtual bool isBold() const { return false; }

	virtual bool isItalic() const { return false; }

	virtual bool isBoldItalic() const { return false; }

	virtual bool hasBold() const { return false; }

	virtual bool hasItalic() const { return false; }

	virtual bool hasBoldItalic() const { return false; }

	virtual bool loaded() const = 0;

	/** Push a new on resource change callback.
	 * @return The Callback Id
	 */
	Uint32 pushFontEventCallback( const FontEventCallback& cb );

	/** Pop the on resource change callback id indicated. */
	void popFontEventCallback( const Uint32& callbackId );

  protected:
	FontType mType;
	std::string mFontName;
	String::HashType mFontHash;
	Uint32 mNumCallBacks;
	std::map<Uint32, FontEventCallback> mCallbacks;

	Font( const FontType& Type, const std::string& setName );

	void sendEvent( const Event& event );
};

}} // namespace EE::Graphics

#endif
