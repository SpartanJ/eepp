#ifndef EE_GRAPHICS_RICHTEXT_HPP
#define EE_GRAPHICS_RICHTEXT_HPP

#include <eepp/core/containers.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/text.hpp>
#include <memory>
#include <utility>
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
	enum class InlineFloat { None, Left, Right };

	enum class InlineClear { None, Left, Right, Both };

	enum class WhiteSpaceWrapMode { Normal, Preserve, BreakSpaces };

	struct FloatExclusion {
		Rectf rect;
		InlineFloat type{ InlineFloat::None };

		bool operator==( const FloatExclusion& other ) const {
			return rect == other.rect && type == other.type;
		}

		bool operator!=( const FloatExclusion& other ) const { return !( *this == other ); }
	};

	enum class BaselineAlignment {
		Baseline,
		Sub,
		Super,
		TextTop,
		TextBottom,
		Middle,
		Top,
		Bottom,
		Length,
		Percentage,
		Auto
	};

	struct BaselineAlignValue {
		BaselineAlignment type{ BaselineAlignment::Baseline };
		Float value{ 0.f };

		BaselineAlignValue( BaselineAlignment type = BaselineAlignment::Baseline,
							Float value = 0.f ) :
			type( type ), value( value ) {}

		bool operator==( const BaselineAlignValue& other ) const {
			return type == other.type && value == other.value;
		}

		bool operator!=( const BaselineAlignValue& other ) const { return !( *this == other ); }
	};

	enum class InlineSourceType { None, TextNode, Widget };

	struct InlineSource {
		InlineSourceType type{ InlineSourceType::None };
		void* ptr{ nullptr };

		InlineSource( InlineSourceType type = InlineSourceType::None, void* ptr = nullptr ) :
			type( type ), ptr( ptr ) {}

		bool operator==( const InlineSource& other ) const {
			return type == other.type && ptr == other.ptr;
		}

		bool operator!=( const InlineSource& other ) const { return !( *this == other ); }
	};

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

	void addSpan( const String::View& text, const FontStyleConfig& style );

	void addSpan( String&& text, const FontStyleConfig& style );

	void addSpan( const String& text, const FontStyleConfig& style, const Rectf& margin,
				  const Rectf& padding, Float lineHeight = 0,
				  const BaselineAlignValue& baselineAlign = {}, InlineSource source = {} );

	void addSpan( const String::View& text, const FontStyleConfig& style, const Rectf& margin,
				  const Rectf& padding, Float lineHeight = 0,
				  const BaselineAlignValue& baselineAlign = {}, InlineSource source = {} );

	void addSpan( String&& text, const FontStyleConfig& style, const Rectf& margin,
				  const Rectf& padding, Float lineHeight = 0,
				  const BaselineAlignValue& baselineAlign = {}, InlineSource source = {} );

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

	/** @brief Enables or disables soft wrapping when max width is set. */
	void setLineWrap( bool lineWrap );

	/** @return Whether soft wrapping is enabled. */
	bool getLineWrap() const { return mLineWrap; }

	void setWhiteSpaceWrapMode( WhiteSpaceWrapMode mode );

	WhiteSpaceWrapMode getWhiteSpaceWrapMode() const { return mWhiteSpaceWrapMode; }

	void setTabWidth( Uint32 tabWidth );

	Uint32 getTabWidth() const { return mTabWidth; }

	bool setExternalFloatExclusions( const std::vector<FloatExclusion>& exclusions );

	const std::vector<FloatExclusion>& getExternalFloatExclusions() const {
		return mExternalFloatExclusions;
	}

	/** @return The minimum intrinsic width of the text block. */
	Float getMinIntrinsicWidth();

	/** @return The maximum intrinsic width of the text block. */
	Float getMaxIntrinsicWidth();

	/**
	 * @brief Adds a drawable (e.g., an image) into the text flow.
	 * @param drawable The drawable to add.
	 */
	void addDrawable( std::shared_ptr<Drawable> drawable );

	/**
	 * @brief Adds a custom size spacer into the text flow.
	 * @param size The physical dimensions of the spacer.
	 */
	void addCustomSize( const Sizef& size, InlineFloat floatType = InlineFloat::None,
						InlineClear clearType = InlineClear::None, Float baseline = -1.f,
						const BaselineAlignValue& baselineAlign = {}, InlineSource source = {},
						bool isBlock = false, bool isBlockFormattingContext = false );

	/** @brief Adds a virtual line break that is not associated with a DOM text character. */
	void addLineBreak();

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
		enum class Type { Text, Drawable, AtomicBox };

		using InlinePath = SmallVector<size_t, 4>;

		Type type{ Type::Text };
		std::shared_ptr<Text> text;
		std::shared_ptr<Drawable> drawable;
		Rectf margin;
		Rectf padding;
		Float lineHeight{ 0 };
		BaselineAlignValue baselineAlign;
		bool suppressBackground{ false };
		Float baseline{ 0 };
		InlineFloat floatType{ InlineFloat::None };
		InlineClear clearType{ InlineClear::None };
		bool isLineBreak{ false };
		bool isBlock{ false };
		bool isBlockFormattingContext{ false };
		InlinePath inlinePath;
		Vector2f position; // Local position relative to RichText origin
		Sizef size;
		Int64 startCharIndex{ 0 };
		Int64 endCharIndex{ 0 };
		Int64 _leafIndex{ -1 }; // O(1) lookup index, assigned during layout
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

	/** Sets line-height as a multiplier of the font's default line spacing.
	 *  Use 0 to reset to font default. */
	void setLineHeight( Float height );

	/** @return The line height multiplier. */
	Float getLineHeight() const { return mLineHeight; }

	/** Sets text-indent (pixels) for the first line. */
	void setTextIndent( Float indent );

	/** @return The text indent in pixels. */
	Float getTextIndent() const { return mTextIndent; }

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
	SmallVector<Rectf> getSelectionRects() const;

	/** @return The current selection as a string. */
	String getSelectionString() const;

	/** Tries to update the layout if has been invalidated. This is automatically called before
	 * draw. */
	void updateLayout();

	/** Invalidates the current layout */
	void invalidateLayout();

	// ── Inline tree types (first-class inline boxes) ─────────────────────────

	/** A single item in the inline formatting tree. */
	struct InlineItem {
		struct TextRun {
			std::shared_ptr<Text> text;
			InlineSource source;
			Rectf margin;
			Rectf padding;
			Float lineHeight{ 0 };
			BaselineAlignValue baselineAlign;
			bool suppressBackground{ false };
		};

		struct Box {
			InlineSource source;
			Rectf margin;
			Rectf padding;
			Float lineHeight{ 0 };
			BaselineAlignValue baselineAlign;
			Color backgroundColor{ Color::Transparent };
			Float borderWidth{ 0 };
			Color borderColor{ Color::Transparent };
			Drawable* backgroundColorDrawable{ nullptr };
			Drawable* backgroundDrawable{ nullptr };
			Drawable* borderDrawable{ nullptr };
			bool backgroundDrawableUsesFragmentColor{ false };
			Uint32 textDecoration{ 0 };
			bool participatesInLineMetrics{ true };
			bool contributesInlineSpacing{ true };
			std::vector<InlineItem> children;
		};

		struct AtomicBox {
			InlineSource source;
			std::shared_ptr<Drawable> drawable;
			Sizef size;
			Float baseline{ 0 };
			InlineFloat floatType{ InlineFloat::None };
			InlineClear clearType{ InlineClear::None };
			bool isLineBreak{ false };
			bool isBlock{ false };
			bool isBlockFormattingContext{ false };
			BaselineAlignValue baselineAlign;
		};

		std::variant<TextRun, Box, AtomicBox> data;

		InlineItem() : data( TextRun{} ) {}
		explicit InlineItem( TextRun run ) : data( std::move( run ) ) {}
		explicit InlineItem( Box box ) : data( std::move( box ) ) {}
		explicit InlineItem( AtomicBox box ) : data( std::move( box ) ) {}

		TextRun& asTextRun() { return std::get<TextRun>( data ); }
		const TextRun& asTextRun() const { return std::get<TextRun>( data ); }
		Box& asBox() { return std::get<Box>( data ); }
		const Box& asBox() const { return std::get<Box>( data ); }
		AtomicBox& asAtomicBox() { return std::get<AtomicBox>( data ); }
		const AtomicBox& asAtomicBox() const { return std::get<AtomicBox>( data ); }

		bool isTextRun() const { return std::holds_alternative<TextRun>( data ); }
		bool isBox() const { return std::holds_alternative<Box>( data ); }
		bool isAtomicBox() const { return std::holds_alternative<AtomicBox>( data ); }
	};

	/** @return The inline item tree. */
	const std::vector<InlineItem>& getInlineItems() const { return mInlineItems; }

	/** A laid-out fragment produced by an inline item on one rendered line. */
	struct InlineFragment {
		enum class Type { TextRun, Box, AtomicBox };

		Type type{ Type::TextRun };
		RenderSpan::InlinePath itemPath;
		size_t lineIndex{ 0 };
		Rectf bounds;
		Rectf paintBounds;
		Int64 startCharIndex{ 0 };
		Int64 endCharIndex{ 0 };
		std::shared_ptr<Text> text;
		BaselineAlignValue baselineAlign;
		InlineSource source;
		bool startsInlineBox{ false };
		bool endsInlineBox{ false };
		Color backgroundColor{ Color::Transparent };
		Float borderWidth{ 0 };
		Color borderColor{ Color::Transparent };
		Drawable* backgroundColorDrawable{ nullptr };
		Drawable* backgroundDrawable{ nullptr };
		Drawable* borderDrawable{ nullptr };
		bool backgroundDrawableUsesFragmentColor{ false };
		Uint32 textDecoration{ 0 };
	};

	/** @return The generated inline fragments. */
	const std::vector<InlineFragment>& getInlineFragments() const { return mInlineFragments; }

	// ── Inline tree builder API (stack-based, used by UIRichText) ───────────

	/** Begin an inline box scope. All subsequent inline items are added as
	 *  children of this box until popInlineBox() is called. An InlineItem::Box
	 *  is created in the inline tree. */
	void pushInlineBox( const Rectf& margin, const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign,
						const Color& backgroundColor = Color::Transparent, Float borderWidth = 0,
						const Color& borderColor = Color::Transparent, Uint32 textDecoration = 0,
						InlineSource source = {}, Drawable* backgroundColorDrawable = nullptr,
						Drawable* backgroundDrawable = nullptr, Drawable* borderDrawable = nullptr,
						bool backgroundDrawableUsesFragmentColor = false );

	/** Close the current inline box scope. */
	void popInlineBox();

	/** Add a text run to the current inline context. */
	void addInlineText( const String& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign, InlineSource source = {} );

	void addInlineText( const String::View& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign, InlineSource source = {} );

	void addInlineText( String&& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign, InlineSource source = {} );

	/** Add an atomic inline-level box to the current inline context. */
	void addInlineAtomicBox( const Sizef& size, InlineFloat floatType, InlineClear clearType,
							 Float baseline, bool isLineBreak,
							 const BaselineAlignValue& baselineAlign, InlineSource source = {} );

  protected:
	template <typename TextType>
	void addSpanImpl( TextType&& text, const FontStyleConfig& style, const Rectf& margin,
					  const Rectf& padding, Float lineHeight,
					  const BaselineAlignValue& baselineAlign, InlineSource source );

	template <typename TextType>
	void addInlineTextImpl( TextType&& text, const FontStyleConfig& style, const Rectf& margin,
							const Rectf& padding, Float lineHeight,
							const BaselineAlignValue& baselineAlign, InlineSource source );

	void rebuildInlineFragments();

	std::vector<InlineItem> mInlineItems;
	RenderSpan::InlinePath mInlinePath; // Path into the inline tree for the stack-based builder
	std::vector<InlineFragment> mInlineFragments;
	std::vector<FloatExclusion> mExternalFloatExclusions;
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
	Float mLineHeight{ 0 };
	Float mTextIndent{ 0 };
	bool mLineWrap{ true };
	WhiteSpaceWrapMode mWhiteSpaceWrapMode{ WhiteSpaceWrapMode::Normal };
	Uint32 mTabWidth{ 8 };
};

}} // namespace EE::Graphics

#endif
