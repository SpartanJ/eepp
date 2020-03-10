#ifndef EE_BORDER_HPP
#define EE_BORDER_HPP

#include <eepp/config.hpp>
#include <eepp/math/size.hpp>
#include <eepp/system/color.hpp>

namespace EE { namespace Graphics {
class VertexBuffer;
}} // namespace EE::Graphics
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::Math;

namespace EE { namespace UI {

enum class BorderType : Uint32 { Inside, Outside };

struct EE_API Border {
	int width = 0;
	Color color;
};

struct EE_API BorderRadiuses {
	Sizef topLeft;
	Sizef topRight;
	Sizef bottomLeft;
	Sizef bottomRight;
};

struct EE_API Borders {
	Border left;
	Border top;
	Border right;
	Border bottom;
	BorderRadiuses radius;

	/** Creates the border geometry into the VertexBuffer provided. The VertexBuffer must be a
	 * a EE::Graphics::PrimitiveType::PRIMITIVE_TRIANGLE_STRIP with VERTEX_FLAGS_PRIMITIVE flags. */
	static void createBorders( VertexBuffer* vbo, const Borders& borders, const Vector2f& pos,
							   const Sizef& size );

	/** Creates a rounded rectangle to use as background of a UI node. The VertexBuffer must be a
	 * a EE::Graphics::PrimitiveType::PRIMITIVE_TRIANGLE_FAN with VERTEX_FLAGS_PRIMITIVE flags. */
	static void createBackground( VertexBuffer* vbo, const BorderRadiuses& radius,
								  const Vector2f& pos, const Sizef& size, const Color& color );
};

}} // namespace EE::UI

#endif // EE_BORDER_HPP
