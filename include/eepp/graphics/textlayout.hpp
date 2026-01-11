#pragma once

#include <eepp/core/string.hpp>
#include <eepp/graphics/shapedglyph.hpp>
#include <eepp/math/size.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace EE::Graphics {

class Font;

class EE_API TextLayout {
  public:
	using Cache = std::shared_ptr<const TextLayout>;

	std::vector<ShapedGlyph> shapedGlyphs;
	std::vector<Float> linesWidth;
	Sizef size;
	TextDirection direction{ TextDirection::Unspecified };
	bool hasMixedDirection : 1 { false };

	bool isRTL() const { return direction == TextDirection::RightToLeft; }

	static Cache layout( const String& string, Font* font, const Uint32& fontSize,
						 const Uint32& style, const Uint32& tabWidth = 4,
						 const Float& outlineThickness = 0.f, std::optional<Float> tabOffset = {},
						 Uint32 textDrawHints = 0,
						 TextDirection baseDirection = TextDirection::LeftToRight );

	static Cache layout( const String::View& string, Font* font, const Uint32& fontSize,
						 const Uint32& style, const Uint32& tabWidth = 4,
						 const Float& outlineThickness = 0.f, std::optional<Float> tabOffset = {},
						 Uint32 textDrawHints = 0,
						 TextDirection baseDirection = TextDirection::LeftToRight );

  protected:
	template <typename StringType>
	static Cache layout( const StringType& string, Font* font, const Uint32& fontSize,
						 const Uint32& style, const Uint32& tabWidth = 4,
						 const Float& outlineThickness = 0.f, std::optional<Float> tabOffset = {},
						 Uint32 textDrawHints = 0,
						 TextDirection baseDirection = TextDirection::LeftToRight );
};

} // namespace EE::Graphics
