#ifndef EE_GRAPHICSCFONT_H
#define EE_GRAPHICSCFONT_H

#include "base.hpp"
#include "fonthelper.hpp"
#include "ctexturefactory.hpp"
#include "ctextcache.hpp"

namespace EE { namespace Graphics {

#define FONT_TYPE_TTF (1)
#define FONT_TYPE_TEX (2)

/** @brief Font interface class. */
class EE_API cFont {
	public:
		cFont( const Uint32& Type, const std::string& Name );

		virtual ~cFont();

		/** Set a text to render
		 * @param Text The Text
		 * @param SupportNewLine If active will search for "\n" and back to a new line.
		 */
		void SetText( const std::wstring& Text );
		void SetText( const std::string& Text );

		/** @return The width of the string rendered */
		eeFloat GetTextWidth() const;

		/** @return Assign a new text and then returns his width */
		eeFloat GetTextWidth( const std::wstring& Text );

		/** @return The current text height */
		eeFloat GetTextHeight();

		/** @return The number of lines of the current text */
		virtual eeInt GetNumLines();

		/** @return The Font Color */
		const eeColorA& Color() const;

		/** Set the color of the string rendered */
		void Color(const eeColorA& Color);

		/** @return The Shadow Font Color */
		const eeColorA& ShadowColor() const;

		/** Set the shadow color of the string rendered */
		void ShadowColor(const eeColorA& Color);

		/** @return The current font size */
		Uint32 GetFontSize() const;

		/** @return The current font height */
		Uint32 GetFontHeight() const;

		/** @return The current text */
		std::wstring GetText();

		/** Set if the font will cache de text width and the number of lines ( default: true ). */
		void CacheData( bool Cache );

		/** @return If the font is caching the text data. */
		const bool& CacheData() const;

		/** @return The last text rendered or setted lines width */
		const std::vector<eeFloat>& GetLinesWidth() const;

		/** Draw a wstring on the screen
		* @param Text The text to draw
		* @param X The start x position
		* @param Y The start y position
		* @param Flags Set some flags to the rendering ( for text align )
		* @param Scale The string rendered scale
		* @param Angle The angle of the string rendered
		* @param Effect Set the Blend Mode ( default ALPHA_NORMAL )
		*/
		void Draw( const std::wstring& Text, const eeFloat& X, const eeFloat& Y, const Uint32& Flags = FONT_DRAW_LEFT, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0, const EE_PRE_BLEND_FUNC& Effect = ALPHA_NORMAL );

		/** Draw the string seted on the screen
		* @param X The start x position
		* @param Y The start y position
		* @param Flags Set some flags to the rendering ( for text align )
		* @param Scale The string rendered scale
		* @param Angle The angle of the string rendered
		* @param Effect Set the Blend Mode ( default ALPHA_NORMAL )
		*/
		void Draw( const eeFloat& X, const eeFloat& Y, const Uint32& Flags = FONT_DRAW_LEFT, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0, const EE_PRE_BLEND_FUNC& Effect = ALPHA_NORMAL );

		/** Draw a string on the screen
		* @param Text The text to draw
		* @param X The start x position
		* @param Y The start y position
		* @param Flags Set some flags to the rendering ( for text align )
		* @param Scale The string rendered scale
		* @param Angle The angle of the string rendered
		* @param Effect Set the Blend Mode ( default ALPHA_NORMAL )
		*/
		void Draw( const std::string& Text, const eeFloat& X, const eeFloat& Y, const Uint32& Flags = FONT_DRAW_LEFT, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0, const EE_PRE_BLEND_FUNC& Effect = ALPHA_NORMAL );

		/** Draw a string on the screen from a cached text
		* @param TextCache The cached text
		* @param X The start x position
		* @param Y The start y position
		* @param Flags Set some flags to the rendering ( for text align )
		* @param Scale The string rendered scale
		* @param Angle The angle of the string rendered
		* @param Effect Set the Blend Mode ( default ALPHA_NORMAL )
		*/
		void Draw( cTextCache& TextCache, const eeFloat& X, const eeFloat& Y, const Uint32& Flags = FONT_DRAW_LEFT, const eeFloat& Scale = 1.0f, const eeFloat& Angle = 0, const EE_PRE_BLEND_FUNC& Effect = ALPHA_NORMAL );

		/** Shrink the wstring to a max width
		* @param Str The string to shrink
		* @param MaxWidth The Max Width posible
		*/
		void ShrinkText( std::wstring& Str, const Uint32& MaxWidth );

		/** Shrink the string to a max width
		* @param Str The string to shrink
		* @param MaxWidth The Max Width posible
		*/
		void ShrinkText( std::string& Str, const Uint32& MaxWidth );

		/** Cache the with of the current text */
		void CacheWidth( const std::wstring& Text, std::vector<eeFloat>& LinesWidth, eeFloat& CachedWidth, eeInt& NumLines );

		/** @return The font texture id */
		const Uint32& GetTexId() const;

		const Uint32& Type() const;

		const std::string& 		Name() const;

		void						Name( const std::string& name );

		const Uint32&				Id();
	protected:
		Uint32 						mType;
		std::string					mFontName;
		Uint32						mFontHash;

		std::wstring 				mText;
		bool 						mCacheData;
		eeColorA 					mColor;
		eeColorA 					mShadowColor;
		eeInt 						mNumLines;
		bool 						mVerticalDraw;
		Uint32 						mTexId;
		eeFloat 					mCachedWidth;
		Uint32 						mHeight;
		Uint32 						mSize;

		std::vector<eeFloat> 		mLinesWidth;
		std::vector<eeGlyph> 		mGlyphs;
		std::vector<eeTexCoords> 	mTexCoords;

		std::vector<eeVertexCoords> mRenderCoords;
		std::vector<eeColorA> 		mColors;

		void CacheWidth();
		void CacheNumLines();
		void SubDraw( const std::wstring& Text, const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, const bool& Cached, const EE_PRE_BLEND_FUNC& Effect );
};

}}

#endif
