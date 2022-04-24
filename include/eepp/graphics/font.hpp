#ifndef EE_GRAPHICSCFONT_H
#define EE_GRAPHICSCFONT_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureregion.hpp>

namespace EE { namespace Graphics {

class EE_API Glyph {
  public:
	Float advance{ 0 }; ///< Offset to move horizontally to the next character
	Rectf bounds;	   ///< Bounding rectangle of the glyph, in coordinates relative to the baseline
	Rect textureRect;  ///< Texture coordinates of the glyph inside the font's texture
	int lsbDelta{ 0 }; //!< Left offset after forced autohint. Internally used by getKerning()
	int rsbDelta{ 0 }; //!< Right offset after forced autohint. Internally used by getKerning()
};

enum class FontType { TTF, BMF, Sprite };

enum FontHorizontalAlign {
	TEXT_ALIGN_LEFT = ( 0 << 0 ),
	TEXT_ALIGN_RIGHT = ( 1 << 0 ),
	TEXT_ALIGN_CENTER = ( 2 << 0 ),
	TEXT_HALIGN_MASK = ( 3 << 0 )
};

enum FontVerticalAlign {
	TEXT_ALIGN_TOP = ( 0 << 2 ),
	TEXT_ALIGN_BOTTOM = ( 1 << 2 ),
	TEXT_ALIGN_MIDDLE = ( 2 << 2 ),
	TEXT_VALIGN_MASK = ( 3 << 2 )
};

/** @brief Font interface class. */
class EE_API Font {
  public:
	enum Event { Load, Unload };

	typedef std::function<void( Uint32, Event, Font* )> FontEventCallback;

	struct Info {
		std::string family; ///< The font family
	};

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

	virtual const Info& getInfo() const = 0;

	virtual const Glyph& getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
								   Float outlineThickness = 0 ) const = 0;

	/** @return The glyph drawable that represents the glyph in a texture. The glyph drawable
	 * allocation is managed by the font. */
	virtual GlyphDrawable* getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
											 bool bold = false,
											 Float outlineThickness = 0 ) const = 0;

	virtual Float getKerning( Uint32 first, Uint32 second, unsigned int characterSize,
							  bool bold ) const = 0;

	virtual Float getLineSpacing( unsigned int characterSize ) const = 0;

	virtual Float getUnderlinePosition( unsigned int characterSize ) const = 0;

	virtual Float getUnderlineThickness( unsigned int characterSize ) const = 0;

	virtual Texture* getTexture( unsigned int characterSize ) const = 0;

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
