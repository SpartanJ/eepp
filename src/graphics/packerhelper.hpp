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
	Uint32	ResourceID;
	Int32	OffsetX;
	Int32	OffsetY;
	Int32	DestWidth;
	Int32	DestHeight;
	Uint32	Flags;
} sShapeHdr;

#define HDR_SHAPE_FLAG_FLIPED ( 1 << 0 )

typedef struct sTextureHdrS {
	char	Name[ HDR_NAME_SIZE ];
	Uint32	ResourceID;
	Uint32	Size;
	Int32	Width;
	Int32	Height;
	Int32	ShapeCount;
} sTextureHdr;

typedef struct sTextureGroupHdrS {
	Uint32	Magic;
	Uint32	TextureCount;
} sTextureGroupHdr;

}}}

#endif
