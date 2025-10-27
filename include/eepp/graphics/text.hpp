#ifndef EE_GRAPHICS_TEXT_HPP
#define EE_GRAPHICS_TEXT_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/texttransform.hpp>

#include <optional>

namespace EE { namespace Graphics {

class FontTrueType;

enum class CharacterAlignment : Uint32 { Left = 0, Center = 1, Right = 2 };

struct WhitespaceDisplayConfig {
	String::StringBaseType spaceDisplayCharacter{ 0 };
	String::StringBaseType tabDisplayCharacter{ 0 };
	CharacterAlignment tabAlign{ CharacterAlignment::Center };
	Color color;
	std::optional<Float> tabOffset;
};

struct ShapedGlyph {
	FontTrueType* font{ nullptr };
	Uint32 glyphIndex{ 0 };
	size_t stringIndex{ 0 };
	Vector2f position;
};

struct TextLayout {
	std::vector<ShapedGlyph> shapedGlyphs;
	std::vector<Float> linesWidth;
	Sizef size;
};

class EE_API TextLayouter {
  public:
	static TextLayout layout( const String& string, Font* font, const Uint32& fontSize,
							  const Uint32& style, const Uint32& tabWidth = 4,
							  const Float& outlineThickness = 0.f,
							  std::optional<Float> tabOffset = {}, Uint32 textDrawHints = 0 );

	static TextLayout layout( const String::View& string, Font* font, const Uint32& fontSize,
							  const Uint32& style, const Uint32& tabWidth = 4,
							  const Float& outlineThickness = 0.f,
							  std::optional<Float> tabOffset = {}, Uint32 textDrawHints = 0 );

  protected:
	template <typename StringType>
	static TextLayout layout( const StringType& string, Font* font, const Uint32& fontSize,
							  const Uint32& style, const Uint32& tabWidth = 4,
							  const Float& outlineThickness = 0.f,
							  std::optional<Float> tabOffset = {}, Uint32 textDrawHints = 0 );
};

// helper class that divides the string into lines and font runs.
class EE_API TextShapeRun {
  public:
	TextShapeRun( String::View str, FontTrueType* font, Uint32 characterSize, Uint32 style,
				  Float outlineThickness );

	String::View curRun() const;

	bool hasNext() const;

	std::size_t pos() const;

	void next();

	bool runIsNewLine() const;

	FontTrueType* font();

  protected:
	void findNextEnd();

	String::View mString;
	std::size_t mIndex{ 0 };
	std::size_t mLen{ 0 };
	Font* mFont{ nullptr };
	Uint32 mCharacterSize;
	Uint32 mStyle;
	Float mOutlineThickness;
	Font* mCurFont{ nullptr };
	Font* mStartFont{ nullptr };
	bool mIsNewLine{ false };
};

class EE_API Text {
  public:
	static bool TextShaperEnabled;
	static Uint32 GlobalInvalidationId;

	enum Style {
		Regular = 0,			///< Regular characters, no style
		Bold = 1 << 0,			///< Bold characters
		Italic = 1 << 1,		///< Italic characters
		Underlined = 1 << 2,	///< Underlined characters
		StrikeThrough = 1 << 3, ///< Strike through characters
		Shadow = 1 << 4			///< Draw a shadow below the text
	};

	static Float tabAdvance( Float spaceHorizontalAdvance, Uint32 tabLength,
							 std::optional<Float> tabOffset );

	static std::string styleFlagToString( const Uint32& flags );

	static Uint32 stringToStyleFlag( const std::string& str );

	static Float getTextWidth( Font* font, const Uint32& fontSize, const String& string,
							   const Uint32& style, const Uint32& tabWidth = 4,
							   const Float& outlineThickness = 0.f, Uint32 textDrawHints = 0,
							   std::optional<Float> tabOffset = {} );

	static Float getTextWidth( Font* font, const Uint32& fontSize, const String::View& string,
							   const Uint32& style, const Uint32& tabWidth = 4,
							   const Float& outlineThickness = 0.f, Uint32 textDrawHints = 0,
							   std::optional<Float> tabOffset = {} );

	static Float getTextWidth( const String& string, const FontStyleConfig& config,
							   const Uint32& tabWidth = 4, Uint32 textDrawHints = 0,
							   std::optional<Float> tabOffset = {} );

	static Float getTextWidth( const String::View& string, const FontStyleConfig& config,
							   const Uint32& tabWidth = 4, Uint32 textDrawHints = 0,
							   std::optional<Float> tabOffset = {} );

	static Sizef draw( const String& string, const Vector2f& pos, Font* font, Float fontSize,
					   const Color& fontColor, Uint32 style = 0, Float outlineThickness = 0.f,
					   const Color& outlineColor = Color::Black,
					   const Color& shadowColor = Color::Black,
					   const Vector2f& shadowOffset = { 1, 1 }, const Uint32& tabWidth = 4,
					   Uint32 textDrawHints = 0,
					   const WhitespaceDisplayConfig& whitespaceDisplayConfig = {} );

	static Sizef draw( const String& string, const Vector2f& pos, const FontStyleConfig& config,
					   const Uint32& tabWidth = 4, Uint32 textDrawHints = 0,
					   const WhitespaceDisplayConfig& whitespaceDisplayConfig = {} );

	static Sizef draw( const String::View& string, const Vector2f& pos, Font* font, Float fontSize,
					   const Color& fontColor, Uint32 style = 0, Float outlineThickness = 0.f,
					   const Color& outlineColor = Color::Black,
					   const Color& shadowColor = Color::Black,
					   const Vector2f& shadowOffset = { 1, 1 }, const Uint32& tabWidth = 4,
					   Uint32 textDrawHints = 0,
					   const WhitespaceDisplayConfig& whitespaceDisplayConfig = {} );

	static Sizef draw( const String::View& string, const Vector2f& pos,
					   const FontStyleConfig& config, const Uint32& tabWidth = 4,
					   Uint32 textDrawHints = 0,
					   const WhitespaceDisplayConfig& whitespaceDisplayConfig = {} );

	static void drawUnderline( const Vector2f& pos, Float width, Font* font, Float fontSize,
							   const Color& fontColor, const Uint32& style, Float outlineThickness,
							   const Color& outlineColor, const Color& shadowColor,
							   const Vector2f& shadowOffset );

	static void drawStrikeThrough( const Vector2f& pos, Float width, Font* font, Float fontSize,
								   const Color& fontColor, const Uint32& style,
								   Float outlineThickness, const Color& outlineColor,
								   const Color& shadowColor, const Vector2f& shadowOffset );

	static Int32 findCharacterFromPos( const Vector2i& pos, bool returnNearest, Font* font,
									   const Uint32& fontSize, const String& string,
									   const Uint32& style, const Uint32& tabWidth = 4,
									   const Float& outlineThickness = 0.f,
									   std::optional<Float> tabOffset = {} );

	static Vector2f findCharacterPos( std::size_t index, Font* font, const Uint32& fontSize,
									  const String& string, const Uint32& style,
									  const Uint32& tabWidth = 4,
									  const Float& outlineThickness = 0.f,
									  std::optional<Float> tabOffset = {},
									  bool allowNewLine = true );

	static std::size_t findLastCharPosWithinLength( Font* font, const Uint32& fontSize,
													const String& string, Float maxWidth,
													const Uint32& style, const Uint32& tabWidth = 4,
													const Float& outlineThickness = 0.f,
													std::optional<Float> tabOffset = {} );

	static std::size_t findLastCharPosWithinLength( Font* font, const Uint32& fontSize,
													const String::View& string, Float maxWidth,
													const Uint32& style, const Uint32& tabWidth = 4,
													const Float& outlineThickness = 0.f,
													std::optional<Float> tabOffset = {} );

	static std::size_t findLastCharPosWithinLength( const String& string, Float maxWidth,
													const FontStyleConfig& config,
													const Uint32& tabWidth = 4,
													std::optional<Float> tabOffset = {} );

	static std::size_t findLastCharPosWithinLength( const String::View& string, Float maxWidth,
													const FontStyleConfig& config,
													const Uint32& tabWidth = 4,
													std::optional<Float> tabOffset = {} );

	static bool wrapText( Font* font, const Uint32& fontSize, String& string, const Float& maxWidth,
						  const Uint32& style, const Uint32& tabWidth = 4,
						  const Float& outlineThickness = 0.f,
						  std::optional<Float> tabOffset = {} );

	static bool wrapText( String& string, const Float& maxWidth, const FontStyleConfig& config,
						  const Uint32& tabWidth = 4, std::optional<Float> tabOffset = {} );

	static Text* New();

	static Text* New( const String& string, Font* font,
					  unsigned int characterSize = PixelDensity::dpToPx( 12 ) );

	static Text* New( Font* font, unsigned int characterSize = PixelDensity::dpToPx( 12 ) );

	Text();

	Text( const String& string, Font* font,
		  unsigned int characterSize = PixelDensity::dpToPx( 12 ) );

	Text( Font* font, unsigned int characterSize = PixelDensity::dpToPx( 12 ) );

	/** Create a text from a font */
	void create( Graphics::Font* font, const String& text = "",
				 Color FontColor = Color( 255, 255, 255, 255 ),
				 Color FontShadowColor = Color( 0, 0, 0, 255 ),
				 Uint32 characterSize = PixelDensity::dpToPx( 12 ) );

	bool setString( const String::View& string );

	bool setString( const String& string );

	bool setString( String&& string );

	void setFont( Font* font );

	void setFontSize( unsigned int size );

	void setStyle( Uint32 style );

	void setColor( const Color& color );

	void setFillColor( const Color& color );

	void setFillColor( const Color& color, Uint32 from, Uint32 to );

	void setOutlineColor( const Color& color );

	void setOutlineThickness( Float thickness );

	void transformText( const TextTransform::Value& transform );

	const String& getString() const;

	String& getString();

	Font* getFont() const;

	unsigned int getCharacterSize() const;

	Uint32 getStyle() const;

	/** @see Set the alpha of each individual character.
	**	This doesn't break any custom color per-character set. */
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

	/** @return The line spacing */
	Float getLineSpacing();

	/** Draw the cached text on screen */
	void draw( const Float& X, const Float& Y, const Vector2f& scale = Vector2f::One,
			   const Float& rotation = 0, BlendMode effect = BlendMode::Alpha(),
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
	Uint32 getNumLines();

	void setStyleConfig( const FontStyleConfig& styleConfig );

	/** Finds the closest cursor position to the point position */
	Int32 findCharacterFromPos( const Vector2i& pos, bool returnNearest = true ) const;

	/** Simulates a selection request and return the initial and end cursor position when the
	 * selection worked. Otherwise both parameters will be -1. */
	void findWordFromCharacterIndex( Int32 characterIndex, Int32& initCur, Int32& endCur ) const;

	/** Shrink the String to a max width
	 * @param MaxWidth The maximum possible width
	 */
	void wrapText( const Uint32& maxWidth );

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

	const Vector2f& getShadowOffset() const;

	void setShadowOffset( const Vector2f& shadowOffset );

	bool hasSameFontStyleConfig( const FontStyleConfig& styleConfig );

	void setTabStops( bool enabled );

	bool hasTabStops() const { return mTabStops; }

	const FontStyleConfig& getFontStyleConfig() const { return mFontStyleConfig; }

  protected:
	struct VertexCoords {
		Vector2f texCoords;
		Vector2f position;
	};

	String mString; ///< String to display
	FontStyleConfig mFontStyleConfig;
	Color mBackgroundColor{ Color::Transparent };

	mutable Rectf mBounds; ///< Bounding rectangle of the text (in local coordinates)
	mutable bool mGeometryNeedUpdate{ false }; ///< Does the geometry need to be recomputed?
	mutable bool mCachedWidthNeedUpdate{ false };
	mutable bool mColorsNeedUpdate{ false };
	mutable bool mContainsColorEmoji{ false };
	bool mTabStops{ false };

	Float mCachedWidth{ 0 };
	Uint32 mAlign{ TEXT_ALIGN_LEFT };
	Uint32 mTabWidth{ 4 };
	Uint32 mInvalidationId{ 0 };

	std::vector<VertexCoords> mVertices;
	std::vector<Color> mColors;
	std::vector<VertexCoords> mOutlineVertices;
	std::vector<Color> mOutlineColors;
	std::vector<Float> mLinesWidth;

	void ensureGeometryUpdate();

	void ensureColorUpdate();

	/** Force to cache the width of the current text */
	void cacheWidth();

	static void addLine( std::vector<VertexCoords>& vertices, Float lineLength, Float lineTop,
						 Float offset, Float thickness, Float outlineThickness, Int32 centerDiffX );

	static void addGlyphQuad( std::vector<VertexCoords>& vertices, Vector2f position,
							  const EE::Graphics::Glyph& glyph, Float italic,
							  Float outlineThickness, Int32 centerDiffX );

	Uint32 getTotalVertices();

	/** Cache the with of the current text */
	void updateWidthCache();

	void draw( const Float& X, const Float& Y, const Vector2f& scale, const Float& rotation,
			   BlendMode effect, const OriginPoint& rotationCenter, const OriginPoint& scaleCenter,
			   const std::vector<Color>& colors, const std::vector<Color>& outlineColors,
			   const Color& backgroundColor );

	void onNewString();

	template <typename StringType>
	static Float getTextWidth( Font* font, const Uint32& fontSize, const StringType& string,
							   const Uint32& style, const Uint32& tabWidth = 4,
							   const Float& outlineThickness = 0.f, Uint32 textDrawHints = 0,
							   std::optional<Float> tabOffset = {} );

	template <typename StringType>
	static Sizef
	draw( const StringType& string, const Vector2f& pos, Font* font, Float fontSize,
		  const Color& fontColor, Uint32 style = 0, Float outlineThickness = 0.f,
		  const Color& outlineColor = Color::Black, const Color& shadowColor = Color::Black,
		  const Vector2f& shadowOffset = { 1, 1 }, const Uint32& tabWidth = 4,
		  Uint32 textDrawHints = 0, const WhitespaceDisplayConfig& whitespaceDisplayConfig = {} );

	template <typename StringType>
	static Sizef draw( const StringType& string, const Vector2f& pos, const FontStyleConfig& config,
					   const Uint32& tabWidth = 4, Uint32 textDrawHints = 0,
					   const WhitespaceDisplayConfig& whitespaceDisplayConfig = {} );

	template <typename StringType>
	static std::size_t findLastCharPosWithinLength( Font* font, const Uint32& fontSize,
													const StringType& string, Float width,
													const Uint32& style, const Uint32& tabWidth = 4,
													const Float& outlineThickness = 0.f,
													std::optional<Float> tabOffset = {} );

	template <typename StringType>
	static bool wrapText( Font* font, const Uint32& fontSize, StringType& string,
						  const Float& maxWidth, const Uint32& style, const Uint32& tabWidth = 4,
						  const Float& outlineThickness = 0.f,
						  std::optional<Float> tabOffset = {} );

	template <typename StringType>
	static bool wrapText( StringType& string, const Float& maxWidth, const FontStyleConfig& config,
						  const Uint32& tabWidth = 4, std::optional<Float> tabOffset = {} );
};

}} // namespace EE::Graphics

#endif
