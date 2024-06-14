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

class EE_API FontTrueType : public Font {
  public:
	static FontTrueType* New( const std::string& FontName );

	static FontTrueType* New( const std::string& FontName, const std::string& filename );

	~FontTrueType();

	bool loadFromFile( const std::string& filename );

	bool loadFromMemory( const void* data, std::size_t sizeInBytes, bool copyData = true );

	bool loadFromStream( IOStream& stream );

	bool loadFromPack( Pack* pack, std::string filePackPath );

	const Font::Info& getInfo() const;

	const Glyph& getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
						   Float outlineThickness = 0, Float maxWidth = 0 ) const;

	const Glyph& getGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold, bool italic,
								  Float outlineThickness = 0 ) const;

	GlyphDrawable* getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
									 bool bold = false, bool italic = false,
									 Float outlineThickness = 0, const Float& maxWidth = 0 ) const;

	Float getKerning( Uint32 first, Uint32 second, unsigned int characterSize, bool bold,
					  bool italic, Float outlineThickness = 0 ) const;

	Float getKerningFromGlyphIndex( Uint32 first, Uint32 second, unsigned int characterSize,
									bool bold, bool italic, Float outlineThickness = 0 ) const;

	Float getLineSpacing( unsigned int characterSize ) const;

	Uint32 getFontHeight( const Uint32& characterSize ) const;

	Float getUnderlinePosition( unsigned int characterSize ) const;

	Float getUnderlineThickness( unsigned int characterSize ) const;

	Texture* getTexture( unsigned int characterSize ) const;

	bool loaded() const;

	FontTrueType& operator=( const FontTrueType& right );

	bool getBoldAdvanceSameAsRegular() const;

	/** You can enable this to not add more space in the advance properties for bold fonts, so they
	 * advance like a regular glyph (useful for monospaced fonts). */
	void setBoldAdvanceSameAsRegular( bool boldAdvanceSameAsRegular );

	bool isColorEmojiFont() const;

	bool isMonospace() const;

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

	virtual bool hasBoldItalic() const { return isBoldItalic() || mFontBoldItalic; }

	FontTrueType* getBoldFont() const { return mFontBold; }

	FontTrueType* getItalicFont() const { return mFontItalic; }

	FontTrueType* getBoldItalicFont() const { return mFontBoldItalic; }

	void setBoldFont( FontTrueType* fontBold );

	void setItalicFont( FontTrueType* fontItalic );

	void setBoldItalicFont( FontTrueType* fontBoldItalic );

	void* face() const { return mFace; }

	void* hb() const { return mHBFont; }

	bool setCurrentSize( unsigned int characterSize ) const;

  protected:
	friend class Text;

	explicit FontTrueType( const std::string& FontName );

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
		explicit Page( const Uint32 fontInternalId, const std::string& pageName );

		~Page();

		GlyphTable glyphs; ///< Table mapping code points to their corresponding glyph
		GlyphDrawableTable
			drawables;		  ///> Table mapping code points to their corresponding glyph drawables.
		Texture* texture;	  ///< Texture containing the pixels of the glyphs
		unsigned int nextRow; ///< Y position of the next new row in the texture
		std::vector<Row> rows; ///< List containing the position of all the existing rows
		Uint32 fontInternalId{ 0 };
	};

	void cleanup();

	const Glyph& getGlyphByIndex( Uint32 index, unsigned int characterSize, bool bold, bool italic,
								  Float outlineThickness, Page& page, const Float& maxWidth ) const;

	const Glyph& getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
						   Float outlineThickness, Page& page, const Float& maxWidth ) const;

	Uint32 getGlyphIndex( const Uint32& codePoint ) const;

	Glyph loadGlyphByIndex( Uint32 codePoint, unsigned int characterSize, bool bold, bool italic,
					 Float outlineThickness, Page& page, const Float& maxWidth = 0.f ) const;

	Rect findGlyphRect( Page& page, unsigned int width, unsigned int height ) const;

	Page& getPage( unsigned int characterSize ) const;

	typedef UnorderedMap<unsigned int, std::unique_ptr<Page>>
		PageTable; ///< Table mapping a character size to its page (texture)

	void* mLibrary; ///< Pointer to the internal library interface (it is typeless to avoid exposing
					///< implementation details)
	void* mFace; ///< Pointer to the internal font face (it is typeless to avoid exposing
				 ///< implementation details)
	void* mStreamRec; ///< Pointer to the stream rec instance (it is typeless to avoid exposing
					  ///< implementation details)
	void* mStroker;	  ///< Pointer to the stroker (it is typeless to avoid exposing implementation
					  ///< details)
	void* mHBFont{ nullptr };
	mutable ScopedBuffer mMemCopy; ///< If loaded from memory, this is the file copy in memory
	Font::Info mInfo;			   ///< Information about the font
	Uint32 mFontInternalId{ 0 };
	mutable PageTable mPages; ///< Table containing the glyphs pages by character size
	mutable std::vector<Uint8>
		mPixelBuffer; ///< Pixel buffer holding a glyph's pixels before being written to the texture
	bool mBoldAdvanceSameAsRegular;
	bool mIsColorEmojiFont{ false };
	bool mIsEmojiFont{ false };
	mutable bool mIsMonospace{ false };
	mutable bool mIsMonospaceComplete{ false };
	mutable bool mUsingFallback{ false };
	bool mEnableEmojiFallback{ true };
	bool mEnableFallbackFont{ true };
	bool mEnableDynamicMonospace{ false };
	bool mIsBold{ false };
	bool mIsItalic{ false };
	mutable UnorderedMap<unsigned int, unsigned int> mClosestCharacterSize;
	mutable UnorderedMap<Uint32, Uint32> mCodePointIndexCache;
	mutable UnorderedMap<Uint64, Float> mKerningCache;
	FontHinting mHinting{ FontHinting::Full };
	FontAntialiasing mAntialiasing{ FontAntialiasing::Grayscale };
	FontTrueType* mFontBold{ nullptr };
	FontTrueType* mFontItalic{ nullptr };
	FontTrueType* mFontBoldItalic{ nullptr };
	Uint32 mFontBoldCb{ 0 };
	Uint32 mFontItalicCb{ 0 };
	Uint32 mFontBoldItalicCb{ 0 };

	void updateFontInternalId();

	bool setFontFace( void* face );

	void updateMonospaceState();
};

}} // namespace EE::Graphics

#endif
