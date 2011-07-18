#ifndef EE_GAMINGMAPHELPER_HPP
#define EE_GAMINGMAPHELPER_HPP

namespace EE { namespace Gaming {

#define MAP_PROPERTY_SIZE			(64)
#define LAYER_NAME_SIZE				(64)
#define MAP_SHAPEGROUP_PATH_SIZE	(128)

typedef struct sPropertyHdrS {
	char	Name[ MAP_PROPERTY_SIZE ];
	char	Value[ MAP_PROPERTY_SIZE ];
} sPropertyHdr;

typedef struct sMapShapeGroupS {
	char	Path[ MAP_SHAPEGROUP_PATH_SIZE ];
} sMapShapeGroup;

typedef struct sVirtualObjS {
	char	Name[ MAP_PROPERTY_SIZE ];
} sVirtualObj;

typedef struct sMapHdrS {
	Uint32	Magic;
	Uint32	SizeX;
	Uint32	SizeY;
	Uint32	TileSizeX;
	Uint32	TileSizeY;
	Uint32	MaxLayers;
	Uint32	LayerCount;
	Uint32	Flags;
	Uint32	PropertyCount;
	Uint32	ShapeGroupCount;
	Uint32	VirtualObjectTypesCount;
} sMapHdr;

typedef struct sLayerHdrS {
	char	Name[ LAYER_NAME_SIZE ];
	Uint32	Type;
	Uint32	Flags;
	Int32	OffsetX;
	Int32	OffsetY;
	Uint32	PropertyCount;
	Uint32	ObjectCount;		//! Only used by the Object Layer
} sLayerHdr;

typedef struct sMapTileGOHdrS {
	Uint32	Type;
	Uint32	Id;
	Uint32	Flags;
} sMapTileGOHdr;

typedef struct sMapObjGOHdrS {
	Uint32	Type;
	Uint32	Id;
	Uint32	Flags;
	Int32	PosX;
	Int32	PosY;
} sMapObjGOHdr;

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
