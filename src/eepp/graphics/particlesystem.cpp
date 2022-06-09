#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/particlesystem.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::Window;

namespace EE { namespace Graphics {

ParticleSystem::ParticleSystem() :
	mParticle( NULL ),
	mPCount( 0 ),
	mTexture( 0 ),
	mPLeft( 0 ),
	mLoops( 0 ),
	mEffect( ParticleEffect::Nofx ),
	mBlend( BlendMode::Add() ),
	mColor(),
	mProgression( 0 ),
	mDirection( 0 ),
	mPos( 0.f, 0.f ),
	mPos2( 0.f, 0.f ),
	mAcc( 0.f, 0.f ),
	mSpeed( 0.f, 0.f ),
	mAlphaDecay( 0.f ),
	mSize( 0.f ),
	mHSize( 0.f ),
	mTime( 0.01f ),
	mLoop( false ),
	mUsed( false ),
	mPointsSup( false ) {}

ParticleSystem::~ParticleSystem() {
	eeSAFE_DELETE_ARRAY( mParticle );
}

void ParticleSystem::create( const ParticleEffect& Effect, const Uint32& NumParticles,
							 const Uint32& TexId, const Vector2f& Pos, const Float& PartSize,
							 const bool& AnimLoop, const Uint32& NumLoops, const ColorAf& Color,
							 const Vector2f& Pos2, const Float& AlphaDecay, const Vector2f& Speed,
							 const Vector2f& Acc ) {
	mPointsSup = GLi->pointSpriteSupported();
	mEffect = Effect;
	mPos = Pos;
	mPCount = NumParticles;
	mTexture = TextureFactory::instance()->getTexture( TexId );
	mLoop = AnimLoop;
	mLoops = NumLoops;
	mColor = Color;
	mSize = PartSize <= 0 ? 16.f : PartSize;
	mHSize = mSize * 0.5f;
	mAlphaDecay = AlphaDecay;
	mSpeed = Speed;
	mAcc = Acc;
	mDirection = 1;
	mProgression = 1;

	if ( mPos2 == Vector2f( 0, 0 ) ) {
		mPos2.x = mPos.x + 10;
		mPos2.y = mPos.y + 10;
	} else {
		mPos2 = Pos2;
	}

	mUsed = true;

	if ( !mLoop && mLoops < 1 )
		mLoops = 1;

	begin();
}

void ParticleSystem::begin() {
	mPLeft = mPCount;

	eeSAFE_DELETE_ARRAY( mParticle );

	mParticle = eeNewArray( Particle, mPCount );

	Particle* P;

	for ( Uint32 i = 0; i < mPCount; i++ ) {
		P = &mParticle[i];
		P->setUsed( true );
		P->setId( i + 1 );

		reset( P );
	}
}

void ParticleSystem::setCallbackReset( const ParticleCallback& pc ) {
	mPC = pc;
}

void ParticleSystem::reset( Particle* P ) {
	Float x, y, radio, q, z, w;

	switch ( mEffect ) {
		case ParticleEffect::Nofx: {
			P->reset( mPos.x, mPos.y, mSpeed.x, mSpeed.y, mAcc.x, mAcc.y, mSize );
			P->setColor( mColor, mAlphaDecay );
			break;
		}
		case ParticleEffect::BlueBall: {
			P->reset( mPos.x, mPos.y, -10, ( -1 * Math::randf() ), 0.01f, Math::randf(), mSize );
			P->setColor( ColorAf( 0.25f, 0.25f, 1, 1 ), 0.1f + ( 0.1f * Math::randf() ) );
			break;
		}
		case ParticleEffect::Fire: {
			x = ( mPos2.x - mPos.x + 1 ) * Math::randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::randf() + mPos.y;

			P->reset( x, y, Math::randf() - 0.5f, ( Math::randf() - 1.1f ) * 8.5f, 0.f, 0.05f,
					  mSize );
			P->setColor( ColorAf( 1.f, 0.5f, 0.1f, ( Math::randf() * 0.5f ) ),
						 Math::randf() * 0.4f + 0.01f );
			break;
		}
		case ParticleEffect::Smoke: {
			x = ( mPos2.x - mPos.x + 1 ) * Math::randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::randf() + mPos.y;

			P->reset( x, y, -( Math::randf() / 3.f + 0.1f ),
					  ( ( Math::randf() * 0.5f ) - 0.7f ) * 3, ( Math::randf() / 200.f ),
					  ( Math::randf() - 0.5f ) / 200.f );
			P->setColor( ColorAf( 0.8f, 0.8f, 0.8f, 0.3f ), ( Math::randf() * 0.005f ) + 0.005f );
			break;
		}
		case ParticleEffect::Snow: {
			x = ( mPos2.x - mPos.x + 1 ) * Math::randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::randf() + mPos.y;
			w = ( Math::randf() + 0.3f ) * 4;

			P->reset( x, y, Math::randf() - 0.5f, w, 0.f, 0.f, w * 3 );
			P->setColor( ColorAf( 1.f, 1.f, 1.f, 0.5f ), 0 );
			break;
		}
		case ParticleEffect::MagicFire: {
			P->reset( mPos.x + Math::randf(), mPos.y, -0.4f + Math::randf() * 0.8f,
					  -0.5f - Math::randf() * 0.4f, 0.f, -( Math::randf() * 0.3f ) );
			P->setColor( ColorAf( 1.f, 0.5f, 0.1f, 0.7f + 0.2f * Math::randf() ),
						 0.01f + Math::randf() * 0.05f );
			break;
		}
		case ParticleEffect::LevelUp: {
			P->reset( mPos.x, mPos.y, Math::randf() * 1.5f - 0.75f, Math::randf() * 1.5f - 0.75f,
					  Math::randf() * 4 - 2, Math::randf() * -4 + 2 );
			P->setColor( ColorAf( 1.f, 0.5f, 0.1f, 1.f ), 0.07f + Math::randf() * 0.01f );
			break;
		}
		case ParticleEffect::LevelUp2: {
			P->reset( mPos.x + Math::randf() * 32 - 16, mPos.y + Math::randf() * 64 - 32,
					  Math::randf() - 0.5f, Math::randf() - 0.5f, Math::randf() - 0.5f,
					  Math::randf() * -0.9f + 0.45f );
			P->setColor( ColorAf( 0.1f + Math::randf() * 0.1f, 0.1f + Math::randf() * 0.1f,
								  0.8f + Math::randf() * 0.3f, 1 ),
						 0.07f + Math::randf() * 0.01f );
			break;
		}
		case ParticleEffect::Heal: {
			P->reset( mPos.x, mPos.y, Math::randf() * 1.4f - 0.7f, Math::randf() * -0.4f - 1.5f,
					  Math::randf() - 0.5f, Math::randf() * -0.2f + 0.1f );
			P->setColor( ColorAf( 0.2f, 0.3f, 0.9f, 0.4f ), 0.01f + Math::randf() * 0.01f );
			break;
		}
		case ParticleEffect::WormHole: {
			int lo, la;
			Float VarB[4];

			for ( lo = 0; lo <= 3; lo++ ) {
				VarB[lo] = Math::randf() * 5;
				la = (int)( Math::randf() * 8 );

				if ( ( la * 0.5f ) != (int)( la * 0.5f ) )
					VarB[lo] = -VarB[lo];
			}

			mProgression = (int)Math::randf() * 10;
			radio = ( P->getId() * 0.125f ) * mProgression;
			x = mPos.x + ( radio * eecos( (Float)P->getId() ) );
			y = mPos.y + ( radio * eesin( (Float)P->getId() ) );

			P->reset( x, y, VarB[0], VarB[1], VarB[2], VarB[3] );
			P->setColor( ColorAf( 1.f, 0.6f, 0.3f, 1.f ), 0.02f + Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::Twirl: {
			z = 10.f + (Float)mProgression;
			w = 10.f + (Float)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection = -1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q = ( ( P->getId() * 0.01f ) + mProgression ) * 2;
			x = mPos.x - w * eesin( q );
			y = mPos.y - z * eecos( q );

			P->reset( x, y, 1, 1, 0, 0 );
			P->setColor( ColorAf( 1.f, 0.25f, 0.25f, 1 ), 0.6f + Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::Flower: {
			radio = eecos( 2 * ( (Float)P->getId() * 0.1f ) ) * 50;
			x = mPos.x + radio * eecos( (Float)P->getId() * 0.1f );
			y = mPos.y + radio * eesin( (Float)P->getId() * 0.1f );

			P->reset( x, y, 1, 1, 0, 0 );
			P->setColor( ColorAf( 1.f, 0.25f, 0.1f, 0.1f ),
						 0.3f + ( 0.2f * Math::randf() ) + Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::Galaxy: {
			radio = ( Math::randf( 1.f, 1.2f ) + eesin( 20.f / (Float)P->getId() ) ) * 60;
			x = mPos.x + radio * eecos( (Float)P->getId() );
			y = mPos.y + radio * eesin( (Float)P->getId() );

			P->reset( x, y, 0, 0, 0, 0 );
			P->setColor( ColorAf( 0.2f, 0.2f, 0.6f + 0.4f * Math::randf(), 1.f ),
						 Math::randf( 0.05f, 0.15f ) );
			break;
		}
		case ParticleEffect::Heart: {
			q = P->getId() * 0.01f;
			x = mPos.x - 50 * eesin( q * 2 ) * eesqrt( eeabs( eecos( q ) ) );
			y = mPos.y - 50 * eecos( q * 2 ) * eesqrt( eeabs( eesin( q ) ) );

			P->reset( x, y, 0.f, 0.f, 0.f, -( Math::randf() * 0.2f ) );
			P->setColor( ColorAf( 1.f, 0.5f, 0.2f, 0.6f + 0.2f * Math::randf() ),
						 0.01f + Math::randf() * 0.08f );
			break;
		}
		case ParticleEffect::BlueExplosion: {
			if ( P->getId() == 0 )
				mProgression += 10;

			radio = atan( static_cast<Float>( P->getId() % 12 ) );
			x = mPos.x + ( radio * eecos( (Float)P->getId() / mProgression ) * 30 );
			y = mPos.y + ( radio * eesin( (Float)P->getId() / mProgression ) * 30 );

			P->reset( x, y, eecos( (Float)P->getId() ), eesin( (Float)P->getId() ), 0, 0 );
			P->setColor( ColorAf( 0.3f, 0.6f, 1.f, 1.f ), 0.03f );
			break;
		}
		case ParticleEffect::GP: {
			radio = 50 + Math::randf() * 15 * eecos( (Float)P->getId() * 3.5f );
			x = mPos.x + ( radio * eecos( (Float)P->getId() * (Float)0.01428571428 ) );
			y = mPos.y + ( radio * eesin( (Float)P->getId() * (Float)0.01428571428 ) );

			P->reset( x, y, 0, 0, 0, 0 );
			P->setColor( ColorAf( 0.2f, 0.8f, 0.4f, 0.5f ), Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::BTwirl: {
			w = 10.f + (Float)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection = -1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q = ( P->getId() * 0.01f + mProgression ) * 2;
			x = mPos.x + w * eesin( q );
			y = mPos.y - w * eecos( q );

			P->reset( x, y, 1, 1, 0, 0 );
			P->setColor( ColorAf( 0.25f, 0.25f, 1.f, 1.f ),
						 0.1f + Math::randf() * 0.3f + Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::BT: {
			w = 10.f + (Float)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection = -1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q = ( P->getId() * 0.01f + mProgression ) * 2;
			x = mPos.x + w * eesin( q );
			y = mPos.y - w * eecos( q );

			P->reset( x, y, -10, -1 * Math::randf(), 0, Math::randf() );
			P->setColor( ColorAf( 0.25f, 0.25f, 1.f, 1.f ),
						 0.1f + Math::randf() * 0.1f + Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::Atomic: {
			radio = 10 + eesin( 2 * ( (Float)P->getId() * 0.1f ) ) * 50;
			x = mPos.x + radio * eecos( (Float)P->getId() * 0.033333 );
			y = mPos.y + radio * eesin( (Float)P->getId() * 0.033333 );

			P->reset( x, y, 1, 1, 0, 0 );
			P->setColor( ColorAf( 0.4f, 0.25f, 1.f, 1.f ),
						 0.3f + Math::randf() * 0.2f + Math::randf() * 0.3f );
			break;
		}
		case ParticleEffect::Callback: {
			if ( mPC.IsSet() ) {
				mPC( P, this );
			}

			break;
		}
	}
}

void ParticleSystem::draw() {
	if ( !mUsed )
		return;

	BlendMode::setMode( mBlend );

	if ( mPointsSup ) {
		if ( NULL != mTexture ) {
			const_cast<Texture*>( mTexture )->bind();
		} else {
			GLi->disable( GL_TEXTURE_2D );
			GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );
		}

		GLi->enable( GL_POINT_SPRITE );
		GLi->pointSize( mSize );

		Uint32 alloc = mPCount * sizeof( Particle );

		GLi->colorPointer( 4, GL_FP, sizeof( Particle ),
						   reinterpret_cast<char*>( &mParticle[0] ) + sizeof( Float ) * 2, alloc );
		GLi->vertexPointer( 2, GL_FP, sizeof( Particle ), reinterpret_cast<char*>( &mParticle[0] ),
							alloc );

		GLi->drawArrays( GL_POINTS, 0, (int)mPCount );

		GLi->disable( GL_POINT_SPRITE );
		GLi->enable( GL_TEXTURE_2D );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
	} else {
		Particle* P;

		BatchRenderer* BR = GlobalBatchRenderer::instance();
		BR->setTexture( mTexture );
		BR->setBlendMode( mBlend );
		BR->quadsBegin();

		for ( Uint32 i = 0; i < mPCount; i++ ) {
			P = &mParticle[i];

			if ( P->isUsed() ) {
				BR->quadsSetColor( Color(
					static_cast<Uint8>( P->r() * 255 ), static_cast<Uint8>( P->g() * 255 ),
					static_cast<Uint8>( P->b() * 255 ), static_cast<Uint8>( P->a() * 255 ) ) );
				BR->batchQuad( P->getX() - mHSize, P->getY() - mHSize, mSize, mSize );
			}
		}

		BR->drawOpt();
	}
}

void ParticleSystem::update() {
	update( Engine::instance()->getCurrentWindow()->getElapsed() );
}

void ParticleSystem::update( const System::Time& time ) {
	if ( !mUsed )
		return;

	Particle* P;

	for ( Uint32 i = 0; i < mPCount; i++ ) {
		P = &mParticle[i];

		if ( P->isUsed() || P->a() > 0.f ) {
			P->update( time.asMilliseconds() * mTime );

			// If not alive
			if ( P->a() <= 0.f ) {
				if ( !mLoop ) {			 // If not loop
					if ( mLoops == 1 ) { // If left only one loop
						P->setUsed( false );
						mPLeft--;
					} else { // more than one
						if ( i == 0 )
							if ( mLoops > 0 )
								mLoops--;

						reset( P );
					}

					if ( mPLeft == 0 ) // Last mParticle?
						mUsed = false;
				} else {
					reset( P );
				}
			}
		}
	}
}

void ParticleSystem::end() {
	mLoop = false;
	mLoops = 1;
}

void ParticleSystem::reuse() {
	mLoop = true;
	mLoops = 0;

	for ( Uint32 i = 0; i < mPCount; i++ )
		mParticle[i].setUsed( true );
}

void ParticleSystem::kill() {
	mUsed = false;
}

void ParticleSystem::setPosition( const Vector2f& Pos ) {
	mPos2.x = Pos.x + ( mPos2.x - mPos.x );
	mPos2.y = Pos.y + ( mPos2.y - mPos.y );
	mPos.x = Pos.x;
	mPos.y = Pos.y;
}

const Vector2f& ParticleSystem::getPosition() const {
	return mPos;
}

void ParticleSystem::setPosition( const Float& x, const Float& y ) {
	setPosition( Vector2f( x, y ) );
}

void ParticleSystem::setPosition2( const Vector2f& Pos ) {
	mPos2.x = Pos.x + ( Pos.x - mPos.x );
	mPos2.y = Pos.y + ( Pos.y - mPos.y );
}

const Vector2f& ParticleSystem::getPosition2() const {
	return mPos2;
}

void ParticleSystem::setPosition2( const Float& x, const Float& y ) {
	setPosition( Vector2f( x, y ) );
}

void ParticleSystem::time( const Float& time ) {
	mTime = ( time >= 0 ) ? time : mTime;
}

Float ParticleSystem::time() const {
	return mTime;
}

void ParticleSystem::setUsing( const bool& inuse ) {
	mUsed = inuse;
}

bool ParticleSystem::isUsing() const {
	return mUsed;
}

const BlendMode& ParticleSystem::getBlendMode() const {
	return mBlend;
}

void ParticleSystem::setBlendMode( const BlendMode& mode ) {
	mBlend = mode;
}

const ColorAf& ParticleSystem::getColor() const {
	return mColor;
}

void ParticleSystem::setColor( const ColorAf& Col ) {
	mColor = Col;
}

const Float& ParticleSystem::getAlphaDecay() const {
	return mAlphaDecay;
}

void ParticleSystem::setAlphaDecay( const Float& Decay ) {
	mAlphaDecay = Decay;
}

const Vector2f& ParticleSystem::getSpeed() const {
	return mSpeed;
}

void ParticleSystem::setSpeed( const Vector2f& speed ) {
	mSpeed = speed;
}

const Vector2f& ParticleSystem::getAcceleration() const {
	return mAcc;
}

void ParticleSystem::setAcceleration( const Vector2f& acc ) {
	mAcc = acc;
}

}} // namespace EE::Graphics
