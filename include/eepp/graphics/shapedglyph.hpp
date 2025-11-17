#pragma once

#include <eepp/config.hpp>
#include <eepp/math/vector2.hpp>

using namespace EE::Math;

namespace EE::Graphics {

class FontTrueType;

enum class TextDirection : Uint8 {
	Unspecified = 0, //!< Unspecified
	LeftToRight = 4, //!< Left-to-right
	RightToLeft, //!< Right-to-left
	TopToBottom, //!< Top-to-bottom
	BottomToTop	 //!< Bottom-to-top
};

struct ShapedGlyph {
	FontTrueType* font{ nullptr };
	Uint32 glyphIndex{ 0 };
	Uint32 stringIndex{ 0 };
	Vector2f position;
	Vector2f advance;
	TextDirection direction;
};

} // namespace EE::Graphics
