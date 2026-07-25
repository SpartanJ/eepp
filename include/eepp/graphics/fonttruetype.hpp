#ifndef EE_GRAPHICS_FONTTRUETYPE_HPP
#define EE_GRAPHICS_FONTTRUETYPE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/texture.hpp>
#include <memory>

namespace EE { namespace System {
class Pack;
class IOStream;
}} // namespace EE::System

namespace EE { namespace Graphics {

enum class FontWeight : Uint16;
struct FontDesc;
class ResourceScope;

class FontTrueType;
class FontService;
using FontTrueTypePtr = ResourcePtr<FontTrueType>;
using FontTrueTypeWeakPtr = ResourceWeakPtr<FontTrueType>;

class EE_API FontTrueType : public Font {
  public:
	static FontTrueTypePtr New( const std::string& FontName );
	static FontTrueTypePtr New( const std::string& FontName, ResourceScope& resourceScope );

	static FontTrueTypePtr New( const std::string& FontName, const std::string& filename );
	static FontTrueTypePtr New( const std::string& FontName, const std::string& filename,
								ResourceScope& resourceScope );

	static FontTrueTypePtr New( const std::string& FontName, const std::string& filename,
								Uint32 faceIndex );
	static FontTrueTypePtr New( const std::string& FontName, const std::string& filename,
								Uint32 faceIndex, ResourceScope& resourceScope );

	~FontTrueType();

	bool loadFromFile( const std::string& filename, Uint32 faceIndex = 0 );

	bool loadFromMemory( const void* data, std::size_t sizeInBytes, bool copyData = true,
						 Uint32 faceIndex = 0 );

	bool loadFromStream( IOStream& stream, Uint32 faceIndex = 0 );

	bool loadFromPack( Pack* pack, std::string filePackPath, Uint32 faceIndex = 0 );

	const Font::Info& getInfo() const;

	bool getFontDesc( FontDesc& desc ) const;

	const Uint32& getFaceIndex() const { return mFaceIndex; }

	/** Sets the OpenType variable font "wght" axis, if available. */
	bool setVariableFontWeight( FontWeight weight );

	Glyph getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
					Float outlineThickness = 0 ) const;

	Float getGlyphAdvance( Uint32 codePoint, unsigned int characterSize, bool bold = false,
						   bool italic = false, Float outlineThickness = 0 ) const;

	Glyph getGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold, bool italic,
						   Float outlineThickness = 0 ) const;

	GlyphDrawable* getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
									 bool bold = false, bool italic = false,
									 Float outlineThickness = 0 ) const;

	GlyphDrawable* getGlyphDrawableFromGlyphIndex( Uint32 glyphIndex, unsigned int characterSize,
												   bool bold = false, bool italic = false,
												   Float outlineThickness = 0 ) const;

	Float getKerning( Uint32 first, Uint32 second, unsigned int characterSize, bool bold,
					  bool italic, Float outlineThickness = 0 ) const;

	Float getKerningFromGlyphIndex( Uint32 first, Uint32 second, unsigned int characterSize,
									bool bold, bool italic, Float outlineThickness = 0 ) const;

	Float getLineSpacing( unsigned int characterSize ) const;

	Float getAscent( unsigned int characterSize ) const;

	Float getDescent( unsigned int characterSize ) const;

	Uint32 getFontHeight( const Uint32& characterSize ) const;

	Float getUnderlinePosition( unsigned int characterSize ) const;

	Float getUnderlineThickness( unsigned int characterSize ) const;

	const TexturePtr& getTexture( unsigned int characterSize ) const;

	bool loaded() const;
	FontService* getFontService() const;

	FontTrueType( const FontTrueType& ) = delete;
	FontTrueType& operator=( const FontTrueType& ) = delete;

	bool getBoldAdvanceSameAsRegular() const;

	/** You can enable this to not add more space in the advance properties for bold fonts, so they
	 * advance like a regular glyph (useful for monospaced fonts). */
	void setBoldAdvanceSameAsRegular( bool boldAdvanceSameAsRegular );

	bool isColorEmojiFont() const;

	bool hasSvgGlyphs() const;

	bool hasColrGlyphs() const;

	/** @return True if the font identifies itself as a monospace font and currently does not hold
	 * any non-monospaced glyph (from a fallback font) */
	bool isMonospace() const;

	/** @return True if the font identifies itself as a monospace font */
	bool isIdentifiedAsMonospace() const;

	bool isScalable() const;

	bool isEmojiFont() const;

	bool hasGlyph( Uint32 codePoint ) const;

	void setIsColorEmojiFont( bool isColorEmojiFont );

	void setIsEmojiFont( bool isEmojiFont );

	void setForceIsMonospace( bool isMonospace );

	bool isEmojiFallbackEnabled() const;

	void setEnableEmojiFallback( bool enableEmojiFallback );

	const Uint32& getFontInternalId() const;

	bool isFallbackFontEnabled() const;

	void setEnableFallbackFont( bool enableFallbackFont );

	bool isSystemFallbackEnabled() const;

	void setEnableSystemFallback( bool enableSystemFallback );

	bool getEnableDynamicMonospace() const;

	void setEnableDynamicMonospace( bool enableDynamicMonospace );

	FontHinting getHinting() const;

	void setHinting( FontHinting hinting );

	FontAntialiasing getAntialiasing() const;

	void setAntialiasing( FontAntialiasing antialiasing );

	virtual bool isBold() const { return mIsBold && !mIsItalic; }

	virtual bool isItalic() const { return mIsItalic && !mIsBold; }

	virtual bool isBoldItalic() const { return mIsBold && mIsItalic; }

	virtual bool hasBold() const { return mIsBold || mFontBold != nullptr; }

	virtual bool hasItalic() const { return mIsItalic || mFontItalic != nullptr; }

	virtual bool hasBoldItalic() const { return isBoldItalic() || mFontBoldItalic != nullptr; }

	const FontTrueTypePtr& getBoldFont() const { return mFontBold; }

	const FontTrueTypePtr& getItalicFont() const { return mFontItalic; }

	const FontTrueTypePtr& getBoldItalicFont() const { return mFontBoldItalic; }

	void setBoldFont( const FontTrueTypePtr& fontBold );

	void setItalicFont( const FontTrueTypePtr& fontItalic );

	void setBoldItalicFont( const FontTrueTypePtr& fontBoldItalic );

	void* face() const { return mFace; }

	void* hb() const { return mHBFont; }

	bool setCurrentSize( unsigned int characterSize ) const;

	void clearCache();

	Uint32 getGlyphIndex( const Uint32& codePoint ) const;

  protected:
	friend class Text;
	friend class TextLayout;
	friend class FontService;
	friend class ResourceScope;

	explicit FontTrueType( const std::string& FontName, FontService& fontService );
	void setFontService( FontService* fontService );

	struct Row {
		Row( unsigned int rowTop, unsigned int rowHeight ) :
			width( 0 ), top( rowTop ), height( rowHeight ) {}

		unsigned int width;	 ///< Current width of the row
		unsigned int top;	 ///< Y position of the row into the texture
		unsigned int height; ///< Height of the row
	};

	typedef UnorderedMap<Uint64, Glyph> GlyphTable; ///< Table mapping a codepoint to its glyph
	typedef UnorderedMap<Uint64, GlyphDrawable*> GlyphDrawableTable;

	struct Page {
		explicit Page( const Uint32 fontInternalId, const std::string& pageName,
					   const FontTrueType* font );

		~Page();

		GlyphTable glyphs; ///< Table mapping code points to their corresponding glyph
		GlyphDrawableTable
			drawables;		///> Table mapping code points to their corresponding glyph drawables.
		TexturePtr texture; ///< Texture containing the pixels of the glyphs
		std::vector<Row> rows;		///< List containing the position of all the existing rows
		Uint32 fontInternalId{ 0 }; // The font internal id
		unsigned int nextRow;		///< Y position of the next new row in the texture
		const FontTrueType* font{ nullptr };
	};

	void cleanup();

	Glyph getGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold, bool italic,
						   Float outlineThickness, Page& page ) const;

	Glyph getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
					Float outlineThickness, Page& page ) const;

	GlyphDrawable* getGlyphDrawableFromGlyphIndex( Uint32 glyphIndex, unsigned int characterSize,
												   bool bold, bool italic, Float outlineThickness,
												   Page& page ) const;

	Glyph loadGlyphByIndex( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
							Float outlineThickness, Page& page ) const;

	Rect findGlyphRect( Page& page, unsigned int width, unsigned int height ) const;

	Page& getPage( unsigned int characterSize ) const;

	typedef UnorderedMap<unsigned int, std::unique_ptr<Page>>
		PageTable; ///< Table mapping a character size to its page (texture)

	void* mLibrary{ nullptr }; ///< Pointer to the internal library interface (it is typeless to
							   ///< avoid exposing implementation details)
	void* mFace{ nullptr }; ///< Pointer to the internal font face (it is typeless to avoid exposing
							///< implementation details)
	void* mStreamRec{ nullptr }; ///< Pointer to the stream rec instance (it is typeless to avoid
								 ///< exposing implementation details)
	void* mStroker{ nullptr };	 ///< Pointer to the stroker (it is typeless to avoid exposing
								 ///< implementation details)
	void* mHBFont{ nullptr };
	mutable ScopedBuffer mMemCopy; ///< If loaded from memory, this is the file copy in memory
	Font::Info mInfo;			   ///< Information about the font
	Uint32 mFontInternalId{ 0 };
	mutable PageTable mPages; ///< Table containing the glyphs pages by character size
	mutable std::vector<Uint8>
		mPixelBuffer; ///< Pixel buffer holding a glyph's pixels before being written to the texture
	bool mBoldAdvanceSameAsRegular{ false };
	bool mIsColorEmojiFont{ false };
	bool mIsEmojiFont{ false };
	bool mHasSvgGlyphs{ false };
	bool mHasColrGlyphs{ false };
	bool mIsBitmapOnly{ false };
	mutable bool mIsMonospace{ false };
	mutable bool mIsMonospaceComplete{ false };
	mutable bool mUsingFallback{ false };
	bool mEnableEmojiFallback{ true };
	bool mEnableFallbackFont{ true };
	bool mEnableSystemFallback{ true };
	bool mEnableDynamicMonospace{ false };
	bool mIsBold{ false };
	bool mIsItalic{ false };
	mutable bool mIsMonospaceCompletePending{ false };
	mutable UnorderedMap<unsigned int, unsigned int> mClosestCharacterSize;
	mutable UnorderedMap<Uint32, Uint32> mCodePointIndexCache;
	mutable UnorderedMap<Uint32, std::tuple<Uint32, Uint32, bool>> mKeyCache;
	mutable UnorderedMap<Uint64, Float> mKerningCache;		// For codepoints (getKerning)
	mutable UnorderedMap<Uint64, Float> mKerningGlyphCache; // For glyph indices
	mutable UnorderedMap<unsigned int, UnorderedMap<Uint64, Float>> mGlyphAdvanceCache;
	FontHinting mHinting{ FontHinting::Full };
	FontAntialiasing mAntialiasing{ FontAntialiasing::Grayscale };
	FontService* mFontService{ nullptr };
	Uint32 mFaceIndex{ 0 };
	FontTrueTypePtr mFontBold;
	FontTrueTypePtr mFontItalic;
	FontTrueTypePtr mFontBoldItalic;

	Float getGlyphTopOffset( unsigned int characterSize ) const;

	void updateFontInternalId();

	bool setFontFace( void* face );

	void updateMonospaceState() const;
};

}} // namespace EE::Graphics

#endif
