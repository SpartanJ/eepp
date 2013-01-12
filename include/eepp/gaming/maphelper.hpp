#ifndef EE_GAMINGMAPHELPER_HPP
#define EE_GAMINGMAPHELPER_HPP

#include <eepp/declares.hpp>

namespace EE { namespace Gaming {

#define MAP_PROPERTY_SIZE			(64)
#define LAYER_NAME_SIZE				(64)
#define MAP_TEXTUREATLAS_PATH_SIZE	(128)

typedef struct sPropertyHdrS {
	char	Name[ MAP_PROPERTY_SIZE ];
	char	Value[ MAP_PROPERTY_SIZE ];
} sPropertyHdr;

typedef struct sMapTextureAtlasS {
	char	Path[ MAP_TEXTUREATLAS_PATH_SIZE ];
} sMapTextureAtlas;

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
	Uint32	TextureAtlasCount;
	Uint32	VirtualObjectTypesCount;
	Uint32	BaseColor;
	Uint32	LightsCount;
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

typedef struct sMapLightHdrS {
	Uint32	Radius;
	Int32	PosX;
	Int32	PosY;
	Uint32	Color;
	Uint32	Type;
} sMapLightHdr;

typedef struct sMapObjObjHdrS {
	char	Name[ MAP_PROPERTY_SIZE ];
	char	Type[ MAP_PROPERTY_SIZE ];
	Uint32	PointCount;
	Uint32	PropertyCount;
} sMapObjObjHdr;

class GObjFlags {
	public:
		enum EE_GAMEOBJECT_FLAGS {
			GAMEOBJECT_STATIC				= ( 1 << 0 ),
			GAMEOBJECT_ANIMATED				= ( 1 << 1 ),
			GAMEOBJECT_MIRRORED				= ( 1 << 2 ),
			GAMEOBJECT_FLIPED				= ( 1 << 3 ),
			GAMEOBJECT_BLOCKED				= ( 1 << 4 ),
			GAMEOBJECT_ROTATE_90DEG			= ( 1 << 5 ),
			GAMEOBJECT_AUTO_FIX_TILE_POS	= ( 1 << 6 )
		};
};

enum EE_GAMEOBJECT_TYPE {
	GAMEOBJECT_TYPE_BASE			= 2088954976UL,	//String::Hash( "Base" )
	GAMEOBJECT_TYPE_VIRTUAL			= 3708695628UL,	//String::Hash( "Virtual" )
	GAMEOBJECT_TYPE_SUBTEXTURE		= 1772101792UL,	//String::Hash( "SubTexture" )
	GAMEOBJECT_TYPE_SUBTEXTUREEX	= 1378537981UL,	//String::Hash( "SubTextureEx" )
	GAMEOBJECT_TYPE_SPRITE			= 3517332124UL,	//String::Hash( "Sprite" )
	GAMEOBJECT_TYPE_OBJECT			= 3343895260UL,	//String::Hash( "Object" )
	GAMEOBJECT_TYPE_POLYGON			= 482716845UL,	//String::Hash( "Polygon" )
	GAMEOBJECT_TYPE_POLYLINE		= 3044927249UL	//String::Hash( "Polyline" )
};

enum EE_LAYER_TYPE {
	MAP_LAYER_TILED,
	MAP_LAYER_OBJECT
};

enum EE_MAP_FLAGS {
	MAP_FLAG_CLAMP_BORDERS		= ( 1 << 0 ),
	MAP_FLAG_CLIP_AREA			= ( 1 << 1 ),
	MAP_FLAG_DRAW_GRID			= ( 1 << 2 ),
	MAP_FLAG_DRAW_TILE_OVER		= ( 1 << 3 ),
	MAP_FLAG_DRAW_BACKGROUND	= ( 1 << 4 ),
	MAP_FLAG_LIGHTS_ENABLED		= ( 1 << 5 ),
	MAP_FLAG_LIGHTS_BYVERTEX	= ( 1 << 6 ),
	MAP_FLAG_SHOW_BLOCKED		= ( 1 << 7 )
};

#define MAP_EDITOR_DEFAULT_FLAGS ( MAP_FLAG_CLAMP_BORDERS | MAP_FLAG_CLIP_AREA | MAP_FLAG_DRAW_GRID | MAP_FLAG_DRAW_BACKGROUND )

enum EE_LAYER_FLAGS {
	LAYER_FLAG_VISIBLE			= ( 1 << 0 ),
	LAYER_FLAG_LIGHTS_ENABLED	= ( 1 << 1 )
};

}}

#endif
