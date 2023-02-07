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
class UINode;

enum class BorderType : Uint32 { Inside, Outside, Outline };

struct EE_API Border {
	int width = 0;
	Color color;
	Color realColor;
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

	static std::string fromBorderType( const BorderType& borderType );

	static BorderType toBorderType( const std::string& borderType );

	static Sizef radiusFromString( const UINode* node, const std::string& val );

	/** Creates the border geometry into the VertexBuffer provided. The VertexBuffer must be a
	 * a EE::Graphics::PrimitiveType::PRIMITIVE_TRIANGLE_STRIP with VERTEX_FLAGS_PRIMITIVE flags. */
	static void createBorders( VertexBuffer* vbo, const Borders& borders, const Vector2f& pos,
							   const Sizef& size );

	/** Creates a rounded rectangle to use as background of a UI node. The VertexBuffer must be a
	 * a EE::Graphics::PrimitiveType::PRIMITIVE_TRIANGLE_FAN with VERTEX_FLAGS_PRIMITIVE flags. */
	static void createBackground( VertexBuffer* vbo, const BorderRadiuses& radius,
								  const Vector2f& pos, const Sizef& size, const Color& color );
};

struct BorderRadiuseStr {
	std::string topLeft;
	std::string topRight;
	std::string bottomLeft;
	std::string bottomRight;
};

struct BorderStr {
	struct {
		std::string left;
		std::string right;
		std::string top;
		std::string bottom;
	} width;
	BorderRadiuseStr radius;
};

}} // namespace EE::UI

#endif // EE_BORDER_HPP
