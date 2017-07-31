#ifndef EE_GRAPHICS_FONTTRUETYPE_HPP
#define EE_GRAPHICS_FONTTRUETYPE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace System {
class Pack;
class IOStream;
}}

namespace EE { namespace Graphics {

class EE_API FontTrueType : public Font {
	public:
		static FontTrueType * New( const std::string FontName ) ;

		~FontTrueType();

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

		FontTrueType& operator =(const FontTrueType& right);
	protected:
		FontTrueType(const std::string FontName);

		struct Row
		{
			Row(unsigned int rowTop, unsigned int rowHeight) : width(0), top(rowTop), height(rowHeight) {}

			unsigned int width;  ///< Current width of the row
			unsigned int top;    ///< Y position of the row into the texture
			unsigned int height; ///< Height of the row
		};

		typedef std::map<Uint64, Glyph> GlyphTable; ///< Table mapping a codepoint to its glyph

		struct Page
		{
			Page();

			~Page();

			GlyphTable       glyphs;  ///< Table mapping code points to their corresponding glyph
			Texture *        texture; ///< Texture containing the pixels of the glyphs
			unsigned int     nextRow; ///< Y position of the next new row in the texture
			std::vector<Row> rows;    ///< List containing the position of all the existing rows
		};

		void cleanup();

		Glyph loadGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness) const;

		Rect findGlyphRect(Page& page, unsigned int width, unsigned int height) const;

		bool setCurrentSize(unsigned int characterSize) const;

		typedef std::map<unsigned int, Page> PageTable; ///< Table mapping a character size to its page (texture)

		void*                      mLibrary;     ///< Pointer to the internal library interface (it is typeless to avoid exposing implementation details)
		void*                      mFace;        ///< Pointer to the internal font face (it is typeless to avoid exposing implementation details)
		void*                      mStreamRec;   ///< Pointer to the stream rec instance (it is typeless to avoid exposing implementation details)
		void*                      mStroker;     ///< Pointer to the stroker (it is typeless to avoid exposing implementation details)
		int*                       mRefCount;    ///< Reference counter used by implicit sharing
		SafeDataPointer            mMemCopy;
		Font::Info                 mInfo;        ///< Information about the font
		mutable PageTable          mPages;       ///< Table containing the glyphs pages by character size
		mutable std::vector<Uint8> mPixelBuffer; ///< Pixel buffer holding a glyph's pixels before being written to the texture
};

}}

#endif
