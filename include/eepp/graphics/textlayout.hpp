#pragma once

#include <eepp/core/string.hpp>
#include <eepp/graphics/linewrap.hpp>
#include <eepp/graphics/shapedglyph.hpp>
#include <eepp/math/size.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace EE::Graphics {

class Font;

struct ShapedTextParagraph {
	std::vector<ShapedGlyph> shapedGlyphs;
	LineWrapInfo wrapInfo;
	Sizef size;
};

class EE_API TextLayout {
  public:
	using Cache = std::shared_ptr<const TextLayout>;

	std::vector<ShapedTextParagraph> paragraphs;
	Sizef size;
	TextDirection direction{ TextDirection::Unspecified };
	bool hasMixedDirection : 1 { false };

	bool isRTL() const { return direction == TextDirection::RightToLeft; }

	std::vector<Float> getLinesWidth() const;

	static Cache layout( const String& string, Font* font, const Uint32& fontSize,
						 const Uint32& style, const Uint32& tabWidth = 4,
						 const Float& outlineThickness = 0.f, std::optional<Float> tabOffset = {},
						 Uint32 textDrawHints = 0,
						 TextDirection baseDirection = TextDirection::LeftToRight,
						 LineWrapMode lineWrapMode = LineWrapMode::NoWrap, Uint32 wrapWidth = 0,
						 bool keepIndentation = false, Float initialXOffset = 0 );

	static Cache layout( const String::View& string, Font* font, const Uint32& fontSize,
						 const Uint32& style, const Uint32& tabWidth = 4,
						 const Float& outlineThickness = 0.f, std::optional<Float> tabOffset = {},
						 Uint32 textDrawHints = 0,
						 TextDirection baseDirection = TextDirection::LeftToRight,
						 LineWrapMode lineWrapMode = LineWrapMode::NoWrap, Uint32 wrapWidth = 0,
						 bool keepIndentation = false, Float initialXOffset = 0 );

  protected:
	static void wrapLayout( const String::View& string, TextLayout&, LineWrapMode lineWrapMode,
							Float wrapWidth, Float vspace, bool keepIndentation, Font* font,
							const Uint32& characterSize, const Uint32& fontStyle,
							const Uint32& tabWidth, const Float& outlineThickness, Float hspace );
};

} // namespace EE::Graphics
