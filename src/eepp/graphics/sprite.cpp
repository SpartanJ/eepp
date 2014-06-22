#include <eepp/graphics/sprite.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/math/originpoint.hpp>

using namespace EE::Window;

#define SPR_FGET( Flag )	( mFlags & Flag )

namespace EE { namespace Graphics {

Sprite::Sprite() :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f, 1.f ),
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
	mAnimTo( 0 ),
	mUserData( NULL )
{
	mCb.Reset();
}

Sprite::Sprite( const std::string& name, const std::string& extension, TextureAtlas * SearchInTextureAtlas )  :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f, 1.f ),
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
	mAnimTo( 0 ),
	mUserData( NULL )
{
	mCb.Reset();
	AddFramesByPattern( name, extension, SearchInTextureAtlas );
}

Sprite::Sprite( SubTexture * SubTexture ) :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f, 1.f ),
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
	mAnimTo( 0 ),
	mUserData( NULL )
{
	mCb.Reset();
	CreateStatic( SubTexture );
}

Sprite::Sprite( const Uint32& TexId, const Sizef &DestSize, const Vector2i &Offset, const Recti& TexSector ) :
	mFlags( SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_EVENTS_ENABLED ),
	mPos(),
	mAngle( 0.f ),
	mScale( 1.f, 1.f ),
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
	mAnimTo( 0 ),
	mUserData( NULL )
{
	mCb.Reset();
	CreateStatic( TexId, DestSize, Offset, TexSector );
}

Sprite::~Sprite() {
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

Sprite& Sprite::operator =( const Sprite& Other ) {
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
		mVertexColors		= eeNewArray( ColorA, 4 );
		mVertexColors[0]	= Other.mVertexColors[0];
		mVertexColors[1]	= Other.mVertexColors[1];
		mVertexColors[2]	= Other.mVertexColors[2];
		mVertexColors[3]	= Other.mVertexColors[3];
	} else {
		mVertexColors		= NULL;
	}

	return *this;
}

Sprite * Sprite::Copy() {
	Sprite * Spr = eeNew( Sprite, () );

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
		Spr->mVertexColors		= eeNewArray( ColorA, 4 );
		Spr->mVertexColors[0]	= mVertexColors[0];
		Spr->mVertexColors[1]	= mVertexColors[1];
		Spr->mVertexColors[2]	= mVertexColors[2];
		Spr->mVertexColors[3]	= mVertexColors[3];
	} else {
		Spr->mVertexColors		= NULL;
	}

	return Spr;
}

void Sprite::ClearFrame() {
	for ( unsigned int i = 0; i < mFrames.size(); i++ )
		mFrames[i].Spr.clear();

	mFrames.clear();
}

void Sprite::Reset() {
	ClearFrame();

	mFlags				= SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_EVENTS_ENABLED;

	mAnimSpeed			= 16.f;
	mScale				= Vector2f::One;
	mRepeations			= -1;

	mAngle				= 0;
	mColor				= ColorA(255, 255, 255, 255);

	mBlend				= ALPHA_NORMAL;
	mEffect				= RN_NORMAL;

	mCurrentFrame		= 0;
	mCurrentSubFrame 	= 0;
	mfCurrentFrame		= 0.f;
	mSubFrames			= 1;
	mAnimTo				= 0;

	DisableVertexColors();
}

void Sprite::CurrentFrame ( unsigned int CurFrame ) {
	if ( CurFrame )
		CurFrame--;

	mfCurrentFrame = CurFrame;
	mCurrentFrame = (unsigned int)CurFrame;

	if ( mfCurrentFrame >= mFrames.size() ) {
		mfCurrentFrame = (Float)mFrames.size() - 1;
		mCurrentFrame = (unsigned int)mFrames.size() - 1;
	}

	if ( mfCurrentFrame < 0 ) {
		mfCurrentFrame = 0.0f;
		mCurrentFrame = 0;
	}
}

void Sprite::CurrentSubFrame( const unsigned int& CurSubFrame ) {
	if ( CurSubFrame < mSubFrames )
		mCurrentSubFrame = CurSubFrame;
}

Quad2f Sprite::GetQuad() {
	SubTexture * S;

	if ( mFrames.size() && ( S = GetCurrentSubTexture() ) ) {
		Rectf TmpR( mPos.x,
					  mPos.y,
					  mPos.x + S->DestSize().x,
					  mPos.y + S->DestSize().y
					);

		Quad2f Q = Quad2f( Vector2f( TmpR.Left, TmpR.Top ),
							   Vector2f( TmpR.Left, TmpR.Bottom ),
							   Vector2f( TmpR.Right, TmpR.Bottom ),
							   Vector2f( TmpR.Right, TmpR.Top )
					);

		Vector2f Center;

		if ( mOrigin.OriginType == OriginPoint::OriginCenter ) {
			Center	= TmpR.Center();
		} else if ( mOrigin.OriginType == OriginPoint::OriginTopLeft ) {
			Center	= mPos;
		} else {
			Center	+= mPos;
		}

		switch ( mEffect ) {
			case RN_NORMAL:
			case RN_MIRROR:
			case RN_FLIP:
			case RN_FLIPMIRROR:
				break;
			case RN_ISOMETRIC:
				Q.V[0].x += ( TmpR.Right - TmpR.Left );
				Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[3].x += ( TmpR.Right - TmpR.Left );
				Q.V[3].y += ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				break;
			case RN_ISOMETRICVERTICAL:
				Q.V[0].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				break;
			case RN_ISOMETRICVERTICALNEGATIVE:
				Q.V[2].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[3].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				break;
		}

		Q.Rotate( mAngle, Center );
		Q.Scale( mScale, Center );

		return Q;
	}

	return Quad2f();
}

eeAABB Sprite::GetAABB() {
	eeAABB TmpR;
	SubTexture * S;

	if ( mFrames.size() && ( S = GetCurrentSubTexture() ) ) {
		if ( mAngle != 0 || mEffect >= 4 ) {
			return GetQuad().ToAABB();
		} else { // The method used if mAngle != 0 works for mAngle = 0, but i prefer to use the faster way
			TmpR = Rectf( mPos.x, mPos.y, mPos.x + S->DestSize().x, mPos.y + S->DestSize().y );

			Vector2f Center;

			if ( mOrigin.OriginType == OriginPoint::OriginCenter ) {
				Center	= TmpR.Center();
			} else if ( mOrigin.OriginType == OriginPoint::OriginTopLeft ) {
				Center	= mPos;
			} else {
				Center	= mPos + mOrigin;
			}

			TmpR.Scale( mScale, Center );
		}
	}

	return TmpR;
}

const Vector2f Sprite::Position() const {
	return mPos;
}

void Sprite::Position(const Float& x, const Float& y) {
	mPos.x = x;
	mPos.y = y;
}

void Sprite::Position( const Vector2f& NewPos ) {
	mPos = NewPos;
}

void Sprite::UpdateVertexColors( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 ) {
	if ( NULL == mVertexColors )
		mVertexColors		= eeNewArray( ColorA, 4 );

	mVertexColors[0]	= Color0;
	mVertexColors[1]	= Color1;
	mVertexColors[2]	= Color2;
	mVertexColors[3]	= Color3;
}

void Sprite::DisableVertexColors() {
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

unsigned int Sprite::FramePos() {
	mFrames.push_back( cFrame() );
	return (unsigned int)mFrames.size() - 1;
}

bool Sprite::CreateStatic( SubTexture * SubTexture ) {
	Reset();

	AddFrame( SubTexture );

	return true;
}

bool Sprite::CreateStatic( const Uint32& TexId, const Sizef& DestSize, const Vector2i& Offset, const Recti& TexSector ) {
	if ( TextureFactory::instance()->TextureIdExists( TexId ) ) {
		Reset();

		AddFrame( TexId, DestSize, Offset, TexSector );

		return true;
	}

	return false;
}

void Sprite::CreateAnimation( const unsigned int& SubFramesNum ) {
	Reset();

	if ( SubFramesNum < 1 )
		mSubFrames = 1;
	else
		mSubFrames = SubFramesNum;
}

bool Sprite::AddFrames( const std::vector<SubTexture*> SubTextures ) {
	if ( SubTextures.size() ) {
		for ( unsigned int i = 0; i < SubTextures.size(); i++ ) {
			if ( NULL != SubTextures[i] ) {
				AddFrame( SubTextures[i] );
			}
		}

		return true;
	}

	return false;
}

bool Sprite::AddFramesByPatternId( const Uint32& SubTextureId, const std::string& extension, TextureAtlas * SearchInTextureAtlas ) {
	std::vector<SubTexture*> SubTextures = TextureAtlasManager::instance()->GetSubTexturesByPatternId( SubTextureId, extension, SearchInTextureAtlas );

	if ( SubTextures.size() ) {
		AddFrames( SubTextures );

		return true;
	}

	eePRINTL( "Sprite::AddFramesByPatternId: Couldn't find any pattern with Id: %d", SubTextureId );

	return false;
}

bool Sprite::AddFramesByPattern( const std::string& name, const std::string& extension, TextureAtlas * SearchInTextureAtlas ) {
	std::vector<SubTexture*> SubTextures = TextureAtlasManager::instance()->GetSubTexturesByPattern( name, extension, SearchInTextureAtlas );

	if ( SubTextures.size() ) {
		AddFrames( SubTextures );

		return true;
	}

	eePRINTL( "Sprite::AddFramesByPattern: Couldn't find any pattern with: %s", name.c_str() );

	return false;
}

bool Sprite::AddSubFrame( SubTexture * SubTexture, const unsigned int& NumFrame, const unsigned int& NumSubFrame ) {
	unsigned int NF, NSF;

	if ( NumFrame >= mFrames.size() )
		NF = 0;
	else
		NF = NumFrame;

	if ( NumSubFrame >= mSubFrames )
		NSF = 0;
	else
		NSF = NumSubFrame;

	if ( NF <= mFrames.size() ) {
		if ( mFrames[NF].Spr.size() != (unsigned int)mSubFrames )
			mFrames[NF].Spr.resize( mSubFrames );

		mFrames[NF].Spr[NSF] = SubTexture;

		return true;
	}

	return false;
}

unsigned int Sprite::AddFrame( SubTexture * SubTexture ) {
	unsigned int id = FramePos();

	AddSubFrame( SubTexture, id, mCurrentSubFrame );

	return id;
}

unsigned int Sprite::AddFrame( const Uint32& TexId, const Sizef& DestSize, const Vector2i& Offset, const Recti& TexSector ) {
	unsigned int id = FramePos();

	if ( AddSubFrame( TexId, id, mCurrentSubFrame, DestSize, Offset, TexSector ) )
		return id;

	return 0;
}

bool Sprite::AddSubFrame(const Uint32& TexId, const unsigned int& NumFrame, const unsigned int& NumSubFrame, const Sizef& DestSize, const Vector2i& Offset, const Recti& TexSector) {
	if ( !TextureFactory::instance()->TextureIdExists( TexId ) )
		return false;

	Texture * Tex = TextureFactory::instance()->GetTexture( TexId );
	SubTexture * S = GlobalTextureAtlas::instance()->Add( eeNew( SubTexture, () ) );

	S->Texture( TexId );

	if ( TexSector.Right > 0 && TexSector.Bottom > 0 )
		S->SrcRect( TexSector );
	else
		S->SrcRect( Recti( 0, 0, (Int32)Tex->ImgWidth(), (Int32)Tex->ImgHeight() ) );

	Sizef destSize( DestSize );

	if ( destSize.x <= 0 ) {
		destSize.x = static_cast<Float> ( S->SrcRect().Right - S->SrcRect().Left );
	}

	if ( destSize.y <= 0 ) {
		destSize.y = static_cast<Float> ( S->SrcRect().Bottom - S->SrcRect().Top );
	}

	S->DestSize( destSize );
	S->Offset( Offset );

	AddSubFrame( S, NumFrame, NumSubFrame );

	return true;
}

void Sprite::Update() {
	Update( Engine::instance()->Elapsed() );
}

void Sprite::Update( const Time& ElapsedTime ) {
	if ( mFrames.size() > 1 && !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) && Time::Zero != ElapsedTime ) {
		unsigned int Size		= (unsigned int)mFrames.size() - 1;

		if ( mRepeations == 0 )
			return;

		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mfCurrentFrame += mAnimSpeed * ElapsedTime.AsSeconds();
		else
			mfCurrentFrame -= mAnimSpeed * ElapsedTime.AsSeconds();

		mCurrentFrame = (unsigned int)mfCurrentFrame;

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
					mfCurrentFrame = (Float)Size;
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
				mfCurrentFrame = (Float)Size;
				mCurrentFrame = Size;
				FireEvent( SPRITE_EVENT_LAST_FRAME );
			}
		}
	}
}

unsigned int Sprite::GetEndFrame() {
	if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) ) {
		return 0;
	} else {
		return (unsigned int)mFrames.size() - 1;
	}
}

void Sprite::SetReverseFromStart() {
	if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
		mFlags |= SPRITE_FLAG_REVERSE_ANIM;

	unsigned int Size = (unsigned int)mFrames.size() - 1;

	mfCurrentFrame = (Float)Size;
	mCurrentFrame = Size;
}

void Sprite::Draw( const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect ) {
	if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
		Update();

	SubTexture * S = GetCurrentSubTexture();

	if ( S == NULL )
		return;

	if ( NULL == mVertexColors )
		S->Draw( mPos.x, mPos.y, mColor, mAngle, mScale, Blend, Effect, mOrigin );
	else
		S->Draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[3], Blend, Effect, mOrigin );
}

void Sprite::Draw() {
	Draw( mBlend, mEffect );
}

void Sprite::Draw( const EE_BLEND_MODE& Blend ) {
	Draw( Blend, mEffect );
}

void Sprite::Draw( const EE_RENDER_MODE& Effect ) {
	Draw( mBlend, Effect );
}

unsigned int Sprite::GetFrame( const unsigned int& FrameNum ) {
	unsigned int FN;

	if ( FrameNum >= mFrames.size() )
		FN = mCurrentFrame;
	else
		FN = FrameNum;

	return FN;
}

unsigned int Sprite::GetSubFrame( const unsigned int& SubFrame ) {
	unsigned int SFN;

	if ( SubFrame >= mSubFrames )
		SFN = mCurrentSubFrame;
	else
		SFN = SubFrame;

	return SFN;
}

Vector2i Sprite::Offset() {
	SubTexture* S = GetCurrentSubTexture();

	if ( S != NULL )
		return S->Offset();

	return Vector2i();
}

void Sprite::Offset( const Vector2i& offset ) {
	SubTexture* S = GetCurrentSubTexture();

	if ( S != NULL ) {
		S->Offset( offset );
	}
}

void Sprite::Size( const Sizef& Size, const unsigned int& FrameNum, const unsigned int& SubFrame ) {
	mFrames[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestSize( Size );
}

void Sprite::Size( const Sizef& Size ) {
	mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->DestSize( Size );
}

Sizef Sprite::Size( const unsigned int& FrameNum, const unsigned int& SubFrame ) {
	return mFrames[ GetFrame(FrameNum) ].Spr[ GetSubFrame(SubFrame) ]->DestSize();
}

Sizef Sprite::Size() {
	return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->DestSize();
}

void Sprite::SetRepeations( const int& Repeations ) {
	mRepeations = Repeations;
}

void Sprite::AutoAnimate( const bool& Autoanim ) {
	if ( Autoanim ) {
		if ( !SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags |= SPRITE_FLAG_AUTO_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags &= ~SPRITE_FLAG_AUTO_ANIM;
	}

}

bool Sprite::AutoAnimate() const {
	return 0 != SPR_FGET( SPRITE_FLAG_AUTO_ANIM );
}

SubTexture* Sprite::GetCurrentSubTexture() {
	if ( mFrames.size() )
		return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ];

	return NULL;
}

SubTexture * Sprite::GetSubTexture( const unsigned int& frame ) {
	if ( frame < mFrames.size() )
		return mFrames[ frame ].Spr[ mCurrentSubFrame ];

	return NULL;
}

SubTexture * Sprite::GetSubTexture( const unsigned int& frame, const unsigned int& SubFrame ) {
	if ( frame < mFrames.size() )
		return mFrames[ frame ].Spr[ SubFrame ];

	return NULL;
}

void Sprite::X( const Float& X ) {
	mPos.x = X;
}

Float Sprite::X() const {
	return mPos.x;
}

void Sprite::Y( const Float& Y ) {
	mPos.y = Y;
}

Float Sprite::Y() const {
	return mPos.y;
}

void Sprite::Angle( const Float& Angle) {
	mAngle = Angle;
}

Float Sprite::Angle() const {
	return mAngle;
}

void Sprite::Scale( const Float& Scale ) {
	this->Scale( Vector2f( Scale, Scale ) );
}

void Sprite::Scale( const Vector2f& Scale ) {
	mScale = Scale;
}

const Vector2f& Sprite::Scale() const {
	return mScale;
}

void Sprite::AnimSpeed( const Float& AnimSpeed ) {
	mAnimSpeed = AnimSpeed;
}

Float Sprite::AnimSpeed() const {
	return mAnimSpeed;
}

bool Sprite::AnimPaused() const {
	return 0 != SPR_FGET( SPRITE_FLAG_ANIM_PAUSED );
}

void Sprite::AnimPaused( const bool& Pause )	{
	if ( Pause ) {
		if ( !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags |= SPRITE_FLAG_ANIM_PAUSED;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags &= ~SPRITE_FLAG_ANIM_PAUSED;
	}
}

void Sprite::Color( const ColorA& Color) {
	mColor = Color;
}

const ColorA& Sprite::Color() const {
	return mColor;
}

void Sprite::Alpha( const Uint8& Alpha ) {
	mColor.Alpha = Alpha;
}

const Uint8& Sprite::Alpha() const {
	return mColor.Alpha;
}

const unsigned int& Sprite::CurrentFrame() const {
	return mCurrentFrame;
}

const Float& Sprite::ExactCurrentFrame() const {
	return mfCurrentFrame;
}

void Sprite::ExactCurrentFrame( const Float& CurrentFrame ) {
	mfCurrentFrame = CurrentFrame;
}

const unsigned int& Sprite::CurrentSubFrame() const {
	return mCurrentSubFrame;
}

void Sprite::RenderMode( const EE_RENDER_MODE& Effect ) {
	mEffect = Effect;
}

const EE_RENDER_MODE& Sprite::RenderMode() const {
	return mEffect;
}

void Sprite::BlendMode( const EE_BLEND_MODE& Blend ) {
	mBlend = Blend;
}

const EE_BLEND_MODE& Sprite::BlendMode() const {
	return mBlend;
}

void Sprite::ReverseAnim( const bool& Reverse ) {
	if ( Reverse ) {
		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags |= SPRITE_FLAG_REVERSE_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags &= ~SPRITE_FLAG_REVERSE_ANIM;
	}
}

bool Sprite::ReverseAnim() const {
	return 0 != SPR_FGET( SPRITE_FLAG_REVERSE_ANIM );
}

Uint32 Sprite::GetNumFrames() {
	return (Uint32)mFrames.size();
}

void Sprite::GoToAndPlay( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mCurrentFrame	= GoTo;
		mfCurrentFrame	= (Float)GoTo;

		AnimPaused( false );
	}
}

void Sprite::GoToAndStop( Uint32 GoTo ) {
	GoToAndPlay( GoTo );
	AnimPaused( true );
}

void Sprite::AnimToFrameAndStop( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mAnimTo = GoTo;

		mFlags |= SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP;

		AnimPaused( false );
	}
}

void Sprite::SetEventsCallback(const SpriteCallback& Cb , void * UserData ) {
	mCb			= Cb;
	mUserData	= UserData;
}

void Sprite::ClearCallback() {
	mCb.Reset();
}

void Sprite::FireEvent( const Uint32& Event ) {
	if ( SPR_FGET( SPRITE_FLAG_EVENTS_ENABLED ) && mCb.IsSet() ) {
		mCb( Event, this, mUserData );
	}
}

void Sprite::Origin( const OriginPoint& origin ) {
	mOrigin = origin;
}

const OriginPoint& Sprite:: Origin() const {
	return mOrigin;
}

void Sprite::Rotate( const Float& angle ) {
	mAngle += angle;
}

}}
