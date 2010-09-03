#ifndef EE_GRAPHICS_VBOHELPER_HPP
#define EE_GRAPHICS_VBOHELPER_HPP

namespace EE { namespace Graphics {

enum EE_VBO_USAGE_TYPE
{
	VBO_USAGE_TYPE_STATIC,
	VBO_USAGE_TYPE_DYNAMIC,
	VBO_USAGE_TYPE_STREAM,
	VBO_USAGE_TYPE_COUNT
};

#define VERTEX_FLAG_POSITION	( 0 )
#define VERTEX_FLAG_TEXTURE0	( 1 )
#define VERTEX_FLAG_TEXTURE1	( 2 )
#define VERTEX_FLAG_TEXTURE2	( 3 )
#define VERTEX_FLAG_TEXTURE3	( 4 )
#define VERTEX_FLAG_TEXTURE4	( 5 )
#define VERTEX_FLAG_COLOR		( 6 )

#define VERTEX_FLAGS_COUNT_ARR	( 6 )
#define VERTEX_FLAGS_COUNT 		( 7 )

#define VERTEX_FLAG_USE_INDICES ( 7 )

const int eeVertexElements[] = {
								2, //Position
								2, //Texture0
								2, //Texture1
								2, //Texture2
								2, //Texture3
								2, //Texture4
								4  //Color0
							};

#define VERTEX_FLAG_GET(X) 		( 1 << (X) )
#define VERTEX_FLAG_SET(F, X)	if ( !( F & VERTEX_FLAG_GET( X ) ) ) F |= VERTEX_FLAG_GET( X );
#define VERTEX_FLAG_QUERY(F, X) ( F & VERTEX_FLAG_GET(X) )

#define VERTEX_FLAGS_DEFAULT	( VERTEX_FLAG_GET( VERTEX_FLAG_POSITION ) | VERTEX_FLAG_GET( VERTEX_FLAG_TEXTURE0 ) | VERTEX_FLAG_GET( VERTEX_FLAG_COLOR ) )

}}

#endif
