#ifndef EE_GRAPHICSCSCROLLPARALLAX_H
#define EE_GRAPHICSCSCROLLPARALLAX_H

#include "base.hpp"
#include "ctexture.hpp"
#include "ctexturefactory.hpp"
#include "csprite.hpp"

namespace EE { namespace Graphics {

class EE_API cScrollParallax {
	public:
		cScrollParallax();
		~cScrollParallax();

		/** Constructor that create's the Scroll Parallax */
		cScrollParallax(const Uint32& TexId, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeRecti& SrcRECT = eeRecti(0, 0, 0, 0), const eeRGBA& Color = eeRGBA(255, 255, 255, 255), const Uint8& Alpha = 255, const EE_PRE_BLEND_FUNC& Effect = ALPHA_NORMAL);

		/** Create's the Scroll Parallax
		* @param TexId The Internal Texture Id
		* @param DestX The X position
		* @param DestY The Y position
		* @param DestWidth The Width of the Parallax
		* @param DestHeight The Height of the Parallax
		* @param SrcRECT The Texture source eeRectf to render ( default render all the texture )
		* @param Color The Texture Color
		* @param Alpha The Texture Alpha
		* @param Effect The Blend Mode ( default ALPHA_NORMAL )
		* @return True if success
		*/
		bool Create(const Uint32& TexId, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeRecti& SrcRECT = eeRecti(0, 0, 0, 0), const eeRGBA& Color = eeRGBA(255, 255, 255, 255), const Uint8& Alpha = 255, const EE_PRE_BLEND_FUNC& Effect = ALPHA_NORMAL);

		/** Set the Alpha */
		void Alpha( const Uint8& Alpha ) { mSpr.Alpha( Alpha ); }

		/** Get the Alpha */
		eeFloat Alpha() const { return mSpr.Alpha(); }

		/** Set the Color */
		void Color( const eeRGBA& Color ) { mSpr.Color( Color ); }

		/** Get the color */
		eeRGBA Color() const { return mSpr.Color(); }

		/** Set the Blend Mode */
		void SetRenderAlphas( const EE_PRE_BLEND_FUNC& Effect ) { mSpr.SetRenderAlphas( Effect ); }

		/** Draw the Scroll Parallax
		* @param XDirVel X Direction Speed to move the parallax.
		* @param YDirVel Y Direction Speed to move the parallax.
		*/
		void Draw( const eeFloat& XDirVel, const eeFloat& YDirVel );
	private:
		cTextureFactory * TF;

		cSprite mSpr;
		eeRecti mSrcRECT;

		Int16 mX, mY;
		eeFloat mPomSx, mPomSy, mWidth, mHeight, mTilerWidth, mTilerHeight, mSx, mSy;
};

}}

#endif
