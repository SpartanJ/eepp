#ifndef EE_GRAPHICSCTEXTCACHE_H
#define EE_GRAPHICSCTEXTCACHE_H

#include "base.hpp"
#include "fonthelper.hpp"

namespace EE { namespace Graphics {

class cFont;

/** @brief Cached text for a fast font rendering. */
class cTextCache {
	public:
		cTextCache( cFont * font, const std::wstring& text = L"", eeColorA FontColor = eeColorA(0xFFFFFFFF), eeColorA ShadowColor = eeColorA(0xFF000000) );

		cTextCache();

		~cTextCache();

		cFont * Font() const;

		void Font( cFont * font );

		std::wstring& Text();

		void Text( const std::wstring& text );

		void Text( const std::string& text );

		eeFloat GetTextWidth();

		eeFloat GetTextHeight();

		const std::vector<eeFloat>& LinesWidth();

		std::vector<eeVertexCoords>& VertextCoords();

		std::vector<eeColorA>& Colors();

		void Draw( const eeFloat& X, const eeFloat& Y, const Uint32& Flags = 0, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0, const EE_RENDERALPHAS& Effect = ALPHA_NORMAL );

		void Cache();

		/** @return The Font Color */
		const eeColorA& Color() const;

		/** Set the color of the string rendered */
		void Color(const eeColorA& Color);

		/** @return The Shadow Font Color */
		const eeColorA& ShadowColor() const;

		/** Set the shadow color of the string rendered */
		void ShadowColor(const eeColorA& Color);
	protected:
		std::wstring 				mText;
		cFont * 					mFont;
		std::vector<eeFloat> 		mLinesWidth;
		eeFloat 					mCachedWidth;
		eeInt 						mNumLines;

		eeColorA					mFontColor;
		eeColorA					mFontShadowColor;

		std::vector<eeVertexCoords>	mRenderCoords;
		std::vector<eeColorA>		mColors;

		void UpdateCoords();
};

}}

#endif
