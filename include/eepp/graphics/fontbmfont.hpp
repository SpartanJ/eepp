#ifndef EE_GRAPHICS_FONTBMFONT_HPP
#define EE_GRAPHICS_FONTBMFONT_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace System {
class Pack;
class IOStream;
}}

namespace EE { namespace Graphics {

class EE_API FontBMFont : public Font {
	public:
		static FontBMFont * New( const std::string FontName ) ;

		~FontBMFont();

		bool loadFromFile(const std::string& filename);

		bool loadFromMemory(const void* data, std::size_t sizeInBytes);

		bool loadFromStream( IOStream& stream );

		bool loadFromPack( Pack * pack, std::string filePackPath );

		const Font::Info& getInfo() const;

		const Glyph& getGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness = 0) const;

		Float getKerning(Uint32 first, Uint32 second, unsigned int characterSize) const;

		Float getLineSpacing(unsigned int characterSize) const;

		Uint32 getFontHeight( const Uint32& characterSize );

		Float getUnderlinePosition(unsigned int characterSize) const;

		Float getUnderlineThickness(unsigned int characterSize) const;

		Texture * getTexture(unsigned int characterSize) const;

		FontBMFont& operator =(const FontBMFont& right);
	protected:
		FontBMFont(const std::string FontName);

		typedef std::map<Uint64, Glyph> GlyphTable; ///< Table mapping a codepoint to its glyph

		struct Page
		{
			GlyphTable       glyphs;  ///< Table mapping code points to their corresponding glyph
			Texture *        texture; ///< Texture containing the pixels of the glyphs
		};

		void cleanup();

		typedef std::map<unsigned int, Page> PageTable; ///< Table mapping a character size to its page (texture)

		Font::Info                 mInfo;        ///< Information about the font
		mutable PageTable          mPages;       ///< Table containing the glyphs pages by character size
		std::string mFilePath;
		Uint32 mFontSize;

		Glyph loadGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness) const;
};

}}

#endif
