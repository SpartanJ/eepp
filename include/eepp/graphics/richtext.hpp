#ifndef EE_GRAPHICS_RICHTEXT_HPP
#define EE_GRAPHICS_RICHTEXT_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/text.hpp>
#include <memory>
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
				  Color color = Color::White, Uint32 style = Text::Regular );

	/** @brief Clears all text spans. */
	void clear();

	/** @brief Sets the default font style configuration used for new spans if not specified. */
	void setFontStyleConfig( const FontStyleConfig& styleConfig );

	/** @return The default font style configuration. */
	FontStyleConfig& getFontStyleConfig() { return mDefaultStyle; }

	/** @brief Sets the text alignment (Left, Center, Right). */
	void setAlign( Uint32 align );

	/** @brief Sets the maximum width for wrapping. If 0, wrapping is disabled. */
	void setMaxWidth( Float width );

	/** @return The maximum width for wrapping. */
	Float getMaxWidth() const { return mMaxWidth; }

	/** @return The list of text spans. */
	std::vector<std::shared_ptr<Text>>& getSpans() { return mSpans; }

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
		std::shared_ptr<Text> text;
		Vector2f position; // Local position relative to RichText origin
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

  protected:
	std::vector<std::shared_ptr<Text>> mSpans;
	std::vector<RenderParagraph> mLines;
	FontStyleConfig mDefaultStyle;
	Uint32 mAlign{ TEXT_ALIGN_LEFT };
	Float mMaxWidth{ 0.f };
	Sizef mSize;
	bool mNeedsLayoutUpdate{ true };

	void updateLayout();
};

}} // namespace EE::Graphics

#endif
