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

		Font( const Uint32& Type, const std::string& setName );
};

}}

#endif
