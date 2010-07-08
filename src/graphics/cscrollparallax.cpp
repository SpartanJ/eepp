#include "cscrollparallax.hpp"

namespace EE { namespace Graphics {

cScrollParallax::cScrollParallax() {
	TF = cTextureFactory::instance();
}

cScrollParallax::~cScrollParallax() {}

cScrollParallax::cScrollParallax( const Uint32& TexId, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeRecti& SrcRECT, const eeRGBA& Color, const Uint8& Alpha, const EE_RENDERALPHAS& Effect ) {
	TF = cTextureFactory::instance();
	Create( TexId, DestX, DestY, DestWidth, DestHeight, SrcRECT, Color, Alpha, Effect );
}

bool cScrollParallax::Create(const Uint32& TexId, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeRecti& SrcRECT, const eeRGBA& Color, const Uint8& Alpha, const EE_RENDERALPHAS& Effect ) {
	cTexture * Tex = TF->GetTexture( TexId );

	if ( NULL == Tex )
		return false;

	mSrcRECT = SrcRECT;

	if ( mSrcRECT.Bottom == 0 && mSrcRECT.Right == 0 ) {
		mSrcRECT.Left = 0;
		mSrcRECT.Top = 0;
		mSrcRECT.Right = (Int32)Tex->Width();
		mSrcRECT.Bottom = (Int32)Tex->Height();
	}

	mWidth = static_cast<eeFloat> ( mSrcRECT.Right - mSrcRECT.Left );
	mHeight = static_cast<eeFloat> ( mSrcRECT.Bottom - mSrcRECT.Top );

	mTilerWidth = DestWidth + DestX;
	mTilerHeight = DestHeight + DestY;

	if (DestX <= mTilerWidth) mPomSx = DestX;
	if (DestY <= mTilerHeight) mPomSy = DestY;

	mX = (Int16) ( ( mTilerWidth - mPomSx ) / mWidth ) + 1;
	mY = (Int16) ( ( mTilerHeight - mPomSy ) / mHeight ) + 1;

	mSpr.CreateStatic( TexId, mWidth, mHeight, 0, 0, mSrcRECT );

	mSpr.Color( Color );
	mSpr.Alpha( Alpha );
	mSpr.SetRenderAlphas( Effect );

	return true;
}

void cScrollParallax::Draw( const eeFloat& XDirVel, const eeFloat& YDirVel ) {
	Int16 tX, tY;
	eeFloat tPomSx, tPomSy, tWidth, tHeight;
	eeRectf tSrcRECT( (eeFloat)mSrcRECT.Left, (eeFloat)mSrcRECT.Top, (eeFloat)mSrcRECT.Right, (eeFloat)mSrcRECT.Bottom );

	mSx = mSx + XDirVel;
	if (mSx > mWidth) mSx = 0;
	if (mSx < -mWidth) mSx = 0;

	mSy = mSy + YDirVel;
	if (mSy > mHeight) mSy = 0;
	if (mSy < -mHeight) mSy = 0;

	for (tY = -1; tY <= mY; tY++ ) {
		for (tX = -1; tX <= mX; tX++) {
			tPomSx = mPomSx + ( (eeFloat)tX * mWidth ) + mSx;
			tPomSy = mPomSy + ( (eeFloat)tY * mHeight ) + mSy;

			if ( (tPomSx + mWidth) > mPomSx && (tPomSy + mHeight) > mPomSy && tPomSx < mTilerWidth && tPomSy < mTilerHeight ) {
				tWidth = mWidth;
				tHeight = mHeight;

				if ( tPomSy + mHeight > mTilerHeight && tPomSy < mTilerHeight ) {
					tSrcRECT.Bottom = tSrcRECT.Bottom - ((tPomSy + mHeight) - mTilerHeight);
					tHeight = tSrcRECT.Bottom - tSrcRECT.Top;
				}

				if ( tPomSx < mPomSx ) {
					tSrcRECT.Left = tSrcRECT.Left + mPomSx - tPomSx;
					tPomSx = mPomSx;
					tWidth = tSrcRECT.Right - tSrcRECT.Left;
				}

				if ( (tPomSx + mWidth) > mTilerWidth && tPomSx < mTilerWidth ) {
					tSrcRECT.Right = tSrcRECT.Right - ((tPomSx + mWidth) - mTilerWidth);
					tWidth = tSrcRECT.Right - tSrcRECT.Left;
				}

				if (tPomSy < mPomSy) {
					tSrcRECT.Top = tSrcRECT.Top + mPomSy - tPomSy;
					tPomSy = mPomSy;
					tHeight = tSrcRECT.Bottom - tSrcRECT.Top;
				}

				mSpr.UpdateSize( tWidth, tHeight );
				mSpr.UpdateSprRECT( eeRecti( (Int32)tSrcRECT.Left, (Int32)tSrcRECT.Top, (Int32)tSrcRECT.Right, (Int32)tSrcRECT.Bottom ) );
				mSpr.UpdatePos( tPomSx, tPomSy );
				mSpr.Draw();
			}
		}
	}
}

}}
