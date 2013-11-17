#ifndef EE_GRAPHICSCTEXTCACHE_H
#define EE_GRAPHICSCTEXTCACHE_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/fonthelper.hpp>

namespace EE { namespace Graphics {

class cFont;

/** @brief Caches text for a fast font rendering. */
class EE_API cTextCache {
	public:
		/** Create a text from a font */
		cTextCache( cFont * font, const String& text = "", eeColorA FontColor = eeColorA(255,255,255,255), eeColorA FontShadowColor = eeColorA(0,0,0,255) );

		cTextCache();

		~cTextCache();

		/** Create a text from a font */
		void Create( cFont * font, const String& text = "", eeColorA FontColor = eeColorA(255,255,255,255), eeColorA FontShadowColor = eeColorA(0,0,0,255) );

		/** @return The font used for the text cache */
		cFont * Font() const;

		/** Change the font used for the text cache */
		void Font( cFont * font );

		/** @return The text cached */
		String& Text();

		/** Set the text to be cached */
		void Text( const String& text );

		/** @return The cached text width */
		eeFloat GetTextWidth();

		/** @return The cached text height */
		eeFloat GetTextHeight();

		/** @return Every cached text line width */
		const std::vector<eeFloat>& LinesWidth();

		/** @return The vertex coordinates cached */
		std::vector<eeVertexCoords>& VertextCoords();

		/** @return The text colors cached */
		std::vector<eeColorA>& Colors();

		/** Draw the cached text on screen */
		void Draw( const eeFloat& X, const eeFloat& Y, const eeVector2f& Scale = eeVector2f::One, const eeFloat& Angle = 0, EE_BLEND_MODE Effect = ALPHA_NORMAL );

		/** @return The Font Color */
		const eeColorA& Color() const;

		/** Set the color of the string rendered */
		void Color(const eeColorA& color);

		/** @see Set the alpha of each individual character.
		**	This doesn't break any custom color per-character setted. */
		void Alpha( const Uint8& alpha );

		/** Set the color of the substring
		* @param color The color
		* @param from The first char to change the color
		* @param to The last char to change the color
		*/
		void Color(const eeColorA& color, Uint32 from, Uint32 to );

		/** @return The Shadow Font Color */
		const eeColorA& ShadowColor() const;

		/** Set the shadow color of the string rendered */
		void ShadowColor(const eeColorA& color);

		/** @return The number of lines that the cached text contains */
		const eeInt& GetNumLines() const;

		/** Set the font draw flags */
		void Flags( const Uint32& flags );

		/** @return The font draw flags */
		const Uint32& Flags() const;

		/** Force to cache the width of the current text */
		void Cache();
	protected:
		friend class cFont;

		String						mText;
		cFont * 					mFont;

		eeFloat 					mCachedWidth;
		eeInt 						mNumLines;
		eeInt						mLargestLineCharCount;

		eeColorA					mFontColor;
		eeColorA					mFontShadowColor;

		Uint32						mFlags;
		Uint32						mVertexNumCached;

		bool						mCachedCoords;

		std::vector<eeFloat> 		mLinesWidth;
		std::vector<eeVertexCoords>	mRenderCoords;
		std::vector<eeColorA>		mColors;

		void UpdateCoords();

		const bool& CachedCoords() const;

		void CachedCoords( const bool& cached );
		
		const eeUint& CachedVerts() const;

		void CachedVerts( const eeUint& num );
};

}}

#endif
