#ifndef EE_GRAPHICS_PIXELPERFECT_H
#define EE_GRAPHICS_PIXELPERFECT_H

#include "ctexturefactory.hpp"

namespace EE { namespace Graphics {

/** Pixel Perfect Collition implementation.
* @param TexId_1 First Texture Id
* @param x1 Screen X axis position for the first texture
* @param y1 Screen Y axis position for the first texture
* @param TexId_2 Second Texture Id
* @param x2 Screen X axis position for the second texture
* @param y2 Screen Y axis position for the second texture
* @param Tex1_SrcRECT The sector of the texture from TexId_1 that you are rendering, the sector you want to collide ( on cSprite the SprSrcRECT )
* @param Tex2_SrcRECT  The sector of the texture from TexId_2 that you are rendering, the sector you want to collide ( on cSprite the SprSrcRECT )
* @warning Stress the CPU easily. \n Creates a copy of the texture on the app contex. \n It will not work with scaled or rotated textures.
* @return True if collided
*/
bool EE_API PixelPerfectCollide( cTexture * Tex1, const Uint16& x1, const Uint16& y1, cTexture * Tex2, const Uint16& x2, const Uint16& y2, const eeRectu& Tex1_SrcRECT = eeRectu(0,0,0,0), const eeRectu& Tex2_SrcRECT = eeRectu(0,0,0,0) );

/** Pixel Perfect Collition implementation between texture and a point
* @param TexId_1 First Texture Id
* @param x1 Screen X axis position for the first texture
* @param y1 Screen Y axis position for the first texture
* @param x2 Screen X axis position for the point on screen
* @param y2 Screen Y axis position for the point on screen
* @param Tex1_SrcRECT The sector of the texture from TexId_1 that you are rendering, the sector you want to collide ( on cSprite the SprSrcRECT )
* @return True if collided
*/
bool EE_API PixelPerfectCollide( cTexture * Tex, const Uint16& x1, const Uint16& y1, const Uint16& x2, const Uint16& y2, const eeRectu& Tex1_SrcRECT = eeRectu(0,0,0,0) );

}}

#endif
