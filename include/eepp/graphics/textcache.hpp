#ifndef EE_GRAPHICSCTEXTCACHE_H
#define EE_GRAPHICSCTEXTCACHE_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/fonthelper.hpp>

namespace EE { namespace Graphics {

class Font;

/** @brief Caches text for a fast font rendering. */
class EE_API TextCache {
	public:
		/** Create a text from a font */
		TextCache( Graphics::Font * font, const String& text = "", ColorA FontColor = ColorA(255,255,255,255), ColorA FontShadowColor = ColorA(0,0,0,255) );

		TextCache();

		~TextCache();

		/** Create a text from a font */
		void create( Graphics::Font * font, const String& text = "", ColorA FontColor = ColorA(255,255,255,255), ColorA FontShadowColor = ColorA(0,0,0,255) );

		/** @return The font used for the text cache */
		Graphics::Font * getFont() const;

		/** Change the font used for the text cache */
		void setFont( Graphics::Font * font );

		/** @return The text cached */
		String& getText();

		/** Set the text to be cached */
		void setText( const String& text );

		/** @return The cached text width */
		Float getTextWidth();

		/** @return The cached text height */
		Float getTextHeight();

		/** @return Every cached text line width */
		const std::vector<Float>& getLinesWidth();

		/** @return The text colors cached */
		std::vector<ColorA>& getColors();

		/** Draw the cached text on screen */
		void draw( const Float& X, const Float& Y, const Vector2f& Scale = Vector2f::One, const Float& Angle = 0, EE_BLEND_MODE Effect = ALPHA_NORMAL );

		/** @return The Font Color */
		const ColorA& getColor() const;

		/** Set the color of the string rendered */
		void setColor(const ColorA& color);

		/** @see Set the alpha of each individual character.
		**	This doesn't break any custom color per-character setted. */
		void setAlpha( const Uint8& alpha );

		/** Set the color of the substring
		* @param color The color
		* @param from The first char to change the color
		* @param to The last char to change the color
		*/
		void setColor(const ColorA& color, Uint32 from, Uint32 to );

		/** @return The Shadow Font Color */
		const ColorA& getShadowColor() const;

		/** Set the shadow color of the string rendered */
		void setShadowColor(const ColorA& color);

		/** @return The number of lines that the cached text contains */
		const int& getNumLines() const;

		/** Set the font draw flags */
		void setFlags( const Uint32& flags );

		/** @return The font draw flags */
		const Uint32& getFlags() const;

		/** Force to cache the width of the current text */
		void cacheWidth();
	protected:
		friend class Font;

		String mText;
		Graphics::Font * mFont;

		Float mCachedWidth;
		int mNumLines;
		int mLargestLineCharCount;

		ColorA mFontColor;
		ColorA mFontShadowColor;

		Uint32 mFlags;
		Uint32 mVertexNumCached;

		bool mCachedCoords;

		std::vector<Float> mLinesWidth;
		std::vector<VertexCoords>	mRenderCoords;
		std::vector<ColorA> mColors;

		void cacheVerts();

		void updateCoords();
};

}}

#endif
