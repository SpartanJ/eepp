#pragma once

#include <eepp/config.hpp>
#include <eepp/math/vector2.hpp>

using namespace EE::Math;

namespace EE::Graphics {

class FontTrueType;

struct ShapedGlyph {
	FontTrueType* font{ nullptr };
	Uint32 glyphIndex{ 0 };
	Uint32 stringIndex{ 0 };
	Vector2f position;
};

} // namespace EE::Graphics
