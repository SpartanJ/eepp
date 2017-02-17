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
	addFramesByPattern( name, extension, SearchInTextureAtlas );
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
	createStatic( SubTexture );
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
	createStatic( TexId, DestSize, Offset, TexSector );
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

Sprite * Sprite::copy() {
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

void Sprite::clearFrame() {
	for ( unsigned int i = 0; i < mFrames.size(); i++ )
		mFrames[i].Spr.clear();

	mFrames.clear();
}

void Sprite::reset() {
	clearFrame();

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

	disableVertexColors();
}

void Sprite::currentFrame ( unsigned int CurFrame ) {
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

void Sprite::currentSubFrame( const unsigned int& CurSubFrame ) {
	if ( CurSubFrame < mSubFrames )
		mCurrentSubFrame = CurSubFrame;
}

Quad2f Sprite::getQuad() {
	SubTexture * S;

	if ( mFrames.size() && ( S = getCurrentSubTexture() ) ) {
		Rectf TmpR( mPos.x,
					  mPos.y,
					  mPos.x + S->destSize().x,
					  mPos.y + S->destSize().y
					);

		Quad2f Q = Quad2f( Vector2f( TmpR.Left, TmpR.Top ),
							   Vector2f( TmpR.Left, TmpR.Bottom ),
							   Vector2f( TmpR.Right, TmpR.Bottom ),
							   Vector2f( TmpR.Right, TmpR.Top )
					);

		Vector2f Center;

		if ( mOrigin.OriginType == OriginPoint::OriginCenter ) {
			Center	= TmpR.getCenter();
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

		Q.rotate( mAngle, Center );
		Q.scale( mScale, Center );

		return Q;
	}

	return Quad2f();
}

eeAABB Sprite::getAABB() {
	eeAABB TmpR;
	SubTexture * S;

	if ( mFrames.size() && ( S = getCurrentSubTexture() ) ) {
		if ( mAngle != 0 || mEffect >= 4 ) {
			return getQuad().toAABB();
		} else { // The method used if mAngle != 0 works for mAngle = 0, but i prefer to use the faster way
			TmpR = Rectf( mPos.x, mPos.y, mPos.x + S->destSize().x, mPos.y + S->destSize().y );

			Vector2f Center;

			if ( mOrigin.OriginType == OriginPoint::OriginCenter ) {
				Center	= TmpR.getCenter();
			} else if ( mOrigin.OriginType == OriginPoint::OriginTopLeft ) {
				Center	= mPos;
			} else {
				Center	= mPos + mOrigin;
			}

			TmpR.scale( mScale, Center );
		}
	}

	return TmpR;
}

const Vector2f Sprite::position() const {
	return mPos;
}

void Sprite::position(const Float& x, const Float& y) {
	mPos.x = x;
	mPos.y = y;
}

void Sprite::position( const Vector2f& NewPos ) {
	mPos = NewPos;
}

void Sprite::updateVertexColors( const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3 ) {
	if ( NULL == mVertexColors )
		mVertexColors		= eeNewArray( ColorA, 4 );

	mVertexColors[0]	= Color0;
	mVertexColors[1]	= Color1;
	mVertexColors[2]	= Color2;
	mVertexColors[3]	= Color3;
}

void Sprite::disableVertexColors() {
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

unsigned int Sprite::framePos() {
	mFrames.push_back( Frame() );
	return (unsigned int)mFrames.size() - 1;
}

bool Sprite::createStatic( SubTexture * SubTexture ) {
	reset();

	addFrame( SubTexture );

	return true;
}

bool Sprite::createStatic( const Uint32& TexId, const Sizef& DestSize, const Vector2i& Offset, const Recti& TexSector ) {
	if ( TextureFactory::instance()->textureIdExists( TexId ) ) {
		reset();

		addFrame( TexId, DestSize, Offset, TexSector );

		return true;
	}

	return false;
}

void Sprite::createAnimation( const unsigned int& SubFramesNum ) {
	reset();

	if ( SubFramesNum < 1 )
		mSubFrames = 1;
	else
		mSubFrames = SubFramesNum;
}

bool Sprite::addFrames( const std::vector<SubTexture*> SubTextures ) {
	if ( SubTextures.size() ) {
		for ( unsigned int i = 0; i < SubTextures.size(); i++ ) {
			if ( NULL != SubTextures[i] ) {
				addFrame( SubTextures[i] );
			}
		}

		return true;
	}

	return false;
}

bool Sprite::addFramesByPatternId( const Uint32& SubTextureId, const std::string& extension, TextureAtlas * SearchInTextureAtlas ) {
	std::vector<SubTexture*> SubTextures = TextureAtlasManager::instance()->getSubTexturesByPatternId( SubTextureId, extension, SearchInTextureAtlas );

	if ( SubTextures.size() ) {
		addFrames( SubTextures );

		return true;
	}

	eePRINTL( "Sprite::AddFramesByPatternId: Couldn't find any pattern with Id: %d", SubTextureId );

	return false;
}

bool Sprite::addFramesByPattern( const std::string& name, const std::string& extension, TextureAtlas * SearchInTextureAtlas ) {
	std::vector<SubTexture*> SubTextures = TextureAtlasManager::instance()->getSubTexturesByPattern( name, extension, SearchInTextureAtlas );

	if ( SubTextures.size() ) {
		addFrames( SubTextures );

		return true;
	}

	eePRINTL( "Sprite::AddFramesByPattern: Couldn't find any pattern with: %s", name.c_str() );

	return false;
}

bool Sprite::addSubFrame( SubTexture * SubTexture, const unsigned int& NumFrame, const unsigned int& NumSubFrame ) {
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

unsigned int Sprite::addFrame( SubTexture * SubTexture ) {
	unsigned int id = framePos();

	addSubFrame( SubTexture, id, mCurrentSubFrame );

	return id;
}

unsigned int Sprite::addFrame( const Uint32& TexId, const Sizef& DestSize, const Vector2i& Offset, const Recti& TexSector ) {
	unsigned int id = framePos();

	if ( addSubFrame( TexId, id, mCurrentSubFrame, DestSize, Offset, TexSector ) )
		return id;

	return 0;
}

bool Sprite::addSubFrame(const Uint32& TexId, const unsigned int& NumFrame, const unsigned int& NumSubFrame, const Sizef& DestSize, const Vector2i& Offset, const Recti& TexSector) {
	if ( !TextureFactory::instance()->textureIdExists( TexId ) )
		return false;

	Texture * Tex = TextureFactory::instance()->getTexture( TexId );
	SubTexture * S = GlobalTextureAtlas::instance()->add( eeNew( SubTexture, () ) );

	S->texture( TexId );

	if ( TexSector.Right > 0 && TexSector.Bottom > 0 )
		S->srcRect( TexSector );
	else
		S->srcRect( Recti( 0, 0, (Int32)Tex->imgWidth(), (Int32)Tex->imgHeight() ) );

	Sizef destSize( DestSize );

	if ( destSize.x <= 0 ) {
		destSize.x = static_cast<Float> ( S->srcRect().Right - S->srcRect().Left );
	}

	if ( destSize.y <= 0 ) {
		destSize.y = static_cast<Float> ( S->srcRect().Bottom - S->srcRect().Top );
	}

	S->destSize( destSize );
	S->offset( Offset );

	addSubFrame( S, NumFrame, NumSubFrame );

	return true;
}

void Sprite::update() {
	update( Engine::instance()->getElapsed() );
}

void Sprite::update( const Time& ElapsedTime ) {
	if ( mFrames.size() > 1 && !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) && Time::Zero != ElapsedTime ) {
		unsigned int Size		= (unsigned int)mFrames.size() - 1;

		if ( mRepeations == 0 )
			return;

		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mfCurrentFrame += mAnimSpeed * ElapsedTime.asSeconds();
		else
			mfCurrentFrame -= mAnimSpeed * ElapsedTime.asSeconds();

		mCurrentFrame = (unsigned int)mfCurrentFrame;

		if ( SPR_FGET( SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP ) ) {
			if ( mAnimTo == mCurrentFrame ) {
				mFlags &= ~SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP;

				goToAndStop( mAnimTo );

				fireEvent( SPRITE_EVENT_END_ANIM_TO );

				return;
			}
		}

		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) && mfCurrentFrame >= Size + 1.0f ) {
			if ( mRepeations < 0 ) {
				mfCurrentFrame = 0.0f;
				mCurrentFrame = 0;
				fireEvent( SPRITE_EVENT_FIRST_FRAME );
			} else {
				if ( mRepeations == 0 ) {
					mfCurrentFrame = (Float)Size;
					mCurrentFrame = Size;
					fireEvent( SPRITE_EVENT_LAST_FRAME );
				} else {
					mfCurrentFrame = 0.0f;
					mCurrentFrame = 0;
					mRepeations--;
					fireEvent( SPRITE_EVENT_FIRST_FRAME );
				}
			}
		} else if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) && mfCurrentFrame < 0.0f ) {
			if ( mRepeations < 0 ) {
				mfCurrentFrame = Size + 1.0f;
				mCurrentFrame = Size;
				fireEvent( SPRITE_EVENT_LAST_FRAME );
			} else {
				if ( mRepeations == 0 ) {
					mfCurrentFrame = 0.0f;
					mCurrentFrame = 0;
					fireEvent( SPRITE_EVENT_FIRST_FRAME );
				} else {
					mfCurrentFrame = Size + 1.0f;
					mCurrentFrame = Size;
					mRepeations--;
					fireEvent( SPRITE_EVENT_LAST_FRAME );
				}
			}
		}

		if ( mfCurrentFrame < 0.0f ) {
			if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) ) {
				mfCurrentFrame = 0.0f;
				mCurrentFrame = 0;
				fireEvent( SPRITE_EVENT_FIRST_FRAME );
			} else {
				mfCurrentFrame = (Float)Size;
				mCurrentFrame = Size;
				fireEvent( SPRITE_EVENT_LAST_FRAME );
			}
		}
	}
}

unsigned int Sprite::getEndFrame() {
	if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) ) {
		return 0;
	} else {
		return (unsigned int)mFrames.size() - 1;
	}
}

void Sprite::setReverseFromStart() {
	if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
		mFlags |= SPRITE_FLAG_REVERSE_ANIM;

	unsigned int Size = (unsigned int)mFrames.size() - 1;

	mfCurrentFrame = (Float)Size;
	mCurrentFrame = Size;
}

void Sprite::draw( const EE_BLEND_MODE& Blend, const EE_RENDER_MODE& Effect ) {
	if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
		update();

	SubTexture * S = getCurrentSubTexture();

	if ( S == NULL )
		return;

	if ( NULL == mVertexColors )
		S->draw( mPos.x, mPos.y, mColor, mAngle, mScale, Blend, Effect, mOrigin );
	else
		S->draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[3], Blend, Effect, mOrigin );
}

void Sprite::draw() {
	draw( mBlend, mEffect );
}

void Sprite::draw( const EE_BLEND_MODE& Blend ) {
	draw( Blend, mEffect );
}

void Sprite::draw( const EE_RENDER_MODE& Effect ) {
	draw( mBlend, Effect );
}

unsigned int Sprite::getFrame( const unsigned int& FrameNum ) {
	unsigned int FN;

	if ( FrameNum >= mFrames.size() )
		FN = mCurrentFrame;
	else
		FN = FrameNum;

	return FN;
}

unsigned int Sprite::getSubFrame( const unsigned int& SubFrame ) {
	unsigned int SFN;

	if ( SubFrame >= mSubFrames )
		SFN = mCurrentSubFrame;
	else
		SFN = SubFrame;

	return SFN;
}

Vector2i Sprite::offset() {
	SubTexture* S = getCurrentSubTexture();

	if ( S != NULL )
		return S->offset();

	return Vector2i();
}

void Sprite::offset( const Vector2i& offset ) {
	SubTexture* S = getCurrentSubTexture();

	if ( S != NULL ) {
		S->offset( offset );
	}
}

void Sprite::size( const Sizef& Size, const unsigned int& FrameNum, const unsigned int& SubFrame ) {
	mFrames[ getFrame(FrameNum) ].Spr[ getSubFrame(SubFrame) ]->destSize( Size );
}

void Sprite::size( const Sizef& Size ) {
	mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->destSize( Size );
}

Sizef Sprite::size( const unsigned int& FrameNum, const unsigned int& SubFrame ) {
	return mFrames[ getFrame(FrameNum) ].Spr[ getSubFrame(SubFrame) ]->destSize();
}

Sizef Sprite::size() {
	return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ]->destSize();
}

void Sprite::setRepetitions( const int& Repeations ) {
	mRepeations = Repeations;
}

void Sprite::autoAnimate( const bool& Autoanim ) {
	if ( Autoanim ) {
		if ( !SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags |= SPRITE_FLAG_AUTO_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags &= ~SPRITE_FLAG_AUTO_ANIM;
	}

}

bool Sprite::autoAnimate() const {
	return 0 != SPR_FGET( SPRITE_FLAG_AUTO_ANIM );
}

SubTexture* Sprite::getCurrentSubTexture() {
	if ( mFrames.size() )
		return mFrames[ mCurrentFrame ].Spr[ mCurrentSubFrame ];

	return NULL;
}

SubTexture * Sprite::getSubTexture( const unsigned int& frame ) {
	if ( frame < mFrames.size() )
		return mFrames[ frame ].Spr[ mCurrentSubFrame ];

	return NULL;
}

SubTexture * Sprite::getSubTexture( const unsigned int& frame, const unsigned int& SubFrame ) {
	if ( frame < mFrames.size() )
		return mFrames[ frame ].Spr[ SubFrame ];

	return NULL;
}

void Sprite::x( const Float& X ) {
	mPos.x = X;
}

Float Sprite::x() const {
	return mPos.x;
}

void Sprite::y( const Float& Y ) {
	mPos.y = Y;
}

Float Sprite::y() const {
	return mPos.y;
}

void Sprite::angle( const Float& Angle) {
	mAngle = Angle;
}

Float Sprite::angle() const {
	return mAngle;
}

void Sprite::scale( const Float& Scale ) {
	this->scale( Vector2f( Scale, Scale ) );
}

void Sprite::scale( const Vector2f& Scale ) {
	mScale = Scale;
}

const Vector2f& Sprite::scale() const {
	return mScale;
}

void Sprite::animSpeed( const Float& AnimSpeed ) {
	mAnimSpeed = AnimSpeed;
}

Float Sprite::animSpeed() const {
	return mAnimSpeed;
}

bool Sprite::animPaused() const {
	return 0 != SPR_FGET( SPRITE_FLAG_ANIM_PAUSED );
}

void Sprite::animPaused( const bool& Pause )	{
	if ( Pause ) {
		if ( !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags |= SPRITE_FLAG_ANIM_PAUSED;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags &= ~SPRITE_FLAG_ANIM_PAUSED;
	}
}

void Sprite::color( const ColorA& Color) {
	mColor = Color;
}

const ColorA& Sprite::color() const {
	return mColor;
}

void Sprite::alpha( const Uint8& Alpha ) {
	mColor.Alpha = Alpha;
}

const Uint8& Sprite::alpha() const {
	return mColor.Alpha;
}

const unsigned int& Sprite::currentFrame() const {
	return mCurrentFrame;
}

const Float& Sprite::exactCurrentFrame() const {
	return mfCurrentFrame;
}

void Sprite::exactCurrentFrame( const Float& CurrentFrame ) {
	mfCurrentFrame = CurrentFrame;
}

const unsigned int& Sprite::currentSubFrame() const {
	return mCurrentSubFrame;
}

void Sprite::renderMode( const EE_RENDER_MODE& Effect ) {
	mEffect = Effect;
}

const EE_RENDER_MODE& Sprite::renderMode() const {
	return mEffect;
}

void Sprite::blendMode( const EE_BLEND_MODE& Blend ) {
	mBlend = Blend;
}

const EE_BLEND_MODE& Sprite::blendMode() const {
	return mBlend;
}

void Sprite::reverseAnim( const bool& Reverse ) {
	if ( Reverse ) {
		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags |= SPRITE_FLAG_REVERSE_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags &= ~SPRITE_FLAG_REVERSE_ANIM;
	}
}

bool Sprite::reverseAnim() const {
	return 0 != SPR_FGET( SPRITE_FLAG_REVERSE_ANIM );
}

Uint32 Sprite::getNumFrames() {
	return (Uint32)mFrames.size();
}

void Sprite::goToAndPlay( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mCurrentFrame	= GoTo;
		mfCurrentFrame	= (Float)GoTo;

		animPaused( false );
	}
}

void Sprite::goToAndStop( Uint32 GoTo ) {
	goToAndPlay( GoTo );
	animPaused( true );
}

void Sprite::animToFrameAndStop( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mAnimTo = GoTo;

		mFlags |= SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP;

		animPaused( false );
	}
}

void Sprite::setEventsCallback(const SpriteCallback& Cb , void * UserData ) {
	mCb			= Cb;
	mUserData	= UserData;
}

void Sprite::clearCallback() {
	mCb.Reset();
}

void Sprite::fireEvent( const Uint32& Event ) {
	if ( SPR_FGET( SPRITE_FLAG_EVENTS_ENABLED ) && mCb.IsSet() ) {
		mCb( Event, this, mUserData );
	}
}

void Sprite::origin( const OriginPoint& origin ) {
	mOrigin = origin;
}

const OriginPoint& Sprite:: origin() const {
	return mOrigin;
}

void Sprite::rotate( const Float& angle ) {
	mAngle += angle;
}

}}
