#include <eepp/graphics/particlesystem.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::Window;

namespace EE { namespace Graphics {

ParticleSystem::ParticleSystem() :
	mParticle( NULL ),
	mPCount( 0 ),
	mTexId( 0 ),
	mPLeft( 0 ),
	mLoops( 0 ),
	mEffect( PSE_Nofx ),
	mBlend( ALPHA_BLENDONE ),
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
	mPointsSup( false )
{
}

ParticleSystem::~ParticleSystem() {
	eeSAFE_DELETE_ARRAY( mParticle );
}

void ParticleSystem::Create( const EE_PARTICLE_EFFECT& Effect, const Uint32& NumParticles, const Uint32& TexId, const Vector2f& Pos, const Float& PartSize, const bool& AnimLoop, const Uint32& NumLoops, const ColorAf& Color, const Vector2f& Pos2, const Float& AlphaDecay, const Vector2f& Speed, const Vector2f& Acc ) {
	mPointsSup		= GLi->PointSpriteSupported();
	mEffect			= Effect;
	mPos			= Pos;
	mPCount			= NumParticles;
	mTexId			= TexId;
	mLoop			= AnimLoop;
	mLoops			= NumLoops;
	mColor			= Color;
	mSize			= PartSize <=0 ? 16.f : PartSize;
	mHSize			= mSize * 0.5f;
	mAlphaDecay		= AlphaDecay;
	mSpeed			= Speed;
	mAcc			= Acc;
	mDirection		= 1;
	mProgression	= 1;

	if ( mPos2 == Vector2f( 0, 0 ) ) {
		mPos2.x	= mPos.x + 10;
		mPos2.y	= mPos.y + 10;
	} else {
		mPos2	= Pos2;
	}

	mUsed = true;

	if ( !mLoop && mLoops < 1 )
		mLoops = 1;

	Begin();
}

void ParticleSystem::Begin() {
	mPLeft = mPCount;

	eeSAFE_DELETE_ARRAY( mParticle );

	mParticle = eeNewArray( Particle, mPCount );

	Particle * P;

	for ( Uint32 i=0; i < mPCount; i++ ) {
		P = &mParticle[i];
		P->Used(true);
		P->Id(i+1);

		Reset( P );
	}
}

void ParticleSystem::SetCallbackReset( const ParticleCallback& pc ) {
	mPC = pc;
}

void ParticleSystem::Reset( Particle * P ) {
	Float x, y, radio, q, z, w;

	switch ( mEffect ) {
		case PSE_Nofx:
		{
			P->Reset( mPos.x, mPos.y, mSpeed.x, mSpeed.y, mAcc.x, mAcc.y, mSize );
			P->Color( mColor , mAlphaDecay );
			break;
		}
		case PSE_BlueBall:
		{
			P->Reset( mPos.x, mPos.y, -10, ( -1 * Math::randf() ), 0.01f, Math::randf(), mSize );
			P->Color( ColorAf( 0.25f ,0.25f ,1 ,1 ), 0.1f + ( 0.1f * Math::randf() ) );
			break;
		}
		case PSE_Fire:
		{
			x = ( mPos2.x - mPos.x + 1 ) * Math::randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::randf() + mPos.y;

			P->Reset( x, y, Math::randf() - 0.5f, ( Math::randf() - 1.1f ) * 8.5f, 0.f, 0.05f, mSize );
			P->Color( ColorAf( 1.f, 0.5f, 0.1f, ( Math::randf() * 0.5f ) ), Math::randf() * 0.4f + 0.01f );
			break;
		}
		case PSE_Smoke:
		{
			x = ( mPos2.x - mPos.x + 1 ) * Math::randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::randf() + mPos.y;

			P->Reset( x, y, -( Math::randf() / 3.f + 0.1f ), ( ( Math::randf() * 0.5f ) - 0.7f ) * 3, ( Math::randf() / 200.f ), ( Math::randf() - 0.5f ) / 200.f );
			P->Color( ColorAf( 0.8f, 0.8f, 0.8f, 0.3f ), ( Math::randf() * 0.005f ) + 0.005f );
			break;
		}
		case PSE_Snow:
		{
			x = ( mPos2.x - mPos.x + 1 ) * Math::randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::randf() + mPos.y;
			w = ( Math::randf() + 0.3f ) * 4;

			P->Reset( x, y, Math::randf() - 0.5f, w, 0.f, 0.f, w * 3 );
			P->Color( ColorAf( 1.f, 1.f, 1.f, 0.5f ), 0 );
			break;
		}
		case PSE_MagicFire:
		{
			P->Reset( mPos.x + Math::randf() , mPos.y, -0.4f + Math::randf() * 0.8f, -0.5f - Math::randf() * 0.4f, 0.f, -( Math::randf() * 0.3f ) );
			P->Color( ColorAf( 1.f, 0.5f, 0.1f, 0.7f + 0.2f * Math::randf() ), 0.01f + Math::randf() * 0.05f );
			break;
		}
		case PSE_LevelUp:
		{
			P->Reset( mPos.x, mPos.y, Math::randf() * 1.5f - 0.75f, Math::randf() * 1.5f - 0.75f, Math::randf() * 4 - 2, Math::randf() * -4 + 2 );
			P->Color( ColorAf( 1.f, 0.5f, 0.1f, 1.f ), 0.07f + Math::randf() * 0.01f );
			break;
		}
		case PSE_LevelUp2:
		{
			P->Reset( mPos.x + Math::randf() * 32 - 16, mPos.y + Math::randf() * 64 - 32, Math::randf() - 0.5f, Math::randf() - 0.5f, Math::randf() - 0.5f, Math::randf() * -0.9f + 0.45f );
			P->Color( ColorAf( 0.1f + Math::randf() * 0.1f, 0.1f + Math::randf() * 0.1f, 0.8f + Math::randf() * 0.3f, 1 ), 0.07f + Math::randf() * 0.01f );
			break;
		}
		case PSE_Heal:
		{
			P->Reset( mPos.x, mPos.y, Math::randf() * 1.4f - 0.7f, Math::randf() * -0.4f - 1.5f, Math::randf() - 0.5f, Math::randf() * -0.2f + 0.1f );
			P->Color( ColorAf( 0.2f, 0.3f, 0.9f, 0.4f ), 0.01f + Math::randf() * 0.01f );
			break;
		}
		case PSE_WormHole:
		{
			int lo, la;
			Float VarB[4];

			for ( lo = 0; lo <= 3; lo++ ) {
				VarB[lo]	= Math::randf() * 5;
				la			= (int)( Math::randf() * 8 );

				if ( ( la * 0.5f ) != (int)( la * 0.5f ) )
					VarB[lo] = -VarB[lo];
			}

			mProgression	= (int) Math::randf() * 10;
			radio			= ( P->Id() * 0.125f ) * mProgression;
			x				= mPos.x + ( radio * eecos( (Float)P->Id() ) );
			y				= mPos.y + ( radio * eesin( (Float)P->Id() ) );

			P->Reset( x, y, VarB[0], VarB[1], VarB[2], VarB[3] );
			P->Color( ColorAf( 1.f, 0.6f, 0.3f, 1.f ), 0.02f + Math::randf() * 0.3f );
			break;
		}
		case PSE_Twirl:
		{
			z		= 10.f + (Float)mProgression;
			w		= 10.f + (Float)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection =-1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q		= ( ( P->Id() * 0.01f ) + mProgression ) * 2;
			x		= mPos.x - w * eesin( q );
			y		= mPos.y - z * eecos( q );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( ColorAf( 1.f, 0.25f, 0.25f, 1 ), 0.6f + Math::randf() * 0.3f );
			break;
		}
		case PSE_Flower:
		{
			radio	= eecos( 2 * ( (Float)P->Id() * 0.1f ) ) * 50;
			x		= mPos.x + radio * eecos( (Float)P->Id() * 0.1f );
			y		= mPos.y + radio * eesin( (Float)P->Id() * 0.1f );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( ColorAf( 1.f, 0.25f, 0.1f, 0.1f ), 0.3f + ( 0.2f * Math::randf()) + Math::randf() * 0.3f );
			break;
		}
		case PSE_Galaxy:
		{
			radio	= ( Math::randf( 1.f, 1.2f ) + eesin( 20.f / (Float)P->Id() ) ) * 60;
			x		= mPos.x + radio * eecos( (Float)P->Id() );
			y		= mPos.y + radio * eesin( (Float)P->Id() );

			P->Reset( x, y, 0, 0, 0, 0 );
			P->Color( ColorAf( 0.2f, 0.2f, 0.6f + 0.4f * Math::randf(), 1.f ), Math::randf( 0.05f, 0.15f ) );
			break;
		}
		case PSE_Heart:
		{
			q		= P->Id() * 0.01f;
			x		= mPos.x - 50 * eesin( q * 2 ) * eesqrt( eeabs( eecos( q ) ) );
			y		= mPos.y - 50 * eecos( q * 2 ) * eesqrt( eeabs( eesin( q ) ) );

			P->Reset( x, y, 0.f, 0.f, 0.f, -( Math::randf() * 0.2f ) );
			P->Color( ColorAf( 1.f, 0.5f, 0.2f, 0.6f + 0.2f * Math::randf() ), 0.01f + Math::randf() * 0.08f );
			break;
		}
		case PSE_BlueExplosion:
		{
			if ( P->Id() == 0 )
				mProgression += 10;

			radio	= atan( static_cast<Float>( P->Id() % 12 ) );
			x		= mPos.x + ( radio * eecos( (Float)P->Id() / mProgression ) * 30 );
			y		= mPos.y + ( radio * eesin( (Float)P->Id() / mProgression ) * 30 );

			P->Reset(x, y, eecos( (Float)P->Id() ), eesin( (Float)P->Id() ), 0, 0 );
			P->Color( ColorAf( 0.3f, 0.6f, 1.f, 1.f ), 0.03f );
			break;
		}
		case PSE_GP:
		{
			radio	= 50 + Math::randf() * 15 * eecos( (Float)P->Id() * 3.5f );
			x		= mPos.x + ( radio * eecos( (Float)P->Id() * (Float)0.01428571428 ) );
			y		= mPos.y + ( radio * eesin( (Float)P->Id() * (Float)0.01428571428 ) );

			P->Reset( x, y, 0, 0, 0, 0 );
			P->Color( ColorAf( 0.2f, 0.8f, 0.4f, 0.5f ), Math::randf() * 0.3f );
			break;
		}
		case PSE_BTwirl:
		{
			w		= 10.f + (Float)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection =-1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q		= ( P->Id() * 0.01f + mProgression ) * 2;
			x		= mPos.x + w * eesin( q );
			y		= mPos.y - w * eecos( q );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( ColorAf( 0.25f, 0.25f, 1.f, 1.f ), 0.1f + Math::randf() * 0.3f + Math::randf() * 0.3f );
			break;
		}
		case PSE_BT:
		{
			w		= 10.f + (Float)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection =-1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q		= ( P->Id() * 0.01f + mProgression ) * 2;
			x		= mPos.x + w * eesin( q );
			y		= mPos.y - w * eecos( q );

			P->Reset( x, y, -10, -1 * Math::randf(), 0, Math::randf() );
			P->Color( ColorAf( 0.25f, 0.25f, 1.f, 1.f ), 0.1f + Math::randf() * 0.1f + Math::randf() * 0.3f );
			break;
		}
		case PSE_Atomic:
		{
			radio	= 10 + eesin( 2 * ( (Float)P->Id() * 0.1f ) ) * 50;
			x		= mPos.x + radio * eecos( (Float)P->Id() * 0.033333 );
			y		= mPos.y + radio * eesin( (Float)P->Id() * 0.033333 );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( ColorAf( 0.4f, 0.25f, 1.f, 1.f ), 0.3f + Math::randf() * 0.2f + Math::randf() * 0.3f );
			break;
		}
		case PSE_Callback:
		{
			if ( mPC.IsSet() ) {
				mPC(P, this);
			}

			break;
		}
	}
}

void ParticleSystem::Draw() {
	if ( !mUsed )
		return;

	TextureFactory * TF = TextureFactory::instance();

	TF->Bind( mTexId );
	BlendMode::SetMode( mBlend );

	if ( mPointsSup ) {
		GLi->Enable( GL_POINT_SPRITE );
		GLi->PointSize( mSize );

		Uint32 alloc = mPCount * sizeof(Particle);

		GLi->ColorPointer	( 4, GL_FP, sizeof(Particle), reinterpret_cast<char*>( &mParticle[0] ) + sizeof(Float) * 2	, alloc	);
		GLi->VertexPointer	( 2, GL_FP, sizeof(Particle), reinterpret_cast<char*>( &mParticle[0] )							, alloc	);

		GLi->DrawArrays( GL_POINTS, 0, (int)mPCount );

		GLi->Disable( GL_POINT_SPRITE );
	} else {
		Texture * Tex = TF->GetTexture( mTexId );

		if ( NULL == Tex )
			return;

		Particle* P;

		BatchRenderer * BR = GlobalBatchRenderer::instance();
		BR->SetTexture( Tex );
		BR->SetBlendMode( mBlend );
		BR->QuadsBegin();

		for ( Uint32 i = 0; i < mPCount; i++ ) {
			P = &mParticle[i];

			if ( P->Used() ) {
				BR->QuadsSetColor( ColorA( static_cast<Uint8> ( P->R() * 255 ), static_cast<Uint8> ( P->G() * 255 ), static_cast<Uint8>( P->B() * 255 ), static_cast<Uint8>( P->A() * 255 ) ) );
				BR->BatchQuad( P->X() - mHSize, P->Y() - mHSize, mSize, mSize );
			}
		}

		BR->DrawOpt();
	}
}

void ParticleSystem::Update() {
	Update( Engine::instance()->Elapsed() );
}

void ParticleSystem::Update( const System::Time& time ) {
	if ( !mUsed )
		return;

	Particle * P;

	for ( Uint32 i = 0; i < mPCount; i++ ) {
		P = &mParticle[i];

		if ( P->Used() || P->A() > 0.f ) {
			P->Update( time.asMilliseconds() * mTime );

			// If not alive
			if ( P->A() <= 0.f ) {
				if ( !mLoop ) { // If not loop
					if ( mLoops == 1 ) { // If left only one loop
						P->Used(false);
						mPLeft--;
					} else { // more than one
						if ( i == 0 )
							if ( mLoops > 0 ) mLoops--;

						Reset(P);
					}

					if ( mPLeft == 0 ) // Last mParticle?
						mUsed = false;
				} else {
					Reset( P );
				}
			}
		}
	}
}

void ParticleSystem::End() {
	mLoop	= false;
	mLoops	= 1;
}

void ParticleSystem::ReUse() {
	mLoop	= true;
	mLoops	= 0;

	for ( Uint32 i = 0; i < mPCount; i++ )
		mParticle[i].Used( true );
}

void ParticleSystem::Kill() {
	mUsed = false;
}

void ParticleSystem::Position( const Vector2f& Pos ) {
	mPos2.x	= Pos.x + ( mPos2.x - mPos.x );
	mPos2.y	= Pos.y + ( mPos2.y - mPos.y );
	mPos.x	= Pos.x;
	mPos.y	= Pos.y;
}

const Vector2f& ParticleSystem::Position() const {
	return mPos;
}

void ParticleSystem::Position(const Float& x, const Float& y) {
	Position( Vector2f( x, y ) );
}

void ParticleSystem::Position2( const Vector2f& Pos ) {
	mPos2.x = Pos.x + ( Pos.x - mPos.x );
	mPos2.y = Pos.y + ( Pos.y - mPos.y );
}

const Vector2f& ParticleSystem::Position2() const {
	return mPos2;
}

void ParticleSystem::Position2( const Float& x, const Float& y ) {
	Position( Vector2f( x, y ) );
}

void ParticleSystem::Time( const Float& time ) {
	mTime = ( time >= 0 ) ? time : mTime;
}

Float ParticleSystem::Time() const {
	return mTime;
}

void ParticleSystem::Using(const bool& inuse) {
	mUsed = inuse;
}

bool ParticleSystem::Using() const {
	return mUsed;
}

const EE_BLEND_MODE& ParticleSystem::BlendMode() const {
	return mBlend;
}

void ParticleSystem::BlendMode( const EE_BLEND_MODE& mode ) {
	mBlend = mode;
}

const ColorAf& ParticleSystem::Color() const {
	return mColor;
}

void ParticleSystem::Color( const ColorAf& Col ) {
	mColor = Col;
}

const Float& ParticleSystem::AlphaDecay() const {
	return mAlphaDecay;
}

void ParticleSystem::AlphaDecay( const Float& Decay ) {
	mAlphaDecay = Decay;
}

const Vector2f& ParticleSystem::Speed() const {
	return mSpeed;
}

void ParticleSystem::Speed( const Vector2f& speed ) {
	mSpeed = speed;
}

const Vector2f& ParticleSystem::Acceleration() const {
	return mAcc;
}

void ParticleSystem::Acceleration( const Vector2f& acc ) {
	mAcc = acc;
}

}}
