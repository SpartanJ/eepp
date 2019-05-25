#ifndef EE_PACKER_HELPER
#define EE_PACKER_HELPER

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics { namespace Private {

#pragma pack(push, 1)

#define HDR_NAME_SIZE 64

struct sTextureRegionHdr {
	char	Name[ HDR_NAME_SIZE ];
	Uint64	Date;
	Int32	X;
	Int32	Y;
	Int32	Width;
	Int32	Height;
	Int32	Channels;
	Uint32	ResourceID;
	Int32	OffsetX;
	Int32	OffsetY;
	Int32	DestWidth;
	Int32	DestHeight;
	Uint32	Flags;
	Uint32	PixelDensity;
};

#define HDR_TEXTUREREGION_FLAG_FLIPED 					( 1 << 0 )

struct sTextureHdr {
	char	Name[ HDR_NAME_SIZE ];
	Uint32	ResourceID;
	Uint32	Size;
	Int32	TextureRegionCount;
};

struct sTextureAtlasHdr {
	Uint32	Magic;
	Uint32	Version;
	Uint64	Date;
	Uint32	TextureCount;
	Uint32	Format;
	Int32	Width;
	Int32	Height;
	Uint32	PixelBorder;
	Uint32	Flags;
	char	TextureFilter;
	char	Reserved[15];
};

#define HDR_TEXTURE_ATLAS_ALLOW_FLIPPING		( 1 << 0 )
#define HDR_TEXTURE_ATLAS_REMOVE_EXTENSION		( 1 << 1 )
#define HDR_TEXTURE_ATLAS_POW_OF_TWO			( 1 << 2 )
#define HDR_TEXTURE_ATLAS_SCALABLE_SVG			( 1 << 3 )

#define EE_TEXTURE_ATLAS_MAGIC		( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'T' << 16 ) | ( 'A' << 24 ) )
#define EE_TEXTURE_ATLAS_EXTENSION ".eta"

#pragma pack(pop)

}}}

#endif
