#pragma once

#include <eepp/core/string.hpp>
#include <eepp/graphics/shapedglyph.hpp>
#include <eepp/math/size.hpp>

#include <optional>
#include <vector>

namespace EE::Graphics {

class Font;

struct TextLayout {
	std::vector<ShapedGlyph> shapedGlyphs;
	std::vector<Float> linesWidth;
	Sizef size;
	bool isRTL{ false };
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

} // namespace EE::Graphics
