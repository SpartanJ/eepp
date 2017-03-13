#ifndef EE_GRAPHICS_FONTTRUETYPE_HPP
#define EE_GRAPHICS_FONTTRUETYPE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/texture.hpp>
#include <map>
#include <string>
#include <vector>

namespace EE { namespace System {
class IOStream;
}}

namespace EE { namespace Graphics {

class EE_API Glyph
{
	public:
		Glyph() : advance(0) {}

		Float advance;     ///< Offset to move horizontally to the next character
		Rectf bounds;      ///< Bounding rectangle of the glyph, in coordinates relative to the baseline
		Recti textureRect; ///< Texture coordinates of the glyph inside the font's texture
};

class EE_API FontTrueType
{
	public:
		/** Shrink the String to a max width
		* @param Str The string to shrink
		* @param MaxWidth The Max Width posible
		*/
		void shrinkText( String& Str, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& MaxWidth );

		/** Shrink the string to a max width
		* @param Str The string to shrink
		* @param MaxWidth The Max Width posible
		*/
		void shrinkText( std::string& Str, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& MaxWidth );

		/** Cache the with of the current text */
		void cacheWidth( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, std::vector<Float>& LinesWidth, Float& CachedWidth, int& NumLines, int& LargestLineCharCount );

		/** Finds the closest cursor position to the point position */
		Int32 findClosestCursorPosFromPoint( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, const Vector2i& pos );

		/** Simulates a selection request and return the initial and end cursor position when the selection worked. Otherwise both parameters will be -1. */
		void selectSubStringFromCursor( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, const Int32& CurPos, Int32& InitCur, Int32& EndCur );

		/** @return The cursor position inside the string */
		Vector2i getCursorPos( const String& Text, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& Pos );

	public:
		struct Info
		{
			std::string family; ///< The font family
		};

		FontTrueType();

		FontTrueType(const FontTrueType& copy);

		~FontTrueType();

		bool loadFromFile(const std::string& filename);

		bool loadFromMemory(const void* data, std::size_t sizeInBytes);

		bool loadFromStream(IOStream& stream);

		const Info& getInfo() const;

		const Glyph& getGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness = 0) const;

		Float getKerning(Uint32 first, Uint32 second, unsigned int characterSize) const;

		Float getLineSpacing(unsigned int characterSize) const;

		Float getUnderlinePosition(unsigned int characterSize) const;

		Float getUnderlineThickness(unsigned int characterSize) const;

		Texture * getTexture(unsigned int characterSize) const;

		FontTrueType& operator =(const FontTrueType& right);

	private:
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

		Recti findGlyphRect(Page& page, unsigned int width, unsigned int height) const;

		bool setCurrentSize(unsigned int characterSize) const;

		typedef std::map<unsigned int, Page> PageTable; ///< Table mapping a character size to its page (texture)

		void*                      mLibrary;     ///< Pointer to the internal library interface (it is typeless to avoid exposing implementation details)
		void*                      mFace;        ///< Pointer to the internal font face (it is typeless to avoid exposing implementation details)
		void*                      mStreamRec;   ///< Pointer to the stream rec instance (it is typeless to avoid exposing implementation details)
		void*                      mStroker;     ///< Pointer to the stroker (it is typeless to avoid exposing implementation details)
		int*                       mRefCount;    ///< Reference counter used by implicit sharing
		Info                       mInfo;        ///< Information about the font
		mutable PageTable          mPages;       ///< Table containing the glyphs pages by character size
		mutable std::vector<Uint8> mPixelBuffer; ///< Pixel buffer holding a glyph's pixels before being written to the texture
};

}}

#endif
