#ifndef EE_GRAPHICS_RICHTEXT_HPP
#define EE_GRAPHICS_RICHTEXT_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/text.hpp>
#include <memory>
#include <variant>
#include <vector>

namespace EE { namespace Graphics {

/**
 * @brief A drawable class that renders rich text with multiple styles and spans.
 *
 * RichText allows rendering text with different fonts, sizes, colors, and styles mixed together.
 * It supports word wrapping and alignment.
 */
class EE_API RichText : public Drawable {
  public:
	/** @return A new instance of RichText. */
	static RichText* New();

	/** @brief Default constructor. */
	RichText();

	/** @brief Destructor. */
	~RichText();

	/**
	 * @brief Adds a text span with a specific style configuration.
	 * @param text The text content.
	 * @param style The font style configuration to apply.
	 */
	void addSpan( const String& text, const FontStyleConfig& style );

	/**
	 * @brief Adds a text span with individual style parameters.
	 * @param text The text content.
	 * @param font The font to use (optional, uses default if null).
	 * @param characterSize The character size (optional, uses default if 0).
	 * @param color The text color (optional, uses default if White).
	 * @param style The text style (optional, uses default if Regular).
	 */
	void addSpan( const String& text, Font* font = nullptr, Uint32 characterSize = 0,
				  Color color = Color::White, Uint32 style = Text::Regular,
				  Color backgroundColor = Color::Transparent );

	/** @brief Clears all text spans. */
	void clear();

	/** @brief Sets the default font style configuration used for new spans if not specified. */
	void setFontStyleConfig( const FontStyleConfig& styleConfig );

	/** @return The default font style configuration. */
	FontStyleConfig& getFontStyleConfig() { return mDefaultStyle; }

	/** @return The default font style configuration. */
	const FontStyleConfig& getFontStyleConfig() const { return mDefaultStyle; }

	/** @brief Sets the text alignment (Left, Center, Right). */
	void setAlign( Uint32 align );

	/** @return The text alignment. */
	Uint32 getAlign() const { return mAlign; }

	/** @brief Sets the maximum width for wrapping. If 0, wrapping is disabled. */
	void setMaxWidth( Float width );

	/** @return The maximum width for wrapping. */
	Float getMaxWidth() const { return mMaxWidth; }

	/** @return The minimum intrinsic width of the text block. */
	Float getMinIntrinsicWidth();

	/** @return The maximum intrinsic width of the text block. */
	Float getMaxIntrinsicWidth();

	enum class BlockType { Text, Drawable, CustomSize };

	struct CustomBlock {
		Sizef size;
		bool isBlock{ false };
	};

	using Block = std::variant<std::shared_ptr<Text>, std::shared_ptr<Drawable>, CustomBlock>;

	/**
	 * @brief Adds a drawable (e.g., an image) into the text flow.
	 * @param drawable The drawable to add.
	 */
	void addDrawable( std::shared_ptr<Drawable> drawable );

	/**
	 * @brief Adds a custom size spacer into the text flow.
	 * @param size The physical dimensions of the spacer.
	 * @param isBlock Whether this spacer acts as a block-level element.
	 */
	void addCustomSize( const Sizef& size, bool isBlock = false );

	/** @return The list of blocks. */
	const std::vector<Block>& getBlocks() { return mBlocks; }

	virtual void draw( const Float& X, const Float& Y, const Vector2f& scale = Vector2f::One,
					   const Float& rotation = 0, BlendMode effect = BlendMode::Alpha(),
					   const OriginPoint& rotationCenter = OriginPoint::OriginCenter,
					   const OriginPoint& scaleCenter = OriginPoint::OriginCenter );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	virtual bool isStateful() { return false; }

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	/** @brief Invalidates the layout, forcing a recalculation on the next update. */
	void invalidate();

	/** @brief Structure representing a rendered span within a line. */
	struct RenderSpan {
		Block block;
		Vector2f position; // Local position relative to RichText origin
		Sizef size;
		Int64 startCharIndex{ 0 };
		Int64 endCharIndex{ 0 };
	};

	/** @brief Structure representing a rendered paragraph (line). */
	struct RenderParagraph {
		std::vector<RenderSpan> spans;
		Float y{ 0 };
		Float height{ 0 };
		Float maxAscent{ 0 };
		Float width{ 0 };
	};

	/** @return The list of rendered lines. */
	const std::vector<RenderParagraph>& getLines() const { return mLines; }

	/** @brief Sets the text selection range. */
	void setSelection( TextSelectionRange range );

	/** @return The current text selection range. */
	TextSelectionRange getSelection() const { return mSelection; }

	/** @brief Sets the selection text color. */
	void setSelectionColor( const Color& color );

	/** @return The selection text color. */
	const Color& getSelectionColor() const { return mSelectionColor; }

	/** @brief Sets the selection background color. */
	void setSelectionBackColor( const Color& color );

	/** @return The selection background color. */
	const Color& getSelectionBackColor() const { return mSelectionBackColor; }

	/** @return The total number of characters in the RichText. */
	Int64 getCharacterCount() const;

	/** @return The character index at the given position. */
	Int64 findCharacterFromPos( const Vector2i& pos ) const;

	/** @return The position of the character at the given index. */
	Vector2f findCharacterPos( Int64 index ) const;

	/** @return A list of rectangles that cover the selection. */
	std::vector<Rectf> getSelectionRects() const;

	/** @return The current selection as a string. */
	String getSelectionString() const;

	/** Tries to update the layout if has been invalidated. This is automatically called before
	 * draw. */
	void updateLayout();

	/** Invalidates the current layout */
	void invalidateLayout();

  protected:
	std::vector<Block> mBlocks;
	std::vector<RenderParagraph> mLines;
	FontStyleConfig mDefaultStyle;
	TextSelectionRange mSelection{ 0, 0 };
	Color mSelectionColor{ Color::White };
	Color mSelectionBackColor{ 0, 0, 255, 150 };
	Uint32 mAlign{ TEXT_ALIGN_LEFT };
	Float mMaxWidth{ 0.f };
	Sizef mSize;
	Int64 mTotalCharacterCount{ 0 };
	bool mNeedsLayoutUpdate{ true };
};

}} // namespace EE::Graphics

#endif
