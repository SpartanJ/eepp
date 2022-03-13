#ifndef EE_GRAPHICS_TEXT_HPP
#define EE_GRAPHICS_TEXT_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>

namespace EE { namespace Graphics {

class EE_API Text {
  public:
	enum Style {
		Regular = 0,			///< Regular characters, no style
		Bold = 1 << 0,			///< Bold characters
		Italic = 1 << 1,		///< Italic characters
		Underlined = 1 << 2,	///< Underlined characters
		StrikeThrough = 1 << 3, ///< Strike through characters
		Shadow = 1 << 4			///< Draw a shadow below the text
	};

	static std::string styleFlagToString( const Uint32& flags );

	static Uint32 stringToStyleFlag( const std::string& str );

	static Text* New();

	static Text* New( const String& string, Font* font, unsigned int characterSize = 12 );

	static Text* New( Font* font, unsigned int characterSize = 12 );

	Text();

	Text( const String& string, Font* font, unsigned int characterSize = 12 );

	Text( Font* font, unsigned int characterSize = 12 );

	/** Create a text from a font */
	void create( Graphics::Font* font, const String& text = "",
				 Color FontColor = Color( 255, 255, 255, 255 ),
				 Color FontShadowColor = Color( 0, 0, 0, 255 ), Uint32 characterSize = 12 );

	void setString( const String& string );

	void setFont( Font* font );

	void setFontSize( unsigned int size );

	void setStyle( Uint32 style );

	void setColor( const Color& color );

	void setFillColor( const Color& color );

	void setFillColor( const Color& color, Uint32 from, Uint32 to );

	void setOutlineColor( const Color& color );

	void setOutlineThickness( Float thickness );

	String& getString();

	Font* getFont() const;

	unsigned int getCharacterSize() const;

	unsigned int getCharacterSizePx() const;

	const Uint32& getFontHeight() const;

	Uint32 getStyle() const;

	/** @see Set the alpha of each individual character.
	**	This doesn't break any custom color per-character setted. */
	void setAlpha( const Uint8& alpha );

	const Color& getFillColor() const;

	const Color& getColor() const;

	const Color& getOutlineColor() const;

	Float getOutlineThickness() const;

	Vector2f findCharacterPos( std::size_t index ) const;

	/** @return The current text local bounds. */
	Rectf getLocalBounds();

	/** @return The cached text width */
	Float getTextWidth();

	/** @return The cached text height */
	Float getTextHeight();

	/** @return The line espacing */
	Float getLineSpacing();

	/** Draw the cached text on screen */
	void draw( const Float& X, const Float& Y, const Vector2f& scale = Vector2f::One,
			   const Float& rotation = 0, BlendMode effect = BlendAlpha,
			   const OriginPoint& rotationCenter = OriginPoint::OriginCenter,
			   const OriginPoint& scaleCenter = OriginPoint::OriginCenter );

	/** @return The Shadow Font Color */
	const Color& getShadowColor() const;

	/** Set the shadow color of the string rendered */
	void setShadowColor( const Color& color );

	/** @return Every cached text line width */
	const std::vector<Float>& getLinesWidth();

	/** Set the text draw align */
	void setAlign( const Uint32& align );

	/** @return The text align */
	const Uint32& getAlign() const;

	/** @return The number of lines that the cached text contains */
	const int& getNumLines();

	void setStyleConfig( const FontStyleConfig& styleConfig );

	/** Finds the closest cursor position to the point position */
	Int32 findCharacterFromPos( const Vector2i& pos, bool returnNearest = true ) const;

	/** Simulates a selection request and return the initial and end cursor position when the
	 * selection worked. Otherwise both parameters will be -1. */
	void findWordFromCharacterIndex( Int32 characterIndex, Int32& InitCur, Int32& EndCur ) const;

	/** Shrink the String to a max width
	 * @param MaxWidth The maximum possible width
	 */
	void shrinkText( const Uint32& maxWidth );

	/** Invalidates the color cache */
	void invalidateColors();

	/** Invalidates the text cache */
	void invalidate();

	/** Sets the tab character width. */
	void setTabWidth( const Uint32& tabWidth );

	/** @return The tab character width */
	const Uint32& getTabWidth() const;

	/** @return The text background color */
	Color getBackgroundColor() const;

	/** Sets text background color. */
	void setBackgroundColor( const Color& backgroundColor );

	/** @return True if the text width cache is disabled. */
	bool getDisableCacheWidth() const;

	/** The text width is cached every time the geometry of the text is updated. It's possible to
	 * disable this to improve performance in very specific scenarios. */
	void setDisableCacheWidth( bool newDisableCacheWidth );

  protected:
	struct VertexCoords {
		Vector2f texCoords;
		Vector2f position;
	};

	String mString;			///< String to display
	Font* mFont;			///< FontTrueType used to display the string
	unsigned int mFontSize; ///< Base size of characters, in pixels
	unsigned int mRealFontSize;
	Uint32 mStyle;		 ///< Text style (see Style enum)
	Color mFillColor;	 ///< Text fill color
	Color mOutlineColor; ///< Text outline color
	Color mBackgroundColor{ Color::Transparent };
	Float mOutlineThickness; ///< Thickness of the text's outline

	mutable Rectf mBounds;			  ///< Bounding rectangle of the text (in local coordinates)
	mutable bool mGeometryNeedUpdate; ///< Does the geometry need to be recomputed?
	mutable bool mCachedWidthNeedUpdate;
	mutable bool mColorsNeedUpdate;
	mutable bool mContainsColorEmoji{ false };
	bool mDisableCacheWidth{ false };

	Float mCachedWidth;
	int mNumLines;
	int mLargestLineCharCount;
	Color mFontShadowColor;
	Uint32 mAlign;
	Uint32 mFontHeight;
	Uint32 mTabWidth;

	std::vector<VertexCoords> mVertices;
	std::vector<Rectf> mGlyphCache;
	std::vector<Color> mColors;

	std::vector<VertexCoords> mOutlineVertices;
	std::vector<Color> mOutlineColors;
	std::vector<Float> mLinesWidth;
	std::vector<Uint32> mLinesStartIndex;

	void ensureGeometryUpdate();

	void ensureColorUpdate();

	/** Force to cache the width of the current text */
	void cacheWidth();

	static void addLine( std::vector<VertexCoords>& vertice, Float lineLength, Float lineTop,
						 Float offset, Float thickness, Float outlineThickness, Int32 centerDiffX );

	static void addGlyphQuad( std::vector<VertexCoords>& vertices, Vector2f position,
							  const EE::Graphics::Glyph& glyph, Float italic,
							  Float outlineThickness, Int32 centerDiffX );

	Uint32 getTotalVertices();

	/** Cache the with of the current text */
	void getWidthInfo();
};

}} // namespace EE::Graphics

#endif
