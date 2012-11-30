#include <eepp/graphics/cparticlesystem.hpp>
#include <eepp/graphics/glhelper.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/window/cengine.hpp>

using namespace EE::Window;

namespace EE { namespace Graphics {

cParticleSystem::cParticleSystem() :
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

cParticleSystem::~cParticleSystem() {
	eeSAFE_DELETE_ARRAY( mParticle );
}

void cParticleSystem::Create( const EE_PARTICLE_EFFECT& Effect, const Uint32& NumParticles, const Uint32& TexId, const eeVector2f& Pos, const eeFloat& PartSize, const bool& AnimLoop, const Uint32& NumLoops, const eeColorAf& Color, const eeVector2f& Pos2, const eeFloat& AlphaDecay, const eeVector2f& Speed, const eeVector2f& Acc ) {
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

	if ( mPos2 == eeVector2f( 0, 0 ) ) {
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

void cParticleSystem::Begin() {
	mPLeft = mPCount;

	eeSAFE_DELETE_ARRAY( mParticle );

	mParticle = eeNewArray( cParticle, mPCount );

	cParticle * P;

	for ( Uint32 i=0; i < mPCount; i++ ) {
		P = &mParticle[i];
		P->Used(true);
		P->Id(i+1);

		Reset( P );
	}
}

void cParticleSystem::SetCallbackReset( const ParticleCallback& pc ) {
	mPC = pc;
}

void cParticleSystem::Reset( cParticle * P ) {
	eeFloat x, y, radio, q, z, w;

	switch ( mEffect ) {
		case PSE_Nofx:
		{
			P->Reset( mPos.x, mPos.y, mSpeed.x, mSpeed.y, mAcc.x, mAcc.y, mSize );
			P->Color( mColor , mAlphaDecay );
			break;
		}
		case PSE_BlueBall:
		{
			P->Reset( mPos.x, mPos.y, -10, ( -1 * Math::Randf() ), 0.01f, Math::Randf(), mSize );
			P->Color( eeColorAf( 0.25f ,0.25f ,1 ,1 ), 0.1f + ( 0.1f * Math::Randf() ) );
			break;
		}
		case PSE_Fire:
		{
			x = ( mPos2.x - mPos.x + 1 ) * Math::Randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::Randf() + mPos.y;

			P->Reset( mPos.x, mPos.y, Math::Randf() - 0.5f, ( Math::Randf() - 1.1f ) * 8.5f, 0.f, 0.05f, mSize );
			P->Color( eeColorAf( 1.f, 0.5f, 0.1f, ( Math::Randf() * 0.5f ) ), Math::Randf() * 0.4f + 0.01f );
			break;
		}
		case PSE_Smoke:
		{
			x = ( mPos2.x - mPos.x + 1 ) * Math::Randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::Randf() + mPos.y;

			P->Reset( x, y, -( Math::Randf() / 3.f + 0.1f ), ( ( Math::Randf() * 0.5f ) - 0.7f ) * 3, ( Math::Randf() / 200.f ), ( Math::Randf() - 0.5f ) / 200.f );
			P->Color( eeColorAf( 0.8f, 0.8f, 0.8f, 0.3f ), ( Math::Randf() * 0.005f ) + 0.005f );
			break;
		}
		case PSE_Snow:
		{
			x = ( mPos2.x - mPos.x + 1 ) * Math::Randf() + mPos.x;
			y = ( mPos2.y - mPos.y + 1 ) * Math::Randf() + mPos.y;
			w = ( Math::Randf() + 0.3f ) * 4;

			P->Reset( x, y, Math::Randf() - 0.5f, w, 0.f, 0.f, w * 3 );
			P->Color( eeColorAf( 1.f, 1.f, 1.f, 0.5f ), 0 );
			break;
		}
		case PSE_MagicFire:
		{
			P->Reset( mPos.x + Math::Randf() , mPos.y, -0.4f + Math::Randf() * 0.8f, -0.5f - Math::Randf() * 0.4f, 0.f, -( Math::Randf() * 0.3f ) );
			P->Color( eeColorAf( 1.f, 0.5f, 0.1f, 0.7f + 0.2f * Math::Randf() ), 0.01f + Math::Randf() * 0.05f );
			break;
		}
		case PSE_LevelUp:
		{
			P->Reset( mPos.x, mPos.y, Math::Randf() * 1.5f - 0.75f, Math::Randf() * 1.5f - 0.75f, Math::Randf() * 4 - 2, Math::Randf() * -4 + 2 );
			P->Color( eeColorAf( 1.f, 0.5f, 0.1f, 1.f ), 0.07f + Math::Randf() * 0.01f );
			break;
		}
		case PSE_LevelUp2:
		{
			P->Reset( mPos.x + Math::Randf() * 32 - 16, mPos.y + Math::Randf() * 64 - 32, Math::Randf() - 0.5f, Math::Randf() - 0.5f, Math::Randf() - 0.5f, Math::Randf() * -0.9f + 0.45f );
			P->Color( eeColorAf( 0.1f + Math::Randf() * 0.1f, 0.1f + Math::Randf() * 0.1f, 0.8f + Math::Randf() * 0.3f, 1 ), 0.07f + Math::Randf() * 0.01f );
			break;
		}
		case PSE_Heal:
		{
			P->Reset( mPos.x, mPos.y, Math::Randf() * 1.4f - 0.7f, Math::Randf() * -0.4f - 1.5f, Math::Randf() - 0.5f, Math::Randf() * -0.2f + 0.1f );
			P->Color( eeColorAf( 0.2f, 0.3f, 0.9f, 0.4f ), 0.01f + Math::Randf() * 0.01f );
			break;
		}
		case PSE_WormHole:
		{
			int lo, la;
			eeFloat VarB[4];

			for ( lo = 0; lo <= 3; lo++ ) {
				VarB[lo]	= Math::Randf() * 5;
				la			= (int)( Math::Randf() * 8 );

				if ( ( la * 0.5f ) != (int)( la * 0.5f ) )
					VarB[lo] = -VarB[lo];
			}

			mProgression	= (int) Math::Randf() * 10;
			radio			= ( P->Id() * 0.125f ) * mProgression;
			x				= mPos.x + ( radio * eecos( (eeFloat)P->Id() ) );
			y				= mPos.y + ( radio * eesin( (eeFloat)P->Id() ) );

			P->Reset( x, y, VarB[0], VarB[1], VarB[2], VarB[3] );
			P->Color( eeColorAf( 1.f, 0.6f, 0.3f, 1.f ), 0.02f + Math::Randf() * 0.3f );
			break;
		}
		case PSE_Twirl:
		{
			z		= 10.f + (eeFloat)mProgression;
			w		= 10.f + (eeFloat)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection =-1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q		= ( ( P->Id() * 0.01f ) + mProgression ) * 2;
			x		= mPos.x - w * eesin( q );
			y		= mPos.y - z * eecos( q );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( eeColorAf( 1.f, 0.25f, 0.25f, 1 ), 0.6f + Math::Randf() * 0.3f );
			break;
		}
		case PSE_Flower:
		{
			radio	= eecos( 2 * ( (eeFloat)P->Id() * 0.1f ) ) * 50;
			x		= mPos.x + radio * eecos( (eeFloat)P->Id() * 0.1f );
			y		= mPos.y + radio * eesin( (eeFloat)P->Id() * 0.1f );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( eeColorAf( 1.f, 0.25f, 0.1f, 0.1f ), 0.3f + ( 0.2f * Math::Randf()) + Math::Randf() * 0.3f );
			break;
		}
		case PSE_Galaxy:
		{
			radio	= ( Math::Randf( 1.f, 1.2f ) + eesin( 20.f / (eeFloat)P->Id() ) ) * 60;
			x		= mPos.x + radio * eecos( (eeFloat)P->Id() );
			y		= mPos.y + radio * eesin( (eeFloat)P->Id() );

			P->Reset( x, y, 0, 0, 0, 0 );
			P->Color( eeColorAf( 0.2f, 0.2f, 0.6f + 0.4f * Math::Randf(), 1.f ), Math::Randf( 0.05f, 0.15f ) );
			break;
		}
		case PSE_Heart:
		{
			q		= P->Id() * 0.01f;
			x		= mPos.x - 50 * eesin( q * 2 ) * eesqrt( eeabs( eecos( q ) ) );
			y		= mPos.y - 50 * eecos( q * 2 ) * eesqrt( eeabs( eesin( q ) ) );

			P->Reset( x, y, 0.f, 0.f, 0.f, -( Math::Randf() * 0.2f ) );
			P->Color( eeColorAf( 1.f, 0.5f, 0.2f, 0.6f + 0.2f * Math::Randf() ), 0.01f + Math::Randf() * 0.08f );
			break;
		}
		case PSE_BlueExplosion:
		{
			if ( P->Id() == 0 )
				mProgression += 10;

			radio	= atan( static_cast<eeFloat>( P->Id() % 12 ) );
			x		= mPos.x + ( radio * eecos( (eeFloat)P->Id() / mProgression ) * 30 );
			y		= mPos.y + ( radio * eesin( (eeFloat)P->Id() / mProgression ) * 30 );

			P->Reset(x, y, eecos( (eeFloat)P->Id() ), eesin( (eeFloat)P->Id() ), 0, 0 );
			P->Color( eeColorAf( 0.3f, 0.6f, 1.f, 1.f ), 0.03f );
			break;
		}
		case PSE_GP:
		{
			radio	= 50 + Math::Randf() * 15 * eecos( (eeFloat)P->Id() * 3.5f );
			x		= mPos.x + ( radio * eecos( (eeFloat)P->Id() * (eeFloat)0.01428571428 ) );
			y		= mPos.y + ( radio * eesin( (eeFloat)P->Id() * (eeFloat)0.01428571428 ) );

			P->Reset( x, y, 0, 0, 0, 0 );
			P->Color( eeColorAf( 0.2f, 0.8f, 0.4f, 0.5f ), Math::Randf() * 0.3f );
			break;
		}
		case PSE_BTwirl:
		{
			w		= 10.f + (eeFloat)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection =-1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q		= ( P->Id() * 0.01f + mProgression ) * 2;
			x		= mPos.x + w * eesin( q );
			y		= mPos.y - w * eecos( q );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( eeColorAf( 0.25f, 0.25f, 1.f, 1.f ), 0.1f + Math::Randf() * 0.3f + Math::Randf() * 0.3f );
			break;
		}
		case PSE_BT:
		{
			w		= 10.f + (eeFloat)mProgression;

			mProgression += mDirection;

			if ( mProgression > 50 )
				mDirection =-1;
			else if ( mProgression < -50 )
				mDirection = 1;

			q		= ( P->Id() * 0.01f + mProgression ) * 2;
			x		= mPos.x + w * eesin( q );
			y		= mPos.y - w * eecos( q );

			P->Reset( x, y, -10, -1 * Math::Randf(), 0, Math::Randf() );
			P->Color( eeColorAf( 0.25f, 0.25f, 1.f, 1.f ), 0.1f + Math::Randf() * 0.1f + Math::Randf() * 0.3f );
			break;
		}
		case PSE_Atomic:
		{
			radio	= 10 + eesin( 2 * ( (eeFloat)P->Id() * 0.1f ) ) * 50;
			x		= mPos.x + radio * eecos( (eeFloat)P->Id() * 0.033333 );
			y		= mPos.y + radio * eesin( (eeFloat)P->Id() * 0.033333 );

			P->Reset( x, y, 1, 1, 0, 0 );
			P->Color( eeColorAf( 0.4f, 0.25f, 1.f, 1.f ), 0.3f + Math::Randf() * 0.2f + Math::Randf() * 0.3f );
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

void cParticleSystem::Draw() {
	if ( !mUsed )
		return;

	cTextureFactory * TF = cTextureFactory::instance();

	TF->Bind( mTexId );
	TF->SetPreBlendFunc( mBlend );

	if ( mPointsSup ) {
		GLi->Enable( GL_POINT_SPRITE );
		GLi->PointSize( mSize );

		Uint32 alloc = mPCount * sizeof(cParticle);

		GLi->ColorPointer	( 4, GL_FP, sizeof(cParticle), reinterpret_cast<char*>( &mParticle[0] ) + sizeof(eeFloat) * 2	, alloc );
		GLi->VertexPointer	( 2, GL_FP, sizeof(cParticle), reinterpret_cast<char*>( &mParticle[0] )							, alloc );

		GLi->DrawArrays( GL_POINTS, 0, (GLsizei)mPCount );

		GLi->Disable( GL_POINT_SPRITE );
	} else {
		cTexture * Tex = TF->GetTexture( mTexId );

		if ( NULL == Tex )
			return;

		cParticle* P;

		cBatchRenderer * BR = cGlobalBatchRenderer::instance();
		BR->SetTexture( Tex );
		BR->SetPreBlendFunc( mBlend );
		BR->QuadsBegin();

		for ( Uint32 i = 0; i < mPCount; i++ ) {
			P = &mParticle[i];

			if ( P->Used() ) {
				BR->QuadsSetColor( eeColorA( static_cast<Uint8> ( P->R() * 255 ), static_cast<Uint8> ( P->G() * 255 ), static_cast<Uint8>( P->B() * 255 ), static_cast<Uint8>( P->A() * 255 ) ) );
				BR->BatchQuad( P->X() - mHSize, P->Y() - mHSize, mSize, mSize );
			}
		}

		BR->DrawOpt();
	}
}

void cParticleSystem::Update() {
	Update( cEngine::instance()->Elapsed() );
}

void cParticleSystem::Update( const eeFloat& Time ) {
	if ( !mUsed )
		return;

	cParticle * P;

	for ( Uint32 i = 0; i < mPCount; i++ ) {
		P = &mParticle[i];

		if ( P->Used() || P->A() > 0.f ) {
			P->Update( Time * mTime );

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

void cParticleSystem::End() {
	mLoop	= false;
	mLoops	= 1;
}

void cParticleSystem::ReUse() {
	mLoop	= true;
	mLoops	= 0;

	for ( Uint32 i = 0; i < mPCount; i++ )
		mParticle[i].Used( true );
}

void cParticleSystem::Kill() {
	mUsed = false;
}

void cParticleSystem::Position( const eeVector2f& Pos ) {
	mPos2.x	= Pos.x + ( mPos2.x - mPos.x );
	mPos2.y	= Pos.y + ( mPos2.y - mPos.y );
	mPos.x	= Pos.x;
	mPos.y	= Pos.y;
}

const eeVector2f& cParticleSystem::Position() const {
	return mPos;
}

void cParticleSystem::Position(const eeFloat& x, const eeFloat& y) {
	Position( eeVector2f( x, y ) );
}

void cParticleSystem::Position2( const eeVector2f& Pos ) {
	mPos2.x = Pos.x + ( Pos.x - mPos.x );
	mPos2.y = Pos.y + ( Pos.y - mPos.y );
}

const eeVector2f& cParticleSystem::Position2() const {
	return mPos2;
}

void cParticleSystem::Position2( const eeFloat& x, const eeFloat& y ) {
	Position( eeVector2f( x, y ) );
}

void cParticleSystem::Time( const eeFloat& time ) {
	mTime = ( time >= 0 ) ? time : mTime;
}

eeFloat cParticleSystem::Time() const {
	return mTime;
}

void cParticleSystem::Using(const bool& inuse) {
	mUsed = inuse;
}

bool cParticleSystem::Using() const {
	return mUsed;
}

const EE_PRE_BLEND_FUNC& cParticleSystem::BlendMode() const {
	return mBlend;
}

void cParticleSystem::BlendMode( const EE_PRE_BLEND_FUNC& mode ) {
	mBlend = mode;
}

const eeColorAf& cParticleSystem::Color() const {
	return mColor;
}

void cParticleSystem::Color( const eeColorAf& Col ) {
	mColor = Col;
}

const eeFloat& cParticleSystem::AlphaDecay() const {
	return mAlphaDecay;
}

void cParticleSystem::AlphaDecay( const eeFloat& Decay ) {
	mAlphaDecay = Decay;
}

const eeVector2f& cParticleSystem::Speed() const {
	return mSpeed;
}

void cParticleSystem::Speed( const eeVector2f& speed ) {
	mSpeed = speed;
}

const eeVector2f& cParticleSystem::Acceleration() const {
	return mAcc;
}

void cParticleSystem::Acceleration( const eeVector2f& acc ) {
	mAcc = acc;
}

}}
