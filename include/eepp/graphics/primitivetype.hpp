#ifndef EE_PRIMITIVETYPE_HPP
#define EE_PRIMITIVETYPE_HPP

namespace EE { namespace Graphics {

/** @enum PrimitiveType The batch renderer and vertex buffer primitive types allowed */
enum PrimitiveType {
	PRIMITIVE_POINTS 			= 0x0000,
	PRIMITIVE_LINES 			= 0x0001,
	PRIMITIVE_LINE_LOOP 		= 0x0002,
	PRIMITIVE_LINE_STRIP 		= 0x0003,
	PRIMITIVE_TRIANGLES 		= 0x0004,
	PRIMITIVE_TRIANGLE_STRIP 	= 0x0005,
	PRIMITIVE_TRIANGLE_FAN 	= 0x0006,
	PRIMITIVE_QUADS 			= 0x0007,
	PRIMITIVE_QUAD_STRIP 		= 0x0008,
	PRIMITIVE_POLYGON 			= 0x0009
};

/** @enum PrimitiveFillMode Defines the fill mode for the primitives. */
enum PrimitiveFillMode {
	DRAW_LINE, 				//!< Draw only lines
	DRAW_FILL 				//!< Draw filled objects
};

}}

#endif
