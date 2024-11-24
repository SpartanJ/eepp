#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::Window;

#define SPR_FGET( Flag ) ( mFlags & Flag )

namespace EE { namespace Graphics {

Sprite* Sprite::New() {
	return eeNew( Sprite, () );
}

Sprite* Sprite::New( const std::string& name, const std::string& extension,
					 TextureAtlas* SearchInTextureAtlas ) {
	return eeNew( Sprite, ( name, extension, SearchInTextureAtlas ) );
}

Sprite* Sprite::New( TextureRegion* TextureRegion ) {
	return eeNew( Sprite, ( TextureRegion ) );
}

Sprite* Sprite::New( const Uint32& TexId, const Sizef& DestSize, const Vector2i& offset,
					 const Rect& TexSector ) {
	return eeNew( Sprite, ( TexId, DestSize, offset, TexSector ) );
}

Sprite* Sprite::fromGif( IOStream& stream ) {
	auto [gif, delay] = Texture::loadGif( stream );
	Sprite* sprite = Sprite::New();

	for ( const auto& texture : gif )
		sprite->addFrame( texture );

	sprite->setAnimationSpeed( 1000.f / (float)delay );
	sprite->setAsTextureRegionOwner( true );
	sprite->setAsTextureOwner( true );
	return sprite;
}

Sprite::Sprite() : Drawable( Drawable::SPRITE ) {}

Sprite::Sprite( const std::string& name, const std::string& extension,
				TextureAtlas* SearchInTextureAtlas ) :
	Drawable( Drawable::SPRITE ) {
	addFramesByPattern( name, extension, SearchInTextureAtlas );
}

Sprite::Sprite( TextureRegion* TextureRegion ) : Drawable( Drawable::SPRITE ) {
	createStatic( TextureRegion );
}

Sprite::Sprite( const Uint32& TexId, const Sizef& DestSize, const Vector2i& Offset,
				const Rect& TexSector ) :
	Drawable( Drawable::SPRITE ) {
	createStatic( TexId, DestSize, Offset, TexSector );
}

Sprite::~Sprite() {
	cleanUpResources();
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

Sprite& Sprite::operator=( const Sprite& Other ) {
	mDrawableType = Other.mDrawableType;
	mFrames = Other.mFrames;
	mFlags = Other.mFlags;
	mColor = Other.mColor;
	mPosition = Other.mPosition;
	mRotation = Other.mRotation;
	mScale = Other.mScale;
	mAnimSpeed = Other.mAnimSpeed;
	mRepetitions = Other.mRepetitions;
	mBlend = Other.mBlend;
	mEffect = Other.mEffect;
	mCurrentFrame = Other.mCurrentFrame;
	mfCurrentFrame = Other.mfCurrentFrame;
	mCurrentSubFrame = Other.mCurrentSubFrame;
	mSubFrames = Other.mSubFrames;
	mAnimTo = Other.mAnimTo;
	mCallbacks = Other.mCallbacks;
	mNumCallBacks = Other.mNumCallBacks;

	if ( NULL != Other.mVertexColors ) {
		mVertexColors = eeNewArray( Color, 4 );
		mVertexColors[0] = Other.mVertexColors[0];
		mVertexColors[1] = Other.mVertexColors[1];
		mVertexColors[2] = Other.mVertexColors[2];
		mVertexColors[3] = Other.mVertexColors[3];
	} else {
		mVertexColors = NULL;
	}

	return *this;
}

Sprite Sprite::clone() {
	Sprite Spr;

	Spr.mDrawableType = mDrawableType;
	Spr.mColor = mColor;
	Spr.mFrames = mFrames;
	Spr.mFlags = mFlags;
	Spr.mPosition = mPosition;
	Spr.mRotation = mRotation;
	Spr.mScale = mScale;
	Spr.mAnimSpeed = mAnimSpeed;
	Spr.mRepetitions = mRepetitions;
	Spr.mBlend = mBlend;
	Spr.mEffect = mEffect;
	Spr.mCurrentFrame = mCurrentFrame;
	Spr.mfCurrentFrame = mfCurrentFrame;
	Spr.mCurrentSubFrame = mCurrentSubFrame;
	Spr.mSubFrames = mSubFrames;
	Spr.mAnimTo = mAnimTo;
	Spr.mCallbacks = mCallbacks;
	Spr.mNumCallBacks = mNumCallBacks;

	if ( NULL != mVertexColors ) {
		Spr.mVertexColors = eeNewArray( Color, 4 );
		Spr.mVertexColors[0] = mVertexColors[0];
		Spr.mVertexColors[1] = mVertexColors[1];
		Spr.mVertexColors[2] = mVertexColors[2];
		Spr.mVertexColors[3] = mVertexColors[3];
	} else {
		Spr.mVertexColors = NULL;
	}

	return Spr;
}

void Sprite::clearFrame() {
	for ( unsigned int i = 0; i < mFrames.size(); i++ )
		mFrames[i].Spr.clear();

	mFrames.clear();
}

void Sprite::cleanUpResources() {
	if ( isTextureRegionOwner() || isTextureOwner() ) {
		size_t frames = getNumFrames();

		for ( size_t i = 0; i < frames; i++ ) {
			for ( size_t f = 0; f < mFrames[i].Spr.size(); f++ ) {
				TextureRegion* region = mFrames[i].Spr[f];
				Texture* texture = region->getTexture();

				if ( isTextureOwner() && texture &&
					 TextureFactory::instance()->exists( texture ) ) {
					eeSAFE_DELETE( texture );
				}

				if ( isTextureRegionOwner() )
					GlobalTextureAtlas::instance()->remove( region );
			}
		}
	}
}

void Sprite::reset() {
	cleanUpResources();
	clearFrame();

	mFlags = SPRITE_FLAG_AUTO_ANIM | SPRITE_FLAG_EVENTS_ENABLED;

	mAnimSpeed = 16.f;
	mScale = Vector2f::One;
	mRepetitions = -1;

	mRotation = 0;
	mColor = Color::White;

	mBlend = BlendMode::Alpha();
	mEffect = RENDER_NORMAL;

	mCurrentFrame = 0;
	mCurrentSubFrame = 0;
	mfCurrentFrame = 0.f;
	mSubFrames = 1;
	mAnimTo = 0;

	disableVertexColors();

	fireEvent( SPRITE_EVENT_NEW_FRAME );
}

void Sprite::setCurrentFrame( unsigned int CurFrame ) {
	if ( CurFrame )
		CurFrame--;

	bool changed = mCurrentFrame != CurFrame;

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

	if ( changed )
		fireEvent( SPRITE_EVENT_NEW_FRAME );
}

void Sprite::setCurrentSubFrame( const unsigned int& CurSubFrame ) {
	if ( CurSubFrame < mSubFrames && mCurrentSubFrame != CurSubFrame ) {
		mCurrentSubFrame = CurSubFrame;
		fireEvent( SPRITE_EVENT_NEW_FRAME );
	}
}

Quad2f Sprite::getQuad() {
	TextureRegion* S;

	if ( mFrames.size() && ( S = getCurrentTextureRegion() ) ) {
		Rectf TmpR( mPosition.x, mPosition.y, mPosition.x + S->getDestSize().x,
					mPosition.y + S->getDestSize().y );

		Quad2f Q = Quad2f( Vector2f( TmpR.Left, TmpR.Top ), Vector2f( TmpR.Left, TmpR.Bottom ),
						   Vector2f( TmpR.Right, TmpR.Bottom ), Vector2f( TmpR.Right, TmpR.Top ) );

		Vector2f Center;

		if ( mOrigin.OriginType == OriginPoint::OriginCenter ) {
			Center = TmpR.getCenter();
		} else if ( mOrigin.OriginType == OriginPoint::OriginTopLeft ) {
			Center = mPosition;
		} else {
			Center += mPosition;
		}

		switch ( mEffect ) {
			case RENDER_NORMAL:
			case RENDER_MIRROR:
			case RENDER_FLIPPED:
			case RENDER_FLIPPED_MIRRORED:
				break;
			case RENDER_ISOMETRIC:
				Q.V[0].x += ( TmpR.Right - TmpR.Left );
				Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[3].x += ( TmpR.Right - TmpR.Left );
				Q.V[3].y += ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				break;
			case RENDER_ISOMETRIC_VERTICAL:
				Q.V[0].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				break;
			case RENDER_ISOMETRIC_VERTICAL_NEGATIVE:
				Q.V[2].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				Q.V[3].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
				break;
		}

		Q.rotate( mRotation, Center );
		Q.scale( mScale, Center );

		return Q;
	}

	return Quad2f();
}

Rectf Sprite::getAABB() {
	Rectf TmpR;
	TextureRegion* S;

	if ( mFrames.size() && ( S = getCurrentTextureRegion() ) ) {
		if ( mRotation != 0 || mEffect >= 4 ) {
			return getQuad().toAABB();
		} else { // The method used if mAngle != 0 works for mAngle = 0, but i prefer to use the
				 // faster way
			TmpR = Rectf( mPosition.x, mPosition.y, mPosition.x + S->getDestSize().x,
						  mPosition.y + S->getDestSize().y );

			Vector2f Center;

			if ( mOrigin.OriginType == OriginPoint::OriginCenter ) {
				Center = TmpR.getCenter();
			} else if ( mOrigin.OriginType == OriginPoint::OriginTopLeft ) {
				Center = mPosition;
			} else {
				Center = mPosition + mOrigin;
			}

			TmpR.scale( mScale, Center );
		}
	}

	return TmpR;
}

void Sprite::updateVertexColors( const Color& Color0, const Color& Color1, const Color& Color2,
								 const Color& Color3 ) {
	if ( NULL == mVertexColors )
		mVertexColors = eeNewArray( Color, 4 );

	mVertexColors[0] = Color0;
	mVertexColors[1] = Color1;
	mVertexColors[2] = Color2;
	mVertexColors[3] = Color3;
}

void Sprite::disableVertexColors() {
	eeSAFE_DELETE_ARRAY( mVertexColors );
}

unsigned int Sprite::framePos() {
	mFrames.push_back( Frame() );
	return (unsigned int)mFrames.size() - 1;
}

bool Sprite::createStatic( TextureRegion* TextureRegion ) {
	reset();

	addFrame( TextureRegion );

	return true;
}

bool Sprite::createStatic( const Uint32& TexId, const Sizef& DestSize, const Vector2i& offset,
						   const Rect& TexSector ) {
	if ( TextureFactory::instance()->existsId( TexId ) ) {
		reset();

		addFrame( TexId, DestSize, offset, TexSector );

		return true;
	}

	return false;
}

bool Sprite::createStatic( Texture* tex, const Sizef& DestSize, const Vector2i& offset,
						   const Rect& TexSector ) {
	if ( tex ) {
		reset();

		addFrame( tex->getTextureId(), DestSize, offset, TexSector );

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

bool Sprite::addFrames( const std::vector<TextureRegion*> TextureRegions ) {
	if ( TextureRegions.size() ) {
		for ( unsigned int i = 0; i < TextureRegions.size(); i++ ) {
			if ( NULL != TextureRegions[i] ) {
				addFrame( TextureRegions[i] );
			}
		}

		return true;
	}

	return false;
}

bool Sprite::addFramesByPatternId( const Uint32& TextureRegionId, const std::string& extension,
								   TextureAtlas* SearchInTextureAtlas ) {
	std::vector<TextureRegion*> TextureRegions =
		TextureAtlasManager::instance()->getTextureRegionsByPatternId( TextureRegionId, extension,
																	   SearchInTextureAtlas );

	if ( TextureRegions.size() ) {
		addFrames( TextureRegions );

		return true;
	}

	Log::warning( "Sprite::AddFramesByPatternId: Couldn't find any pattern with Id: %d",
				  TextureRegionId );

	return false;
}

bool Sprite::addFramesByPattern( const std::string& name, const std::string& extension,
								 TextureAtlas* SearchInTextureAtlas ) {
	std::vector<TextureRegion*> TextureRegions =
		TextureAtlasManager::instance()->getTextureRegionsByPattern( name, extension,
																	 SearchInTextureAtlas );

	if ( TextureRegions.size() ) {
		addFrames( TextureRegions );

		return true;
	}

	Log::warning( "Sprite::AddFramesByPattern: Couldn't find any pattern with: %s", name.c_str() );

	return false;
}

bool Sprite::addSubFrame( TextureRegion* TextureRegion, const unsigned int& NumFrame,
						  const unsigned int& NumSubFrame ) {
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

		mFrames[NF].Spr[NSF] = TextureRegion;

		return true;
	}

	return false;
}

unsigned int Sprite::addFrame( TextureRegion* TextureRegion ) {
	unsigned int id = framePos();

	addSubFrame( TextureRegion, id, mCurrentSubFrame );

	return id;
}

unsigned int Sprite::addFrame( const Uint32& TexId, const Sizef& DestSize, const Vector2i& Offset,
							   const Rect& TexSector ) {
	unsigned int id = framePos();

	if ( addSubFrame( TexId, id, mCurrentSubFrame, DestSize, Offset, TexSector ) )
		return id;

	return 0;
}

unsigned int Sprite::addFrame( Texture* tex, const Sizef& DestSize, const Vector2i& offset,
							   const Rect& TexSector ) {
	unsigned int id = framePos();

	if ( addSubFrame( tex, id, mCurrentSubFrame, DestSize, offset, TexSector ) )
		return id;

	return 0;
}

bool Sprite::addSubFrame( Texture* tex, const unsigned int& NumFrame,
						  const unsigned int& NumSubFrame, const Sizef& DestSize,
						  const Vector2i& Offset, const Rect& TexSector ) {
	if ( tex )
		return addSubFrame( tex->getTextureId(), NumFrame, NumSubFrame, DestSize, Offset,
							TexSector );
	return false;
}

bool Sprite::addSubFrame( const Uint32& TexId, const unsigned int& NumFrame,
						  const unsigned int& NumSubFrame, const Sizef& DestSize,
						  const Vector2i& Offset, const Rect& TexSector ) {
	if ( !TextureFactory::instance()->existsId( TexId ) )
		return false;

	Texture* Tex = TextureFactory::instance()->getTexture( TexId );
	TextureRegion* S = GlobalTextureAtlas::instance()->add( TextureRegion::New() );

	S->setTextureId( TexId );

	if ( TexSector.Right > 0 && TexSector.Bottom > 0 )
		S->setSrcRect( TexSector );
	else
		S->setSrcRect( Rect( 0, 0, (Int32)Tex->getImageWidth(), (Int32)Tex->getImageHeight() ) );

	Sizef destSize( DestSize );

	if ( destSize.x <= 0 ) {
		destSize.x = static_cast<Float>( S->getSrcRect().Right - S->getSrcRect().Left );
	}

	if ( destSize.y <= 0 ) {
		destSize.y = static_cast<Float>( S->getSrcRect().Bottom - S->getSrcRect().Top );
	}

	S->setDestSize( destSize );
	S->setOffset( Offset );

	addSubFrame( S, NumFrame, NumSubFrame );

	return true;
}

void Sprite::update() {
	update( Engine::instance()->getCurrentWindow()->getElapsed() );
}

void Sprite::update( const Time& ElapsedTime ) {
	if ( !( mFrames.size() > 1 && !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) &&
			Time::Zero != ElapsedTime ) )
		return;

	unsigned int Size = (unsigned int)mFrames.size() - 1;

	if ( mRepetitions == 0 )
		return;

	if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
		mfCurrentFrame += mAnimSpeed * ElapsedTime.asSeconds();
	else
		mfCurrentFrame -= mAnimSpeed * ElapsedTime.asSeconds();

	auto lastFrame = mCurrentFrame;
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
		if ( mRepetitions < 0 ) {
			mfCurrentFrame = 0.0f;
			mCurrentFrame = 0;
			fireEvent( SPRITE_EVENT_FIRST_FRAME );
		} else {
			if ( mRepetitions == 0 ) {
				mfCurrentFrame = (Float)Size;
				mCurrentFrame = Size;
				fireEvent( SPRITE_EVENT_LAST_FRAME );
			} else {
				mfCurrentFrame = 0.0f;
				mCurrentFrame = 0;
				mRepetitions--;
				fireEvent( SPRITE_EVENT_FIRST_FRAME );
			}
		}
	} else if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) && mfCurrentFrame < 0.0f ) {
		if ( mRepetitions < 0 ) {
			mfCurrentFrame = Size + 1.0f;
			mCurrentFrame = Size;
			fireEvent( SPRITE_EVENT_LAST_FRAME );
		} else {
			if ( mRepetitions == 0 ) {
				mfCurrentFrame = 0.0f;
				mCurrentFrame = 0;
				fireEvent( SPRITE_EVENT_FIRST_FRAME );
			} else {
				mfCurrentFrame = Size + 1.0f;
				mCurrentFrame = Size;
				mRepetitions--;
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

	if ( lastFrame != mCurrentFrame )
		fireEvent( SPRITE_EVENT_NEW_FRAME );
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

	if ( mCurrentFrame != Size ) {
		mfCurrentFrame = (Float)Size;
		mCurrentFrame = Size;

		fireEvent( SPRITE_EVENT_NEW_FRAME );
	}
}

void Sprite::draw( const BlendMode& Blend, const RenderMode& Effect ) {
	if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
		update();

	TextureRegion* S = getCurrentTextureRegion();

	if ( S == NULL )
		return;

	if ( NULL == mVertexColors )
		S->draw( mPosition.x, mPosition.y, mColor, mRotation, mScale, Blend, Effect, mOrigin );
	else
		S->draw( mPosition.x, mPosition.y, mRotation, mScale, mVertexColors[0], mVertexColors[1],
				 mVertexColors[2], mVertexColors[3], Blend, Effect, mOrigin );
}

void Sprite::draw() {
	draw( mBlend, mEffect );
}

void Sprite::draw( const Vector2f& position ) {
	draw( position, Sizef::Zero );
}

void Sprite::draw( const Vector2f& position, const Sizef& size ) {
	if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
		update();

	TextureRegion* S = getCurrentTextureRegion();

	if ( S == NULL )
		return;

	Sizef oldSize = S->getDestSize();

	if ( size != Sizef::Zero ) {
		S->setDestSize( size );
	}

	if ( NULL == mVertexColors )
		S->draw( position.x, position.y, getColor(), mRotation, mScale, mBlend, mEffect, mOrigin );
	else
		S->draw( position.x, position.y, mRotation, mScale, getColor(), getColor(), getColor(),
				 getColor(), mBlend, mEffect, mOrigin );

	if ( size != Sizef::Zero ) {
		S->setDestSize( oldSize );
	}
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

Vector2i Sprite::getOffset() {
	TextureRegion* S = getCurrentTextureRegion();

	if ( S != NULL )
		return S->getOffset();

	return Vector2i();
}

void Sprite::setOffset( const Vector2i& offset ) {
	TextureRegion* S = getCurrentTextureRegion();

	if ( S != NULL ) {
		S->setOffset( offset );
	}
}

void Sprite::setSize( const Sizef& Size, const unsigned int& FrameNum,
					  const unsigned int& SubFrame ) {
	mFrames[getFrame( FrameNum )].Spr[getSubFrame( SubFrame )]->setDestSize( Size );
}

void Sprite::setSize( const Sizef& Size ) {
	mFrames[mCurrentFrame].Spr[mCurrentSubFrame]->setDestSize( Size );
}

Sizef Sprite::setSize( const unsigned int& FrameNum, const unsigned int& SubFrame ) {
	return mFrames[getFrame( FrameNum )].Spr[getSubFrame( SubFrame )]->getDestSize();
}

Sizef Sprite::getSize() {
	return mFrames[mCurrentFrame].Spr[mCurrentSubFrame]->getSize();
}

Sizef Sprite::getPixelsSize() {
	return mFrames[mCurrentFrame].Spr[mCurrentSubFrame]->getPixelsSize();
}

void Sprite::setRepetitions( const int& Repeations ) {
	mRepetitions = Repeations;
}

void Sprite::setAutoAnimate( const bool& Autoanim ) {
	if ( Autoanim ) {
		if ( !SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags |= SPRITE_FLAG_AUTO_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_AUTO_ANIM ) )
			mFlags &= ~SPRITE_FLAG_AUTO_ANIM;
	}
}

bool Sprite::getAutoAnimate() const {
	return 0 != SPR_FGET( SPRITE_FLAG_AUTO_ANIM );
}

TextureRegion* Sprite::getCurrentTextureRegion() {
	if ( mFrames.size() )
		return mFrames[mCurrentFrame].Spr[mCurrentSubFrame];

	return NULL;
}

TextureRegion* Sprite::getTextureRegion( const unsigned int& frame ) {
	if ( frame < mFrames.size() )
		return mFrames[frame].Spr[mCurrentSubFrame];

	return NULL;
}

TextureRegion* Sprite::getTextureRegion( const unsigned int& frame, const unsigned int& SubFrame ) {
	if ( frame < mFrames.size() )
		return mFrames[frame].Spr[SubFrame];

	return NULL;
}

void Sprite::setRotation( const Float& rotation ) {
	mRotation = rotation;
}

Float Sprite::getRotation() const {
	return mRotation;
}

void Sprite::setScale( const Float& Scale ) {
	this->setScale( Vector2f( Scale, Scale ) );
}

void Sprite::setScale( const Vector2f& Scale ) {
	mScale = Scale;
}

const Vector2f& Sprite::getScale() const {
	return mScale;
}

void Sprite::setAnimationSpeed( const Float& AnimSpeed ) {
	mAnimSpeed = AnimSpeed;
}

Float Sprite::getAnimationSpeed() const {
	return mAnimSpeed;
}

bool Sprite::isAnimationPaused() const {
	return 0 != SPR_FGET( SPRITE_FLAG_ANIM_PAUSED );
}

void Sprite::setAnimationPaused( const bool& Pause ) {
	if ( Pause ) {
		if ( !SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags |= SPRITE_FLAG_ANIM_PAUSED;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_ANIM_PAUSED ) )
			mFlags &= ~SPRITE_FLAG_ANIM_PAUSED;
	}
}

const unsigned int& Sprite::getCurrentFrame() const {
	return mCurrentFrame;
}

const Float& Sprite::getExactCurrentFrame() const {
	return mfCurrentFrame;
}

void Sprite::setExactCurrentFrame( const Float& CurrentFrame ) {
	mfCurrentFrame = CurrentFrame;
}

const unsigned int& Sprite::getCurrentSubFrame() const {
	return mCurrentSubFrame;
}

void Sprite::setRenderMode( const RenderMode& Effect ) {
	mEffect = Effect;
}

const RenderMode& Sprite::getRenderMode() const {
	return mEffect;
}

void Sprite::setBlendMode( const BlendMode& Blend ) {
	mBlend = Blend;
}

const BlendMode& Sprite::getBlendMode() const {
	return mBlend;
}

void Sprite::setReverseAnimation( const bool& Reverse ) {
	if ( Reverse ) {
		if ( !SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags |= SPRITE_FLAG_REVERSE_ANIM;
	} else {
		if ( SPR_FGET( SPRITE_FLAG_REVERSE_ANIM ) )
			mFlags &= ~SPRITE_FLAG_REVERSE_ANIM;
	}
}

bool Sprite::getReverseAnimation() const {
	return 0 != SPR_FGET( SPRITE_FLAG_REVERSE_ANIM );
}

Uint32 Sprite::getNumFrames() {
	return (Uint32)mFrames.size();
}

void Sprite::goToAndPlay( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		if ( mCurrentFrame != GoTo ) {
			mCurrentFrame = GoTo;
			mfCurrentFrame = (Float)GoTo;
			fireEvent( SPRITE_EVENT_NEW_FRAME );
		}

		setAnimationPaused( false );
	}
}

void Sprite::goToAndStop( Uint32 GoTo ) {
	goToAndPlay( GoTo );
	setAnimationPaused( true );
}

void Sprite::animToFrameAndStop( Uint32 GoTo ) {
	if ( GoTo )
		GoTo--;

	if ( GoTo < mFrames.size() ) {
		mAnimTo = GoTo;

		mFlags |= SPRITE_FLAG_ANIM_TO_FRAME_AND_STOP;

		setAnimationPaused( false );
	}
}

Uint32 Sprite::pushEventsCallback( const SpriteCallback& cb, void* UserData ) {
	mCallbacks[++mNumCallBacks] = SpriteCbData{ cb, UserData };
	return mNumCallBacks;
}

bool Sprite::popEventsCallback( const Uint32& callbackId ) {
	return mCallbacks.erase( callbackId ) > 0;
}

void Sprite::setEventsCallback( const SpriteCallback& cb, void* UserData ) {
	pushEventsCallback( cb, UserData );
}

void Sprite::fireEvent( const Uint32& Event ) {
	if ( SPR_FGET( SPRITE_FLAG_EVENTS_ENABLED ) ) {
		for ( const auto& cb : mCallbacks ) {
			cb.second.cb( Event, this, cb.second.userData );
		}
	}
}

Sprite& Sprite::setAsTextureRegionOwner( bool set ) {
	if ( set )
		mFlags |= SPRITE_FLAG_TEXTURE_REGION_OWNER;
	else
		mFlags &= ~SPRITE_FLAG_TEXTURE_REGION_OWNER;
	return *this;
}

bool Sprite::isTextureRegionOwner() const {
	return mFlags & SPRITE_FLAG_TEXTURE_REGION_OWNER;
}

Sprite& Sprite::setAsTextureOwner( bool set ) {
	if ( set )
		mFlags |= SPRITE_FLAG_TEXTURE_OWNER;
	else
		mFlags &= ~SPRITE_FLAG_TEXTURE_OWNER;
	return *this;
}

bool Sprite::isTextureOwner() const {
	return mFlags & SPRITE_FLAG_TEXTURE_OWNER;
}

void Sprite::setOrigin( const OriginPoint& origin ) {
	mOrigin = origin;
}

const OriginPoint& Sprite::getOrigin() const {
	return mOrigin;
}

void Sprite::rotate( const Float& angle ) {
	mRotation += angle;
}

}} // namespace EE::Graphics
