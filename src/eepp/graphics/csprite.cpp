#include <eepp/graphics/csprite.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/graphics/cglobaltextureatlas.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>

using namespace EE::Window;

#define SPR_FGET( Flag )	( mFlags & Flag )

namespace EE { namespace Graphics {

cSprite::cSprite() :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_SCALE_CENTERED | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f ),
	mAnimSpeed( 16.f ),
	mColor( 255,255,255,255 ),
	mVertexColors( NULL ),
	mRepeations( -1 ),
	mBlend( ALPHA_NORMAL ),
	mEffect( RN_NORMAL ),
	mCurrentFrame( 0 ),
	mfCurrentFrame( 0.f ),
	mCurrentSubFrame( 0 ),
	mSubFrames( 1 ),
	mAnimTo( 0 )
{
	mCb.Reset();
}

cSprite::cSprite( const std::string& name, const std::string& extension, cTextureAtlas * SearchInTextureAtlas )  :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_SCALE_CENTERED | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f ),
	mAnimSpeed( 16.f ),
	mColor( 255,255,255,255 ),
	mVertexColors( NULL ),
	mRepeations( -1 ),
	mBlend( ALPHA_NORMAL ),
	mEffect( RN_NORMAL ),
	mCurrentFrame( 0 ),
	mfCurrentFrame( 0.f ),
	mCurrentSubFrame( 0 ),
	mSubFrames( 1 ),
	mAnimTo( 0 )
{
	mCb.Reset();
	AddFramesByPattern( name, extension, SearchInTextureAtlas );
}

cSprite::cSprite( cSubTexture * SubTexture ) :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_SCALE_CENTERED | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f ),
	mAnimSpeed( 16.f ),
	mColor( 255,255,255,255 ),
	mVertexColors( NULL ),
	mRepeations( -1 ),
	mBlend( ALPHA_NORMAL ),
	mEffect( RN_NORMAL ),
	mCurrentFrame( 0 ),
	mfCurrentFrame( 0.f ),
	mCurrentSubFrame( 0 ),
	mSubFrames( 1 ),
	mAnimTo( 0 )
{
	mCb.Reset();
	CreateStatic( SubTexture );
}

cSprite::cSprite( const Uint32& TexId, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& offSetX, const eeFloat& offSetY, const eeRecti& TexSector ) :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_SCALE_CENTERED | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f ),
	mAnimSpeed( 16.f ),
	mColor( 255,255,255,255 ),
	mVertexColors( NULL ),
	mRepeations( -1 ),
	mBlend( ALPHA_NORMAL ),
	mEffect( RN_NORMAL ),
	mCurrentFrame( 0 ),
	mfCurrentFrame( 0.f ),
	mCurrentSubFrame( 0 ),
	mSubFrames( 1 ),
	mAnimTo( 0 )
{
	mCb.Reset();
	CreateStatic( TexId, DestWidth, DestHeight, offSetX, offSetY, TexSector );
}


cSprite::~cSprite() {
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

cSprite& cSprite::operator =( const cSprite& Other ) {
	mFrames				= Other.mFrames;
	mFlags				= Other.mFlags;
	mPos				= Other.mPos;
	mAngle				= Other.mAngle;
	mScale				= Other.mScale;
	mAnimSpeed			= Other.mAnimSpeed;
	mColor				= Other.mColor;
	mRepeations			= Other.mRepeations;
	mBlend				= Other.mBlend;
	mEffect				= Other.mEffect;
	mCurrentFrame		= Other.mCurrentFrame;
	mfCurrentFrame		= Other.mfCurrentFrame;
	mCurrentSubFrame	= Other.mCurrentSubFrame;
	mSubFrames			= Other.mSubFrames;
	mAnimTo				= Other.mAnimTo;
	mCb					= Other.mCb;

	if ( NULL != Other.mVertexColors ) {
		mVertexColors		= eeNewArray( eeColorA, 4 );
		mVertexColors[0]	= Other.mVertexColors[0];
		mVertexColors[1]	= Other.mVertexColors[1];
		mVertexColors[2]	= Other.mVertexColors[2];
		mVertexColors[3]	= Other.mVertexColors[3];
	} else {
		mVertexColors		= NULL;
	}

	return *this;
}

cSprite * cSprite::Copy() {
	cSprite * Spr = eeNew( cSprite, () );

	Spr->mFrames			= mFrames;
	Spr->mFlags				= mFlags;
	Spr->mPos				= mPos;
	Spr->mAngle				= mAngle;
	Spr->mScale				= mScale;
	Spr->mAnimSpeed			= mAnimSpeed;
	Spr->mColor				= mColor;
	Spr->mRepeations		= mRepeations;
	Spr->mBlend				= mBlend;
	Spr->mEffect			= mEffect;
	Spr->mCurrentFrame		= mCurrentFrame;
	Spr->mfCurrentFrame		= mfCurrentFrame;
	Spr->mCurrentSubFrame	= mCurrentSubFrame;
	Spr->mSubFrames			= mSubFrames;
	Spr->mAnimTo			= mAnimTo;
	Spr->mCb				= mCb;

	if ( NULL != mVertexColors ) {
		Spr->mVertexColors		= eeNewArray( eeColorA, 4 );
		Spr->mVertexColors[0]	= mVertexColors[0];
		Spr->mVertexColors[1]	= mVertexColors[1];
		Spr->mVertexColors[2]	= mVertexColors[2];
		Spr->mVertexColors[3]	= mVertexColors[3];
	} else {
		Spr->mVertexColors		= NULL;
	}

	return Spr;
}

void cSprite::ClearFrame() {
	for ( eeUint i = 0; i < mFrames.size(); i++ )
		mFrames[i].Spr.clear();

	mFrames.clear();
}

void cSprite::Reset() {
	ClearFrame();

	mFlags				= SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_SCALE_CENTERED | SPRITE_FLAG_EVENTS_ENABLED;

	mAnimSpeed			= 16.f;
	mScale				= 1;
	mRepeations			= -1;

	mAngle				= 0;
	mColor				= eeColorA(255, 255, 255, 255);

	mBlend				= ALPHA_NORMAL;
	mEffect				= RN_NORMAL;

	mCurrentFrame		= 0;
	mCurrentSubFrame 	= 0;
	mfCurrentFrame		= 0.f;
	mSubFrames			= 1;
	mAnimTo				= 0;

	DisableVertexColors();
}

void cSprite::CurrentFrame ( eeUint CurFrame ) {
	if ( CurFrame )
		CurFrame--;

	mfCurrentFrame = CurFrame;
	mCurrentFrame = (eeUint)CurFrame;

	if ( mfCurrentFrame >= mFrames.size() ) {
		mfCurrentFrame = (eeFloat)mFrames.size() - 1;
		mCurrentFrame = (eeUint)mFrames.size() - 1;
	}

	if ( mfCurrentFrame < 0 ) {
		mfCurrentFrame = 0.0f;
		mCurrentFrame = 0;
	}
}

void cSprite::CurrentSubFrame( const eeUint& CurSubFrame ) {
	if ( CurSubFrame < mSubFrames )
		mCurrentSubFrame = CurSubFrame;
}

eeVector2f cSprite::GetRotationCenter( const eeRectf& DestRECT ) {
	return eeVector2f ( DestRECT.Left + (DestRECT.Right - DestRECT.Left - 1.0f) * 0.5f , DestRECT.Top + (DestRECT.Bottom - DestRECT.Top - 1.0f) * 0.5f );
}

eeAABB cSprite::GetAABB() {
	eeAABB TmpR;

	if ( mFrames.size() ) {
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
			cSubTexture * S = GetCurrentSubTexture();

			if ( S != NULL ) {
				if ( SPR_FGET( SPRITE_FLAG_SCALE_CENTERED ) ) {
					if ( mScale == 1.f ) {
						TmpR = eeRectf( mPos.x, mPos.y, mPos.x + S->DestWidth(), mPos.y + S->DestHeight() );
					} else {
						eeFloat halfW = S->DestWidth() * 0.5f;
						eeFloat halfH = S->DestHeight() * 0.5f;
						TmpR = eeRectf( mPos.x + halfW - halfW * mScale, mPos.y + halfH - halfH * mScale, mPos.x + halfW + halfW * mScale, mPos.y + halfH + halfH * mScale );
					}
				} else {
					TmpR = eeRectf(mPos.x, mPos.y, mPos.x + S->DestWidth() * mScale, mPos.y + + S->DestHeight() * mScale);
				}
			}
		}
	}

	return TmpR;
}

const eeVector2f cSprite::Position() const {
	return mPos;
}

void cSprite::Position(const eeFloat& x, const eeFloat& y) {
	mPos.x = x;
	mPos.y = y;
}

void cSprite::Position( const eeVector2f& NewPos ) {
	mPos = NewPos;
}

void cSprite::UpdateVertexColors( const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3 ) {
	if ( NULL == mVertexColors )
		mVertexColors		= eeNewArray( eeColorA, 4 );

	mVertexColors[0]	= Color0;
	mVertexColors[1]	= Color1;
	mVertexColors[2]	= Color2;
	mVertexColors[3]	= Color3;
}

void cSprite::DisableVertexColors() {
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

eeUint cSprite::FramePos() {
	mFrames.push_back( cFrame() );
	return (eeUint)mFrames.size() - 1;
}

bool cSprite::CreateStatic( cSubTexture * SubTexture ) {
	Reset();

	AddFrame( SubTexture );

	return true;
}

bool cSprite::CreateStatic( const Uint32& TexId, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& offSetX, const eeFloat& offSetY, const eeRecti& TexSector ) {
	if ( cTextureFactory::instance()->TextureIdExists( TexId ) ) {
		Reset();

		AddFrame( TexId, DestWidth, DestHeight, offSetX, offSetY, TexSector );

		return true;
	}

	return false;
}

void cSprite::CreateAnimation( const eeUint& SubFramesNum ) {
	Reset();

	if ( SubFramesNum < 1 )
		mSubFrames = 1;
	else
		mSubFrames = SubFramesNum;
}

bool cSprite::AddFrames( const std::vector<cSubTexture*> SubTextures ) {
	if ( SubTextures.size() ) {
		for ( eeUint i = 0; i < SubTextures.size(); i++ ) {
			if ( NULL != SubTextures[i] ) {
				AddFrame( SubTextures[i] );
			}
		}

		return true;
	}

	return false;
}

bool cSprite::AddFramesByPatternId( const Uint32& SubTextureId, const std::string& extension, cTextureAtlas * SearchInTextureAtlas ) {
	std::vector<cSubTexture*> SubTextures = cTextureAtlasManager::instance()->GetSubTexturesByPatternId( SubTextureId, extension, SearchInTextureAtlas );

	if ( SubTextures.size() ) {
		AddFrames( SubTextures );

		return true;
	}

	return false;
}

bool cSprite::AddFramesByPattern( const std::string& name, const std::string& extension, cTextureAtlas * SearchInTextureAtlas ) {
	std::vector<cSubTexture*> SubTextures = cTextureAtlasManager::instance()->GetSubTexturesByPattern( name, extension, SearchInTextureAtlas );

	if ( SubTextures.size() ) {
		AddFrames( SubTextures );

		return true;
	}

	return false;
}

bool cSprite::AddSubFrame( cSubTexture * SubTexture, const eeUint& NumFrame, const eeUint& NumSubFrame ) {
	eeUint NF, NSF;

	if ( NumFrame >= mFrames.size() )
		NF = 0;
	else
		NF = NumFrame;

	if ( NumSubFrame >= mSubFrames )
		NSF = 0;
	else
		NSF = NumSubFrame;

	if ( mFrames[NF].Spr.size() != (eeUint)mSubFrames )
		mFrames[NF].Spr.resize( mSubFrames );

	mFrames[NF].Spr[NSF] = SubTexture;

	return true;
}

eeUint cSprite::AddFrame( cSubTexture * SubTexture ) {
	eeUint id = FramePos();

	AddSubFrame( SubTexture, id, mCurrentSubFrame );

	return id;
}

eeUint cSprite::AddFrame(const Uint32& TexId, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeFloat& offSetX, const eeFloat& offSetY, const eeRecti& TexSector) {
	eeUint id = FramePos();

	if ( AddSubFrame( TexId, id, mCurrentSubFrame, DestWidth, DestHeight, offSetX, offSetY, TexSector ) )
		return id;

	return 0;
}

bool cSprite::AddSubFrame(const Uint32& TexId, const eeUint& NumFrame, const eeUint& NumSubFrame, const eeFloat& DestWidth, const eeFloat& DestHeight, const Int32& offSetX, const Int32& offSetY, const eeRecti& TexSector) {
	if ( !cTextureFactory::instance()->TextureIdExists( TexId ) )
		return false;

	cTexture * Tex = cTextureFactory::instance()->GetTexture( TexId );
	cSubTexture * S = cGlobalTextureAtlas::instance()->Add( eeNew( cSubTexture, () ) );

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

	AddSubFrame( S, NumFrame, NumSubFrame );

	return true;
}

void cSprite::Update() {
	Update( (eeFloat)cEngine::instance()->Elapsed() );
}

void cSprite::Update( const eeFloat& ElapsedTime ) {
	if ( mFrames.size() > 1 && !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) && 0 != ElapsedTime ) {
		eeUint Size		= (eeUint)mFrames.size() - 1;

		if ( mRepeations == 0 )
			return;

		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mfCurrentFrame += ( ( mAnimSpeed * ElapsedTime ) / 1000.f );
		else
			mfCurrentFrame -= ( ( mAnimSpeed * ElapsedTime ) / 1000.f );

		mCurrentFrame = (eeUint)mfCurrentFrame;

		if ( SPR_FGET( SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP ) ) {
			if ( mAnimTo == mCurrentFrame ) {
				mFlags &= ~SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP;

				GoToAndStop( mAnimTo );

				FireEvent( SPRITE_EVENT_END_ANIM_TO );

				return;
			}
		}

		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) && mfCurrentFrame >= Size + 1.0f ) {
			if ( mRepeations < 0 ) {
				mfCurrentFrame = 0.0f;
				mCurrentFrame = 0;
				FireEvent( SPRITE_EVENT_FIRST_FRAME );
			} else {
				if ( mRepeations == 0 ) {
					mfCurrentFrame = (eeFloat)Size;
					mCurrentFrame = Size;
					FireEvent( SPRITE_EVENT_LAST_FRAME );
				} else {
					mfCurrentFrame = 0.0f;
					mCurrentFrame = 0;
					mRepeations--;
					FireEvent( SPRITE_EVENT_FIRST_FRAME );
				}
			}
		} else if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) && mfCurrentFrame < 0.0f ) {
			if ( mRepeations < 0 ) {
				mfCurrentFrame = Size + 1.0f;
				mCurrentFrame = Size;
				FireEvent( SPRITE_EVENT_LAST_FRAME );
			} else {
				if ( mRepeations == 0 ) {
					mfCurrentFrame = 0.0f;
					mCurrentFrame = 0;
					FireEvent( SPRITE_EVENT_FIRST_FRAME );
				} else {
					mfCurrentFrame = Size + 1.0f;
					mCurrentFrame = Size;
					mRepeations--;
					FireEvent( SPRITE_EVENT_LAST_FRAME );
				}
			}
		}

		if ( mfCurrentFrame < 0.0f ) {
			if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) ) {
				mfCurrentFrame = 0.0f;
				mCurrentFrame = 0;
				FireEvent( SPRITE_EVENT_FIRST_FRAME );
			} else {
				mfCurrentFrame = (eeFloat)Size;
				mCurrentFrame = Size;
				FireEvent( SPRITE_EVENT_LAST_FRAME );
			}
		}
	}
}

eeUint cSprite::GetEndFrame() {
	if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) ) {
		return 0;
	} else {
		return (eeUint)mFrames.size() - 1;
	}
}

void cSprite::SetReverseFromStart() {
	if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
		mFlags |= SPRITE_FLAG_REVERSE_ANIM;

	eeUint Size = (eeUint)mFrames.size() - 1;

	mfCurrentFrame = (eeFloat)Size;
	mCurrentFrame = Size;
}

void cSprite::Draw( const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect ) {
	if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
		Update();

	cSubTexture * S = GetCurrentSubTexture();

	if ( S == NULL )
		return;

	if ( NULL == mVertexColors )
		S->Draw( mPos.x, mPos.y, mColor, mAngle, mScale, Blend, Effect, 0 != SPR_FGET( SPRITE_FLAG_SCALE_CENTERED ) );
	else
		S->Draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[3], Blend, Effect, 0 != SPR_FGET( SPRITE_FLAG_SCALE_CENTERED ) );
}

void cSprite::Draw() {
	Draw( mBlend, mEffect );
}

void cSprite::Draw( const EE_BLEND_MODE& Blend ) {
	Draw( Blend, mEffect );
}

void cSprite::Draw( const EE_RENDER_MODE& Effect ) {
	Draw( mBlend, Effect );
}

eeUint cSprite::GetFrame( const eeUint& FrameNum ) {
	eeUint FN;

	if ( FrameNum >= mFrames.size() )
		FN = mCurrentFrame;
	else
		FN = FrameNum;

	return FN;
}

eeUint cSprite::GetSubFrame( const eeUint& SubFrame ) {
	eeUint SFN;

	if ( SubFrame >= mSubFrames )
		SFN = mCurrentSubFrame;
	else
		SFN = SubFrame;

	return SFN;
}

Int32 cSprite::OffsetX() {
	cSubTexture* S = GetCurrentSubTexture();

	if ( S != NULL )
		return S->OffsetX();

	return 0;
}

void cSprite::OffsetX( const Int32& offsetx ) {
	cSubTexture* S = GetCurrentSubTexture();

	if ( S != NULL )
		S->OffsetX( offsetx );
}

Int32 cSprite::OffsetY() {
	cSubTexture* S = GetCurrentSubTexture();

	if ( S != NULL )
		return S->OffsetY();

	return 0;
}

void cSprite::OffsetY( const Int32& offsety ) {
	cSubTexture* S = GetCurrentSubTexture();

	if ( S != NULL )
		S->OffsetY( offsety );
}

void cSprite::Offset( const eeVector2i& offset ) {
	cSubTexture* S = GetCurrentSubTexture();

	if ( S != NULL ) {
		S->OffsetX( offset.x );
		S->OffsetY( offset.y );
	}
}

void cSprite::Width( const eeFloat& Width, const eeUint& FrameNum, const eeUint& SubFrame ) {
	mFrames[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestWidth( Width );
}

void cSprite::Width( const eeFloat& Width ) {
	mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->DestWidth( Width );
}

eeFloat cSprite::Width( const eeUint& FrameNum, const eeUint& SubFrame ) {
	return mFrames[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestWidth();
}

eeFloat cSprite::Width() {
	return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->DestWidth();
}

void cSprite::Height( const eeFloat& Height, const eeUint& FrameNum, const eeUint& SubFrame ) {
	mFrames[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestHeight( Height );
}

void cSprite::Height( const eeFloat& Height ) {
	mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->DestHeight( Height );
}

eeFloat cSprite::Height( const eeUint& FrameNum, const eeUint& SubFrame ) {
	return mFrames[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestHeight();
}

eeFloat cSprite::Height() {
	return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->DestHeight();
}

void cSprite::SetRepeations( const int& Repeations ) {
	mRepeations = Repeations;
}

void cSprite::AutoAnimate( const bool& Autoanim ) {
	if ( Autoanim ) {
		if ( !SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags |= SPRITE_FLAG_AUTO_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags &= ~SPRITE_FLAG_AUTO_ANIM;
	}

}

bool cSprite::AutoAnimate() const {
	return 0 != SPR_FGET( SPRITE_FLAG_AUTO_ANIM );
}

eeQuad2f cSprite::GetQuad() {
	if ( mFrames.size() ) {
		cSubTexture* S = GetCurrentSubTexture();
		eeRectf TmpR;

		if ( SPR_FGET( SPRITE_FLAG_SCALE_CENTERED ) ) {
			if ( mScale == 1.0f )
				TmpR = eeRectf( mPos.x, mPos.y, mPos.x + S->DestWidth(), mPos.y + S->DestHeight() );
			else {
				eeFloat halfW = S->DestWidth() * 0.5f;
				eeFloat halfH = S->DestHeight() * 0.5f;
				TmpR = eeRectf( mPos.x + halfW - halfW * mScale, mPos.y + halfH - halfH * mScale, mPos.x + halfW + halfW * mScale, mPos.y + halfH + halfH * mScale );
			}
		} else {
			TmpR = eeRectf( mPos.x, mPos.y, mPos.x + S->DestWidth() * mScale, mPos.y + S->DestHeight() * mScale );
		}

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

cSubTexture* cSprite::GetCurrentSubTexture() {
	if ( mFrames.size() )
		return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ];

	return NULL;
}

cSubTexture * cSprite::GetSubTexture( const eeUint& frame ) {
	if ( frame < mFrames.size() )
		return mFrames[ frame ].Spr[ mCurrentSubFrame ];

	return NULL;
}

cSubTexture * cSprite::GetSubTexture( const eeUint& frame, const eeUint& SubFrame ) {
	if ( frame < mFrames.size() )
		return mFrames[ frame ].Spr[ SubFrame ];

	return NULL;
}

void cSprite::X( const eeFloat& X ) {
	mPos.x = X;
}

eeFloat cSprite::X() const {
	return mPos.x;
}

void cSprite::Y( const eeFloat& Y ) {
	mPos.y = Y;
}

eeFloat cSprite::Y() const {
	return mPos.y;
}

void cSprite::Angle( const eeFloat& Angle) {
	mAngle = Angle;
}

eeFloat cSprite::Angle() const {
	return mAngle;
}

void cSprite::Scale( const eeFloat& Scale ) {
	mScale = Scale;
}

eeFloat cSprite::Scale() const {
	return mScale;
}

void cSprite::AnimSpeed( const eeFloat& AnimSpeed ) {
	mAnimSpeed = AnimSpeed;
}

eeFloat cSprite::AnimSpeed() const {
	return mAnimSpeed;
}

bool cSprite::AnimPaused() const {
	return 0 != SPR_FGET( SPRITE_FLAG_ANIM_PAUSED );
}

void cSprite::AnimPaused( const bool& Pause )	{
	if ( Pause ) {
		if ( !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags |= SPRITE_FLAG_ANIM_PAUSED;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags &= ~SPRITE_FLAG_ANIM_PAUSED;
	}
}

void cSprite::Color( const eeColorA& Color) {
	mColor = Color;
}

const eeColorA& cSprite::Color() const {
	return mColor;
}

void cSprite::Alpha( const Uint8& Alpha ) {
	mColor.Alpha = Alpha;
}

const Uint8& cSprite::Alpha() const {
	return mColor.Alpha;
}

bool cSprite::ScaleCentered() const {
	return 0 != SPR_FGET( SPRITE_FLAG_SCALE_CENTERED );
}

void cSprite::ScaleCentered( const bool& ScaleCentered ) {
	if ( ScaleCentered ) {
		if ( !SPR_FGET( SPRITE_FLAG_SCALE_CENTERED ) )
			mFlags |= SPRITE_FLAG_SCALE_CENTERED;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_SCALE_CENTERED ) )
			mFlags &= ~SPRITE_FLAG_SCALE_CENTERED;
	}
}

const eeUint& cSprite::CurrentFrame() const {
	return mCurrentFrame;
}

const eeFloat& cSprite::ExactCurrentFrame() const {
	return mfCurrentFrame;
}

void cSprite::ExactCurrentFrame( const eeFloat& CurrentFrame ) {
	mfCurrentFrame = CurrentFrame;
}

const eeUint& cSprite::CurrentSubFrame() const {
	return mCurrentSubFrame;
}

void cSprite::RenderType( const EE_RENDER_MODE& Effect ) {
	mEffect = Effect;
}

const EE_RENDER_MODE& cSprite::RenderType() const {
	return mEffect;
}

void cSprite::BlendMode( const EE_BLEND_MODE& Blend ) {
	mBlend = Blend;
}

const EE_BLEND_MODE& cSprite::BlendMode() const {
	return mBlend;
}

void cSprite::ReverseAnim( const bool& Reverse ) {
	if ( Reverse ) {
		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags |= SPRITE_FLAG_REVERSE_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags &= ~SPRITE_FLAG_REVERSE_ANIM;
	}
}

bool cSprite::ReverseAnim() const {
	return 0 != SPR_FGET( SPRITE_FLAG_REVERSE_ANIM );
}

Uint32 cSprite::GetNumFrames() {
	return (Uint32)mFrames.size();
}

void cSprite::GoToAndPlay( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mCurrentFrame	= GoTo;
		mfCurrentFrame	= (eeFloat)GoTo;

		AnimPaused( false );
	}
}

void cSprite::GoToAndStop( Uint32 GoTo ) {
	GoToAndPlay( GoTo );
	AnimPaused( true );
}

void cSprite::AnimToFrameAndStop( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mAnimTo = GoTo;

		mFlags |= SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP;

		AnimPaused( false );
	}
}

void cSprite::SetEventsCallback( const SpriteCallback& Cb ) {
	mCb = Cb;
}

void cSprite::ClearCallback() {
	mCb.Reset();
}

void cSprite::FireEvent( const Uint32& Event ) {
	if ( SPR_FGET( SPRITE_FLAG_EVENTS_ENABLED ) && mCb.IsSet() ) {
		mCb( Event, this );
	}
}

}}
