#include "csprite.hpp"
#include "cglobalshapegroup.hpp"

namespace EE { namespace Graphics {

cSprite::cSprite() {
	Reset();
}

cSprite::~cSprite() {
	ClearFrame();
}

void cSprite::ClearFrame() {
	for ( eeUint i = 0; i < Frame.size(); i++ )
		Frame[i].Spr.clear();

	Frame.clear();
}

void cSprite::Reset() {
	ClearFrame();
	mAutoAnim = true;
	mAnimSpeed = 0.02f;
	mScale = 1;
	mRepeations = -1;
	mReverseAnim = false;
	mAnimPaused = false;
	mAlpha = 255;
	mAngle = 0;
	mColor = eeRGBA(255, 255, 255, mAlpha);

	mColor0.voidRGB = true;
	mColor1.voidRGB = true;
	mColor2.voidRGB = true;
	mColor3.voidRGB = true;

	mFrameData.CurrentFrame = 0;
	mFrameData.CurrentSubFrame = 0;
	mFrameData.fCurrentFrame = 0.f;
	mFrameData.SubFrames = 1;

	mScaleCentered = true;
	mBlend = ALPHA_NORMAL;
	mEffect = RN_NORMAL;
}

void cSprite::CurrentFrame (const eeFloat &CurFrame ) {
	mFrameData.fCurrentFrame = CurFrame;
	mFrameData.CurrentFrame = (eeUint)CurFrame;

	if ( mFrameData.fCurrentFrame >= Frame.size() ) {
		mFrameData.fCurrentFrame = (eeFloat)Frame.size() - 1;
		mFrameData.CurrentFrame = (eeUint)Frame.size() - 1;
	}

	if ( mFrameData.fCurrentFrame < 0 ) {
		mFrameData.fCurrentFrame = 0.0f;
		mFrameData.CurrentFrame = 0;
	}
}

void cSprite::CurrentSubFrame( const eeUint& CurSubFrame ) {
	if ( CurSubFrame < mFrameData.SubFrames )
		mFrameData.CurrentSubFrame = CurSubFrame;
}

eeVector2f cSprite::GetRotationCenter( const eeRectf& DestRECT ) {
	return eeVector2f ( DestRECT.Left + (DestRECT.Right - DestRECT.Left - 1.0f) * 0.5f , DestRECT.Top + (DestRECT.Bottom - DestRECT.Top - 1.0f) * 0.5f );
}

eeRecti cSprite::SprSrcRect() {
	if ( Frame.size() )
		return GetCurrentShape()->SrcRect();
	else
		return eeRecti();
}

eeRectf cSprite::SprDestRectf() {
	eeRectf TmpR;

	if ( Frame.size() ) {
		if ( mAngle != 0 || mEffect >= 4 ) {
			eeQuad2f Q = GetQuad();
			eeFloat MinX = Q.V[0].x, MaxX = Q.V[0].x, MinY = Q.V[0].y, MaxY = Q.V[0].y;
			for (Uint8 i = 1; i < 4; i++ ) {
				if ( MinX > Q.V[i].x ) MinX = Q.V[i].x;
				if ( MaxX < Q.V[i].x ) MaxX = Q.V[i].x;
				if ( MinY > Q.V[i].y ) MinY = Q.V[i].y;
				if ( MaxY < Q.V[i].y ) MaxY = Q.V[i].y;
			}

			TmpR.Left = MinX;
			TmpR.Right = MaxX;
			TmpR.Top = MinY;
			TmpR.Bottom = MaxY;
		} else { // The method used if mAngle != 0 works for mAngle = 0, but i prefer to use the faster way
			cShape* S = GetCurrentShape();
			if ( S != NULL && mScaleCentered )
				if (mScale == 1.f)
					TmpR = eeRectf( mX, mY, mX + S->DestWidth(), mY + S->DestHeight() );
				else {
					eeFloat halfW = S->DestWidth() * 0.5f;
					eeFloat halfH = S->DestHeight() * 0.5f;
					TmpR = eeRectf( mX + halfW - halfW * mScale, mY + halfH - halfH * mScale, mX + halfW + halfW * mScale, mY + halfH + halfH * mScale );
				}
			else
				TmpR = eeRectf(mX, mY, mX + S->DestWidth() * mScale, mY + + S->DestHeight() * mScale);
		}
	}

	return TmpR;
}

eeRecti cSprite::SprDestRECT() {
	eeRectf R = SprDestRectf();
	return eeRecti( (Int32)R.Left, (Int32)R.Top, (Int32)R.Right, (Int32)R.Bottom );
}

eeAABB cSprite::GetAABB() {
	return SprDestRectf();
}

void cSprite::UpdatePos(const eeFloat& x, const eeFloat& y) {
	mX = x;
	mY = y;
}

void cSprite::UpdatePos( const eeVector2f& NewPos ) {
	UpdatePos( NewPos.x, NewPos.y );
}

void cSprite::UpdateSize( const eeFloat& Width, const eeFloat& Height, const eeUint& FrameNum ) {
	eeUint FN;
	if ( FrameNum >= Frame.size() )
		FN = mFrameData.CurrentFrame;
	else
		FN = FrameNum;

	cShape* S = Frame[FN].Spr[mFrameData.CurrentSubFrame];
	S->DestWidth( Width );
	S->DestHeight( Height );
}

void cSprite::UpdateVertexColors( const eeRGBA& Color0, const eeRGBA& Color1, const eeRGBA& Color2, const eeRGBA& Color3 ) {
	mColor0 = Color0;
	mColor1 = Color1;
	mColor2 = Color2;
	mColor3 = Color3;
	mColor0.voidRGB = false;
	mColor1.voidRGB = false;
	mColor2.voidRGB = false;
	mColor3.voidRGB = false;
}

void cSprite::Update( const eeFloat& x, const eeFloat& y, const eeFloat& Scale, const eeFloat& Angle, const Uint8& Alpha, const eeRGBA& Color ) {
	mX = x;
	mY = y;
	mScale = Scale;
	mAngle = Angle;
	mColor = Color;
	this->Alpha(Alpha);
}

Uint32 cSprite::GetTexture(const eeUint& FrameNum, const eeUint& SubFrameNum) {
	if ( FrameNum < Frame.size() && SubFrameNum < mFrameData.SubFrames )
		return Frame[FrameNum].Spr[SubFrameNum]->Texture();

	return 0;
}

eeUint cSprite::FramePos() {
	Frame.push_back( cFrame() );
	return Frame.size() - 1;
}

bool cSprite::CreateStatic(const Uint32& TexId, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& offSetX, const eeFloat& offSetY, const eeRecti& TexSector) {
	if ( TexId < cTextureFactory::instance()->GetNumTextures() ) {
		Reset();
		mFrameData.SubFrames = 1;
		mAnimated = false;
		eeUint id = FramePos();

		return AddSubFrame(TexId, id, 0, DestWidth, DestHeight, offSetX, offSetY, TexSector);
	}

	return false;
}

void cSprite::CreateAnimation( const eeUint& SubFramesNum ) {
	Reset();
	mAnimated = true;

	if ( SubFramesNum < 1 )
		mFrameData.SubFrames = 1;
	else
		mFrameData.SubFrames = SubFramesNum;
}

eeUint cSprite::AddFrame(const Uint32& TexId, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& offSetX, const eeFloat& offSetY, const eeRecti& TexSector) {
	eeUint id = FramePos();

	if ( TexId < cTextureFactory::instance()->GetNumTextures() ) {
		AddSubFrame( TexId, id, 0, DestWidth, DestHeight, offSetX, offSetY, TexSector );
		return id;
	} else
		return 0;
}

bool cSprite::AddSubFrame(const Uint32& TexId, const eeUint& NumFrame, const eeUint& NumSubFrame, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& offSetX, const eeFloat& offSetY, const eeRecti& TexSector) {
	cTexture * Tex = cTextureFactory::instance()->GetTexture( TexId );

	if ( NULL == Tex )
		return false;

	eeUint NF, NSF;

	if ( NumFrame >= Frame.size() )
		NF = 0;
	else
		NF = NumFrame;

	if ( NumSubFrame >= mFrameData.SubFrames )
		NSF = 0;
	else
		NSF = NumSubFrame;

	if ( Frame[NF].Spr.size() != (eeUint)mFrameData.SubFrames )
		Frame[NF].Spr.resize( mFrameData.SubFrames );

	cShape * S = cGlobalShapeGroup::instance()->Add( new cShape() );
	Frame[NF].Spr[NSF] = S;

	if ( TexId ) {
		S->Texture( TexId );

		if ( TexSector.Right > 0 && TexSector.Bottom > 0 )
			S->SrcRect( TexSector );
		else
			S->SrcRect( eeRecti( 0, 0, (Int32)Tex->Width(), (Int32)Tex->Height() ) );

		if ( DestWidth > 0 )
			S->DestWidth( DestWidth );
		else
			S->DestWidth( static_cast<eeFloat> ( S->SrcRect().Right - S->SrcRect().Left ) );

		if ( DestHeight > 0 )
			S->DestHeight( DestHeight );
		else
			S->DestHeight( static_cast<eeFloat> ( S->SrcRect().Bottom - S->SrcRect().Top ) );

		S->OffsetX( offSetX );
		S->OffsetY( offSetY );

		return true;
	}

	return false;
}

void cSprite::Animate( const eeFloat& ElapsedTime ) {
	if ( mAnimated && mAutoAnim && !mAnimPaused ) {
		eeUint Size = Frame.size() - 1;
		eeFloat Elapsed = ( ElapsedTime != -99999.f ) ? (eeFloat)ElapsedTime : (eeFloat)cEngine::instance()->Elapsed();

		if ( mRepeations == 0 )
			return;

		if ( !mReverseAnim )
			mFrameData.fCurrentFrame += ( Elapsed * mAnimSpeed );
		else
			mFrameData.fCurrentFrame -= ( Elapsed * mAnimSpeed );

		mFrameData.CurrentFrame = (eeUint)mFrameData.fCurrentFrame;

		if ( !mReverseAnim && mFrameData.fCurrentFrame > Size + 1.0f ) {
			if ( mRepeations < 0 ) {
				mFrameData.fCurrentFrame = 0.0f;
				mFrameData.CurrentFrame = 0;
			} else {
				if ( mRepeations == 0 ) {
					mFrameData.fCurrentFrame = (eeFloat)Size;
					mFrameData.CurrentFrame = Size;
				} else {
					mFrameData.fCurrentFrame = 0.0f;
					mFrameData.CurrentFrame = 0;

					mRepeations--;
				}
			}
		} else if ( mReverseAnim && mFrameData.fCurrentFrame < 0.0f ) {
			if ( mRepeations < 0 ) {
				mFrameData.fCurrentFrame = Size + 1.0f;
				mFrameData.CurrentFrame = Size;
			} else {
				if ( mRepeations == 0 ) {
					mFrameData.fCurrentFrame = 0.0f;
					mFrameData.CurrentFrame = 0;
				} else {
					mFrameData.fCurrentFrame = Size + 1.0f;
					mFrameData.CurrentFrame = Size;

					mRepeations--;
				}
			}
		}

		if ( mFrameData.fCurrentFrame < 0.0f ) {
			if ( mReverseAnim ) {
				mFrameData.fCurrentFrame = 0.0f;
				mFrameData.CurrentFrame = 0;
			} else {
				mFrameData.fCurrentFrame = (eeFloat)Size;
				mFrameData.CurrentFrame = Size;
			}
		}
	}
}

eeUint cSprite::GetEndFrame() {
	if ( mReverseAnim ) {
		return 0;
	} else {
		return Frame.size() - 1;
	}
}

void cSprite::SetReverseFromStart() {
	mReverseAnim = true;
	eeUint Size = Frame.size() - 1;

	mFrameData.fCurrentFrame = (eeFloat)Size;
	mFrameData.CurrentFrame = Size;
}

void cSprite::Draw( const EE_RENDERALPHAS& Blend, const EE_RENDERTYPE& Effect, const eeFloat& ElapsedTime ) {
	Animate( ElapsedTime );

	cShape * S = GetCurrentShape();

	if ( S == NULL )
		return;

	if ( mColor0.voidRGB )
		S->Draw( mX, mY, mColor, mAngle, mScale, Blend, Effect, mScaleCentered );
	else
		S->Draw( mX, mY, mAngle, mScale, mColor0, mColor1, mColor2, mColor3, Blend, Effect, mScaleCentered );
}

void cSprite::Draw() {
	Draw(mBlend, mEffect);
}

void cSprite::Draw( const EE_RENDERALPHAS& Blend ) {
	Draw(Blend, mEffect);
}

void cSprite::Draw( const EE_RENDERTYPE& Effect ) {
	Draw(mBlend, Effect);
}

eeUint cSprite::GetFrame( const eeUint& FrameNum ) {
	eeUint FN;

	if ( FrameNum >= Frame.size() )
		FN = mFrameData.CurrentFrame;
	else
		FN = FrameNum;

	return FN;
}

eeUint cSprite::GetSubFrame( const eeUint& SubFrame ) {
	eeUint SFN;

	if ( SubFrame >= mFrameData.SubFrames )
		SFN = mFrameData.CurrentSubFrame;
	else
		SFN = SubFrame;

	return SFN;
}

const eeFloat cSprite::OffSetX() {
	cShape* S = GetCurrentShape();

	if ( S != NULL )
		return S->OffsetX();

	return 0.0f;
}

void cSprite::OffSetX( const eeFloat& offsetx ) {
	cShape* S = GetCurrentShape();

	if ( S != NULL )
		S->OffsetX( offsetx );
}

const eeFloat cSprite::OffSetY() {
	cShape* S = GetCurrentShape();

	if ( S != NULL )
		return S->OffsetY();

	return 0.0f;
}

void cSprite::OffSetY( const eeFloat& offsety ) {
	cShape* S = GetCurrentShape();

	if ( S != NULL )
		S->OffsetY( offsety );
}

void cSprite::OffSet( const eeVector2f& offset ) {
	cShape* S = GetCurrentShape();

	if ( S != NULL ) {
		S->OffsetX( offset.x );
		S->OffsetY( offset.y );
	}
}

void cSprite::UpdateSprRECT( const eeRecti& R, const eeUint& FrameNum, const eeUint& SubFrame ) {
	Frame[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->SrcRect( R );
}

void cSprite::Width( const eeFloat& Width, const eeUint& FrameNum, const eeUint& SubFrame ) {
	Frame[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestWidth( Width );
}

eeFloat cSprite::Width( const eeUint& FrameNum, const eeUint& SubFrame ) {
	return Frame[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestWidth();
}

void cSprite::Height( const eeFloat& Height, const eeUint& FrameNum, const eeUint& SubFrame ) {
	Frame[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestHeight( Height );
}

eeFloat cSprite::Height( const eeUint& FrameNum, const eeUint& SubFrame ) {
	return Frame[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestHeight();
}

void cSprite::SetRepeations( const int& Repeations ) {
	mRepeations = Repeations;
}

void cSprite::SetAutoAnimate( const bool& Autoanim ) {
	mAutoAnim = Autoanim;
}

eeQuad2f cSprite::GetQuad() {
	if ( Frame.size() ) {
		cShape* S = GetCurrentShape();
		eeRectf TmpR;

		if ( mScaleCentered )
			if ( mScale == 1.0f )
				TmpR = eeRectf( mX, mY, mX + S->DestWidth(), mY + S->DestHeight() );
			else {
				eeFloat halfW = S->DestWidth() * 0.5f;
				eeFloat halfH = S->DestHeight() * 0.5f;
				TmpR = eeRectf( mX + halfW - halfW * mScale, mY + halfH - halfH * mScale, mX + halfW + halfW * mScale, mY + halfH + halfH * mScale );
			}
		else
			TmpR = eeRectf( mX, mY, mX + S->DestWidth() * mScale, mY + S->DestHeight() * mScale );

		eeQuad2f Q = eeQuad2f( eeVector2f( TmpR.Left, TmpR.Top ), eeVector2f( TmpR.Left, TmpR.Bottom ), eeVector2f( TmpR.Right, TmpR.Bottom ), eeVector2f( TmpR.Right, TmpR.Top ) );

		switch ( mEffect ) {
			case RN_NORMAL:
			case RN_MIRROR:
			case RN_FLIP:
			case RN_FLIPMIRROR:
				if ( mAngle != 0.0f )
					Q.Rotate( mAngle, GetRotationCenter(TmpR) );
				return Q;

				break;
			case RN_ISOMETRIC:
				Q.V[0].x += ( TmpR.Right - TmpR.Left );
				Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[3].x += ( TmpR.Right - TmpR.Left );
				Q.V[3].y += ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );

				if ( mAngle != 0.0f )
					Q.Rotate( mAngle, GetRotationCenter(TmpR) );
				return Q;

				break;
			case RN_ISOMETRICVERTICAL:
				Q.V[0].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );

				if ( mAngle != 0.0f )
					Q.Rotate( mAngle, GetRotationCenter(TmpR) );
				return Q;

				break;
			case RN_ISOMETRICVERTICALNEGATIVE:
				Q.V[2].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[3].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );

				if ( mAngle != 0.0f )
					Q.Rotate( mAngle, GetRotationCenter(TmpR) );
				return Q;

				break;
		}
	}
	return eeQuad2f();
}

cShape* cSprite::GetCurrentShape() {
	if ( Frame.size() )
		return Frame[ mFrameData.CurrentFrame ].Spr[ mFrameData.CurrentSubFrame ];

	return NULL;
}

cShape* cSprite::GetShape( const eeUint& frame ) {
	if ( frame < Frame.size() )
		return Frame[ frame ].Spr[ mFrameData.CurrentSubFrame ];

	return NULL;
}

cShape* cSprite::GetShape( const eeUint& frame, const eeUint& SubFrame ) {
	if ( frame < Frame.size() )
		return Frame[ frame ].Spr[ SubFrame ];

	return NULL;
}

}}
