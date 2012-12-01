#ifndef EE_PACKER_HELPER
#define EE_PACKER_HELPER

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics { namespace Private {

#define HDR_NAME_SIZE 64

typedef struct sSubTextureHdrS {
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
} sSubTextureHdr;

#define HDR_SUBTEXTURE_FLAG_FLIPED 					( 1 << 0 )

typedef struct sTextureHdrS {
	char	Name[ HDR_NAME_SIZE ];
	Uint32	ResourceID;
	Uint32	Size;
	Int32	SubTextureCount;
} sTextureHdr;

typedef struct sTextureAtlasHdrS {
	Uint32	Magic;
	Uint32	TextureCount;
	Uint32	Format;
	Int32	Width;
	Int32	Height;
	Uint32	PixelBorder;
	Uint32	Flags;
} sTextureAtlasHdr;

#define HDR_TEXTURE_ATLAS_ALLOW_FLIPPING		( 1 << 0 )
#define HDR_TEXTURE_ATLAS_REMOVE_EXTENSION		( 1 << 1 )
#define HDR_TEXTURE_ATLAS_POW_OF_TWO			( 1 << 2 )

#define EE_TEXTURE_ATLAS_MAGIC_OLD ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'T' << 16 ) | ( 'G' << 24 ) )
#define EE_TEXTURE_ATLAS_MAGIC ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'T' << 16 ) | ( 'A' << 24 ) )
#define EE_TEXTURE_ATLAS_EXTENSION ".eta"

}}}

#endif
