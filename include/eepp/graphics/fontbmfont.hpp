#ifndef EE_GRAPHICS_FONTBMFONT_HPP
#define EE_GRAPHICS_FONTBMFONT_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace System {
class Pack;
class IOStream;
}} // namespace EE::System

namespace EE { namespace Graphics {

/** @brief Implementation of AngelCode BMFont fonts. */
class EE_API FontBMFont : public Font {
  public:
	static FontBMFont* New( const std::string fontName );

	static FontBMFont* New( const std::string fontName, const std::string& filename );

	~FontBMFont();

	bool loadFromFile( const std::string& filename );

	bool loadFromMemory( const void* data, std::size_t sizeInBytes,
						 const std::string& imageFileBasePath );

	bool loadFromStream( IOStream& stream );

	bool loadFromPack( Pack* pack, std::string filePackPath );

	bool isMonospace() const;

	bool isScalable() const;

	const Font::Info& getInfo() const;

	const Glyph& getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
						   Float outlineThickness = 0, Float maxWidth = 0 ) const;

	GlyphDrawable* getGlyphDrawable( Uint32 codePoint, unsigned int characterSize,
									 bool bold = false, Float outlineThickness = 0,
									 const Float& maxWidth = 0 ) const;

	Float getKerning( Uint32 first, Uint32 second, unsigned int characterSize, bool bold ) const;

	Float getLineSpacing( unsigned int characterSize ) const;

	Uint32 getFontHeight( const Uint32& characterSize ) const;

	Float getUnderlinePosition( unsigned int characterSize ) const;

	Float getUnderlineThickness( unsigned int characterSize ) const;

	Texture* getTexture( unsigned int characterSize ) const;

	bool loaded() const;

	FontBMFont& operator=( const FontBMFont& right );

  protected:
	FontBMFont( const std::string FontName );

	typedef std::map<Uint64, Glyph> GlyphTable; ///< Table mapping a codepoint to its glyph
	typedef std::map<Uint64, GlyphDrawable*> GlyphDrawableTable;

	struct Page {
		~Page();

		GlyphTable glyphs; ///< Table mapping code points to their corresponding glyph
		GlyphDrawableTable
			drawables;	  ///> Table mapping code points to their corresponding glyph drawables.
		Texture* texture; ///< Texture containing the pixels of the glyphs
	};

	void cleanup();

	typedef std::map<unsigned int, Page>
		PageTable; ///< Table mapping a character size to its page (texture)

	Font::Info mInfo;		  ///< Information about the font
	mutable PageTable mPages; ///< Table containing the glyphs pages by character size
	std::string mFilePath;
	Uint32 mFontSize;
	bool mIsMonospace{ false };

	Glyph loadGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
					 Float outlineThickness ) const;
};

}} // namespace EE::Graphics

#endif
