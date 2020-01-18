#ifndef EE_GRAPHICS_VBOHELPER_HPP
#define EE_GRAPHICS_VBOHELPER_HPP

namespace EE { namespace Graphics {

/// VBO default usage type
enum class VertexBufferUsageType : int {
	Static,
	Dynamic,
	Stream,
};

enum VertexFlags {
	VERTEX_FLAG_POSITION = 0,
	VERTEX_FLAG_TEXTURE0 = 1,
	VERTEX_FLAG_TEXTURE1 = 2,
	VERTEX_FLAG_TEXTURE2 = 3,
	VERTEX_FLAG_TEXTURE3 = 4,
	VERTEX_FLAG_COLOR = 5,
	VERTEX_FLAG_USE_INDICES = 6
};

#define VERTEX_FLAGS_COUNT_ARR ( 5 )
#define VERTEX_FLAGS_COUNT ( 6 )

const int VertexElementCount[] = {
	2, // Position
	2, // Texture0
	2, // Texture1
	2, // Texture2
	2, // Texture3
	4  // Color0
};

#define VERTEX_FLAG_GET( X ) ( 1 << ( X ) )
#define VERTEX_FLAG_SET( F, X )          \
	if ( !( F & VERTEX_FLAG_GET( X ) ) ) \
		F |= VERTEX_FLAG_GET( X );
#define VERTEX_FLAG_QUERY( F, X ) ( F & VERTEX_FLAG_GET( X ) )

/// Vertex data will have position, color and texture UV.
#define VERTEX_FLAGS_DEFAULT                                                              \
	( VERTEX_FLAG_GET( VERTEX_FLAG_POSITION ) | VERTEX_FLAG_GET( VERTEX_FLAG_TEXTURE0 ) | \
	  VERTEX_FLAG_GET( VERTEX_FLAG_COLOR ) )

/// Vertex data will have position and color.
#define VERTEX_FLAGS_PRIMITIVE \
	( VERTEX_FLAG_GET( VERTEX_FLAG_POSITION ) | VERTEX_FLAG_GET( VERTEX_FLAG_COLOR ) )

}} // namespace EE::Graphics

#endif
