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
		Graphics::Font * font() const;

		/** Change the font used for the text cache */
		void font( Graphics::Font * font );

		/** @return The text cached */
		String& text();

		/** Set the text to be cached */
		void text( const String& text );

		/** @return The cached text width */
		Float getTextWidth();

		/** @return The cached text height */
		Float getTextHeight();

		/** @return Every cached text line width */
		const std::vector<Float>& linesWidth();

		/** @return The vertex coordinates cached */
		std::vector<eeVertexCoords>& vertextCoords();

		/** @return The text colors cached */
		std::vector<ColorA>& colors();

		/** Draw the cached text on screen */
		void draw( const Float& X, const Float& Y, const Vector2f& Scale = Vector2f::One, const Float& Angle = 0, EE_BLEND_MODE Effect = ALPHA_NORMAL );

		/** @return The Font Color */
		const ColorA& color() const;

		/** Set the color of the string rendered */
		void color(const ColorA& color);

		/** @see Set the alpha of each individual character.
		**	This doesn't break any custom color per-character setted. */
		void alpha( const Uint8& alpha );

		/** Set the color of the substring
		* @param color The color
		* @param from The first char to change the color
		* @param to The last char to change the color
		*/
		void color(const ColorA& color, Uint32 from, Uint32 to );

		/** @return The Shadow Font Color */
		const ColorA& shadowColor() const;

		/** Set the shadow color of the string rendered */
		void shadowColor(const ColorA& color);

		/** @return The number of lines that the cached text contains */
		const int& getNumLines() const;

		/** Set the font draw flags */
		void flags( const Uint32& flags );

		/** @return The font draw flags */
		const Uint32& flags() const;

		/** Force to cache the width of the current text */
		void cache();
	protected:
		friend class Font;

		String						mText;
		Graphics::Font * 					mFont;

		Float 					mCachedWidth;
		int 						mNumLines;
		int						mLargestLineCharCount;

		ColorA					mFontColor;
		ColorA					mFontShadowColor;

		Uint32						mFlags;
		Uint32						mVertexNumCached;

		bool						mCachedCoords;

		std::vector<Float> 		mLinesWidth;
		std::vector<eeVertexCoords>	mRenderCoords;
		std::vector<ColorA>		mColors;

		void updateCoords();

		const bool& cachedCoords() const;

		void cachedCoords( const bool& cached );
		
		const unsigned int& cachedVerts() const;

		void cachedVerts( const unsigned int& num );
};

}}

#endif
