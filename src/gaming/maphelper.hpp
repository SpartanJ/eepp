#ifndef EE_GAMINGMAPHELPER_HPP
#define EE_GAMINGMAPHELPER_HPP

namespace EE { namespace Gaming {

class GObjFlags {
	public:
		enum EE_GAMEOBJECT_FLAGS {
			GAMEOBJECT_STATIC		=  ( 1 << 0 ),
			GAMEOBJECT_ANIMATED		=  ( 1 << 1 ),
			GAMEOBJECT_MIRRORED		=  ( 1 << 2 ),
			GAMEOBJECT_FLIPED		=  ( 1 << 3 ),
			GAMEOBJECT_BLOCKED		=  ( 1 << 4 )
		};
};

enum EE_GAMEOBJECT_TYPE {
	GAMEOBJECT_TYPE_BASE		= 236430550u,	//MakeHash( "Base" ),
	GAMEOBJECT_TYPE_VIRTUAL		= 4069800883u,	//MakeHash( "Virtual" ),
	GAMEOBJECT_TYPE_SHAPE		= 3517332124u,	//MakeHash( "Shape" ),
	GAMEOBJECT_TYPE_SHAPEEX		= 3708695628u,	//MakeHash( "ShapeEx" ),
	GAMEOBJECT_TYPE_SPRITE		= 2088954976u	//MakeHash( "Sprite" )
};

enum EE_LAYER_TYPE {
	MAP_LAYER_TILED,
	MAP_LAYER_OBJECT
};

enum EE_MAP_FLAGS {
	MAP_FLAG_CLAMP_BODERS	= ( 1 << 0 ),
	MAP_FLAG_CLIP_AREA		= ( 1 << 1 ),
	MAP_FLAG_DRAW_GRID		= ( 1 << 2 ),
	MAP_FLAG_DRAW_TILE_OVER = ( 1 << 3 )
};

}}

#endif
