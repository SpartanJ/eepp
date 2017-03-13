#ifndef EE_GRAPHICS_TEXT_HPP
#define EE_GRAPHICS_TEXT_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fonthelper.hpp>

namespace EE { namespace Graphics {

class EE_API Text {
	public:
		enum Style
		{
			Regular	   		= 0,	  ///< Regular characters, no style
			Bold			= 1 << 0, ///< Bold characters
			Italic			= 1 << 1, ///< Italic characters
			Underlined		= 1 << 2, ///< Underlined characters
			StrikeThrough	= 1 << 3  ///< Strike through characters
		};

		Text();

		Text(const String& string, Font * font, unsigned int characterSize = 30);

		Text(Font * font, unsigned int characterSize = 30);

		/** Create a text from a font */
		void create(Graphics::Font * font, const String& text = "", ColorA FontColor = ColorA(255,255,255,255), ColorA FontShadowColor = ColorA(0,0,0,255) , Uint32 characterSize = 12);

		void setString(const String& string);

		void setFont(Font * font);

		void setCharacterSize(unsigned int size);

		void setStyle(Uint32 style);

		void setColor(const ColorA& color);

		void setFillColor(const ColorA& color);

		void setOutlineColor(const ColorA& color);

		void setOutlineThickness(Float thickness);

		String& getString();

		Font * getFont() const;

		unsigned int getCharacterSize() const;

		unsigned int getCharacterSizePx() const;

		const Uint32& getFontHeight() const;

		Uint32 getStyle() const;

		/** @see Set the alpha of each individual character.
		**	This doesn't break any custom color per-character setted. */
		void setAlpha( const Uint8& alpha );

		const ColorA& getFillColor() const;

		const ColorA& getColor() const;

		const ColorA& getOutlineColor() const;

		Float getOutlineThickness() const;

		Vector2f findCharacterPos(std::size_t index) const;

		Rectf getLocalBounds();
		
		/** @return The cached text width */
		Float getTextWidth();

		/** @return The cached text height */
		Float getTextHeight();

		/** Draw the cached text on screen */
		void draw( const Float& X, const Float& Y, const Vector2f& Scale = Vector2f::One, const Float& Angle = 0, EE_BLEND_MODE Effect = ALPHA_NORMAL );

		/** @return The Shadow Font Color */
		const ColorA& getShadowColor() const;

		/** Set the shadow color of the string rendered */
		void setShadowColor(const ColorA& color);

		/** @return Every cached text line width */
		const std::vector<Float>& getLinesWidth();

		/** Set the font draw flags */
		void setFlags( const Uint32& flags );

		/** @return The font draw flags */
		const Uint32& getFlags() const;

		/** @return The number of lines that the cached text contains */
		const int& getNumLines() const;

		/** Force to cache the width of the current text */
		void cacheWidth();
	protected:
		void ensureGeometryUpdate();

		String				mString;			 ///< String to display
		Font *				mFont;			   ///< FontTrueType used to display the string
		unsigned int		mCharacterSize;	  ///< Base size of characters, in pixels
		unsigned int		mRealCharacterSize;
		Uint32				mStyle;			  ///< Text style (see Style enum)
		ColorA				mFillColor;		  ///< Text fill color
		ColorA				mOutlineColor;	   ///< Text outline color
		Float				mOutlineThickness;   ///< Thickness of the text's outline
		Sizei				mTextureSize;

		mutable Rectf   	mBounds;			 ///< Bounding rectangle of the text (in local coordinates)
		mutable bool		mGeometryNeedUpdate; ///< Does the geometry need to be recomputed?

		Float				mCachedWidth;
		int					mNumLines;
		int					mLargestLineCharCount;
		ColorA				mFontShadowColor;
		Uint32				mFlags;
		Uint32				mFontHeight;

		std::vector<VertexCoords>	mVertices;
		std::vector<ColorA> mColors;

		std::vector<VertexCoords>	mOutlineVertices;
		std::vector<ColorA> mOutlineColors;
		std::vector<Float> mLinesWidth;
};

}}


#endif
