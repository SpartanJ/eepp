#ifndef EE_GRAPHICSCFONT_H
#define EE_GRAPHICSCFONT_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/fonthelper.hpp>
#include <eepp/graphics/texturefactory.hpp>

namespace EE { namespace Graphics {

class EE_API Glyph {
	public:
		Glyph() : advance(0) {}

		Float advance;     ///< Offset to move horizontally to the next character
		Rectf bounds;      ///< Bounding rectangle of the glyph, in coordinates relative to the baseline
		Recti textureRect; ///< Texture coordinates of the glyph inside the font's texture
};

/** @brief Font interface class. */
class EE_API Font {
	public:
		struct Info
		{
			std::string family; ///< The font family
		};

		virtual ~Font();

		/** @return The current font height */
		virtual Uint32 getFontHeight( const Uint32& characterSize ) = 0;

		/** @return The type of the instance of the font, can be FONT_TYPE_TTF ( true type font ) or FONT_TYPE_TEX ( texture font ) */
		const Uint32& getType() const;

		/** @return The font name */
		const std::string&	getName() const;

		/** Change the font name ( and id, because it's the font name hash ) */
		void setName( const std::string& setName );

		/** @return The font id */
		const Uint32& getId();

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
		void cacheWidth( const String& TextCache, const Uint32& characterSize, bool bold, Float outlineThickness, std::vector<Float>& LinesWidth, Float& CachedWidth, int& NumLines, int& LargestLineCharCount );

		/** Finds the closest cursor position to the point position */
		Int32 findClosestCursorPosFromPoint( const String& TextCache, const Uint32& characterSize, bool bold, Float outlineThickness, const Vector2i& pos );

		/** Simulates a selection request and return the initial and end cursor position when the selection worked. Otherwise both parameters will be -1. */
		void selectSubStringFromCursor(const String& TextCache, const Int32& CurPos, Int32& InitCur, Int32& EndCur );

		/** @return The cursor position inside the string */
		Vector2i getCursorPos( const String& TextCache, const Uint32& characterSize, bool bold, Float outlineThickness, const Uint32& Pos );

		virtual const Info& getInfo() const = 0;

		virtual const Glyph& getGlyph(Uint32 codePoint, unsigned int characterSize, bool bold, Float outlineThickness = 0) const = 0;

		virtual Float getKerning(Uint32 first, Uint32 second, unsigned int characterSize) const = 0;

		virtual Float getLineSpacing(unsigned int characterSize) const = 0;

		virtual Float getUnderlinePosition(unsigned int characterSize) const = 0;

		virtual Float getUnderlineThickness(unsigned int characterSize) const = 0;

		virtual Texture * getTexture(unsigned int characterSize) const = 0;
	protected:
		Uint32 						mType;
		std::string					mFontName;
		Uint32						mFontHash;
		Uint32 						mTexId;
		Uint32 						mHeight;
		Uint32 						mSize;
		Int32						mLineSkip;

		std::vector<GlyphData> 		mGlyphs;
		std::vector<TextureCoords> 	mTexCoords;

		Font( const Uint32& Type, const std::string& setName );
};

}}

#endif
