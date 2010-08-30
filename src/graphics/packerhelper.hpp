#ifndef EE_PACKER_HELPER
#define EE_PACKER_HELPER

#include "base.hpp"

namespace EE { namespace Graphics { namespace Private {

#define HDR_NAME_SIZE 64

typedef struct sShapeHdrS {
	char	Name[ HDR_NAME_SIZE ];
	Uint32	Date;
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
} sShapeHdr;

#define HDR_SHAPE_FLAG_FLIPED 					( 1 << 0 )

typedef struct sTextureHdrS {
	char	Name[ HDR_NAME_SIZE ];
	Uint32	ResourceID;
	Uint32	Size;
	Int32	ShapeCount;
} sTextureHdr;

typedef struct sTextureGroupHdrS {
	Uint32	Magic;
	Uint32	TextureCount;
	Uint32	Format;
	Int32	Width;
	Int32	Height;
	Uint32	PixelBorder;
	Uint32	Flags;
} sTextureGroupHdr;

#define HDR_TEXTURE_GROUP_ALLOW_FLIPPING		( 1 << 0 )
#define HDR_TEXTURE_GROUP_REMOVE_EXTENSION		( 1 << 1 )
#define HDR_TEXTURE_GROUP_POW_OF_TWO			( 1 << 2 )

}}}

#endif
