#include "cparticlesystem.hpp"
#include "glhelper.hpp"
#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

cParticleSystem::cParticleSystem() :
	mParticle( NULL ),
	mPCount( 0 ),
	mTexId( 0 ),
	mPLeft( 0 ),
	mLoops( 0 ),
	mEffect( Nofx ),
	mColor(),
	mProgression( 0 ),
	mDirection( 0 ),
	mLoop( false ),
	mUsed( false ),
	mPointsSup( false ),
	mX( 0.f ),
	mY( 0.f ),
	mXAcc( 0.f ),
	mYAcc( 0.f ),
	mXSpeed( 0.f ),
	mYSpeed( 0.f ),
	mAlphaDecay( 0.f ),
	mSize( 0.f ),
	mHSize( 0.f ),
	mTime( 0.01f ),
	mX2( 0.f ),
	mY2( 0.f )

{
}

cParticleSystem::~cParticleSystem() {
	eeSAFE_DELETE_ARRAY( mParticle );
}

void cParticleSystem::Create(const EE_PARTICLE_EFFECT& Effect, const Uint32& NumParticles, const Uint32& TexId, const eeFloat& X, const eeFloat& Y, const eeFloat& PartSize, const bool& AnimLoop, const Uint32& NumLoops, const eeColorAf& Color, const eeFloat& X2, const eeFloat& Y2, const eeFloat& AlphaDecay, const eeFloat& XSpeed, const eeFloat& YSpeed, const eeFloat& XAcceleration, const eeFloat& YAcceleration) {
	mPointsSup = GLi->PointSpriteSupported();

	mEffect = Effect;
	mX = X;
	mY = Y;
	mPCount = NumParticles;
	mTexId = TexId;
	mLoop = AnimLoop;
	mLoops = NumLoops;

	if ( !mLoop && mLoops < 1 )
		mLoops = 1;

	mColor = Color;

	if (PartSize<=0)
		mSize = 16.0f;
	else
		mSize = PartSize;

	mHSize = mSize * 0.5f;

	mAlphaDecay = AlphaDecay;
	mXSpeed = XSpeed;
	mYSpeed = YSpeed;
	mXAcc = XAcceleration;
	mYAcc = YAcceleration;

	mDirection = 1;
	mProgression = 1;

	if (X2 == 0 && Y2 == 0) {
		mX2 = mX + 10;
		mY2 = mY + 10;
	} else {
		mX2 = X2;
		mY2 = Y2;
	}

	mUsed = true;

	Begin();
}

void cParticleSystem::Begin() {
	mPLeft = mPCount;

	eeSAFE_DELETE_ARRAY( mParticle );

	mParticle = eeNewArray( cParticle, mPCount );

	for ( Uint32 i=0; i < mPCount; i++ ) {
		cParticle * P = &mParticle[i];
		P->Used(true);
		P->Id(i+1);
		Reset(P);
	}
}

void cParticleSystem::SetCallbackReset( const ParticleCallback& pc ) {
	mPC = pc;
}

void cParticleSystem::Reset(cParticle* P) {
	eeFloat x, y, radio, q, z, w;

	switch(mEffect) {
		case Nofx:
			P->Reset(mX, mY, mXSpeed, mYSpeed, mXAcc, mYAcc, mSize);
			P->SetColor( mColor , mAlphaDecay );
			break;
		case BlueBall:
			P->Reset(mX, mY, -10, (-1 * eeRandf()), 0.01f, eeRandf(), mSize);
			P->SetColor( eeColorAf(0.25f ,0.25f ,1 ,1) , 0.1f + (0.1f * eeRandf()));
			break;
		case Fire:
			x = (mX2 - mX + 1) * eeRandf() + mX;
			y = (mY2 - mY + 1) * eeRandf() + mY;
			P->Reset(mX, mY, eeRandf() - 0.5f, (eeRandf() - 1.1f) * 8.5f, 0.f, 0.05f, mSize);
			P->SetColor(eeColorAf(1.f, 0.5f, 0.1f, (eeRandf() * 0.5f) ), eeRandf() * 0.4f + 0.01f );
			break;
		case Smoke:
			x = (mX2 - mX + 1) * eeRandf() + mX;
			y = (mY2 - mY + 1) * eeRandf() + mY;
			P->Reset(x, y, -(eeRandf() / 3.f + 0.1f), ((eeRandf() * 0.5f) - 0.7f) * 3, (eeRandf() / 200.f), (eeRandf() - 0.5f) / 200.f );
			P->SetColor( eeColorAf(0.8f,0.8f,0.8f,0.3f), (eeRandf() * 0.005f) + 0.005f );
			break;
		case Snow:
			x = (mX2 - mX + 1) * eeRandf() + mX;
			y = (mY2 - mY + 1) * eeRandf() + mY;
			w = (eeRandf() + 0.3f) * 4;
			P->Reset(x, y, eeRandf() - 0.5f, w, 0.f, 0.f, w * 3);
			P->SetColor( eeColorAf(1.f, 1.f, 1.f, 0.5f), 0 );
			break;
		case MagicFire:
			P->Reset( mX + eeRandf() , mY, -0.4f + eeRandf() * 0.8f, -0.5f - eeRandf() * 0.4f, 0.f, -(eeRandf() * 0.3f) );
			P->SetColor( eeColorAf(1.f, 0.5f, 0.1f, 0.7f + 0.2f * eeRandf()), 0.01f + eeRandf() * 0.05f );
			break;
		case LevelUp:
			P->Reset(mX, mY, eeRandf() * 1.5f - 0.75f, eeRandf() * 1.5f - 0.75f, eeRandf() * 4 - 2, eeRandf() * -4 + 2 );
			P->SetColor( eeColorAf(1.f, 0.5f, 0.1f, 1.f), 0.07f + eeRandf() * 0.01f );
			break;
		case LevelUp2:
			P->Reset(mX + eeRandf() * 32 - 16, mY + eeRandf() * 64 - 32, eeRandf() - 0.5f, eeRandf() - 0.5f, eeRandf() - 0.5f, eeRandf() * -0.9f + 0.45f );
			P->SetColor( eeColorAf(0.1f + eeRandf() * 0.1f, 0.1f + eeRandf() * 0.1f, 0.8f + eeRandf() * 0.3f, 1), 0.07f + eeRandf() * 0.01f );
			break;
		case Heal:
			P->Reset(mX, mY, eeRandf() * 1.4f - 0.7f, eeRandf() * -0.4f - 1.5f, eeRandf() - 0.5f, eeRandf() * -0.2f + 0.1f );
			P->SetColor( eeColorAf(0.2f, 0.3f, 0.9f, 0.4f), 0.01f + eeRandf() * 0.01f );
			break;
		case WormHole:
			int lo, la;
			eeFloat VarB[4];

			for (lo = 0; lo <= 3; lo++) {
				VarB[lo] = eeRandf() * 5;
				la = (int)(eeRandf() * 8);
				if ( (la * 0.5f) != (int)(la*0.5f) )
					VarB[lo] = -VarB[lo];
			}
			mProgression = (int) eeRandf() * 10;
			radio = (P->Id() * 0.125f) * mProgression;

			x = mX + (radio * eecos( (eeFloat)P->Id() ));
			y = mY + (radio * eesin( (eeFloat)P->Id() ));

			P->Reset(x, y, VarB[0], VarB[1], VarB[2], VarB[3]);
			P->SetColor( eeColorAf(1.f, 0.6f, 0.3f, 1.f), 0.02f + eeRandf() * 0.3f );
			break;
		case Twirl:
			z = 10.f + (eeFloat)mProgression;
			w = 10.f + (eeFloat)mProgression;
			mProgression+=mDirection;
			if (mProgression > 50) mDirection =-1;
			if (mProgression < -50) mDirection = 1;
			q = (P->Id() * 0.01f) + mProgression;
			x = mX - w * eesin((eeFloat)q * 2);
			y = mY - z * eecos((eeFloat)q * 2);

			P->Reset(x, y, 1, 1, 0, 0);
			P->SetColor( eeColorAf(1.f, 0.25f, 0.25f, 1), 0.6f + eeRandf() * 0.3f );
			break;
		case Flower:
			radio = eecos( 2 * ( (eeFloat)P->Id() * 0.1f ) ) * 50;
			x = mX + radio * eecos( (eeFloat)P->Id() * 0.1f );
			y = mY + radio * eesin( (eeFloat)P->Id() * 0.1f );

			P->Reset(x, y, 1, 1, 0 , 0);
			P->SetColor( eeColorAf(1.f, 0.25f, 0.1f, 0.1f), 0.3f + (0.2f * eeRandf()) + eeRandf() * 0.3f );
			break;
		case Galaxy:
			radio = (eeRandf(1.f, 1.2f) + eesin( 20.f / (eeFloat)P->Id() )) * 60;
			x = mX + radio * eecos( (eeFloat)P->Id() );
			y = mY + radio * eesin( (eeFloat)P->Id() );
			P->Reset(x, y, 0, 0, 0, 0);
			P->SetColor( eeColorAf(0.2f, 0.2f, 0.6f + 0.4f * eeRandf(), 1.f), eeRandf(0.05f, 0.15f) );
			break;
		case Heart:
			q = P->Id() * 0.01f;
			x = mX - 50 * eesin(q * 2) * eesqrt( eeabs( eecos(q) ) );
			y = mY - 50 * eecos(q * 2) * eesqrt( eeabs( eesin(q) ) );
			P->Reset(x, y, 0.f, 0.f, 0.f, -(eeRandf() * 0.2f));
			P->SetColor( eeColorAf(1.f, 0.5f, 0.2f, 0.6f + 0.2f * eeRandf()), 0.01f + eeRandf() * 0.08f );
			break;
		case BlueExplosion:
			if ( P->Id() == 0 ) mProgression+=10;
			radio = atan( static_cast<eeFloat>( P->Id() % 12 ) );
			x = mX + (radio * eecos( (eeFloat)P->Id() / mProgression ) * 30);
			y = mY + (radio * eesin( (eeFloat)P->Id() / mProgression ) * 30);

			P->Reset(x, y, eecos( (eeFloat)P->Id() ), eesin( (eeFloat)P->Id() ), 0, 0 );
			P->SetColor( eeColorAf(0.3f, 0.6f, 1.f, 1.f), 0.03f );
			break;
		case GP:
			radio = 50 + eeRandf() * 15 * eecos( (eeFloat)P->Id() * 3.5f );
			x = mX + ( radio * eecos( (eeFloat)P->Id() * (eeFloat)0.01428571428 ) );
			y = mY + ( radio * eesin( (eeFloat)P->Id() * (eeFloat)0.01428571428 ) );

			P->Reset(x, y, 0, 0, 0, 0);
			P->SetColor( eeColorAf(0.2f, 0.8f, 0.4f, 0.5f) , eeRandf() * 0.3f );
			break;
		case BTwirl:
			w = 10.f + (eeFloat)mProgression;
			mProgression+=mDirection;
			if (mProgression > 50) mDirection =-1;
			if (mProgression < -50) mDirection = 1;
			q = P->Id() * 0.01f + mProgression;
			x = mX + w * eesin((eeFloat)q * 2);
			y = mY - w * eecos((eeFloat)q * 2);

			P->Reset(x, y, 1, 1, 0, 0);
			P->SetColor( eeColorAf(0.25f, 0.25f, 1.f, 1.f), 0.1f + eeRandf() * 0.3f + eeRandf() * 0.3f );
			break;
		case BT:
			w = 10.f + (eeFloat)mProgression;
			mProgression+=mDirection;
			if (mProgression > 50) mDirection =-1;
			if (mProgression < -50) mDirection = 1;
			q = P->Id() * 0.01f + mProgression;
			x = mX + w * eesin((eeFloat)q * 2);
			y = mY - w * eecos((eeFloat)q * 2);

			P->Reset(x, y, -10, -1 * eeRandf(), 0, eeRandf());
			P->SetColor( eeColorAf(0.25f, 0.25f, 1.f, 1.f), 0.1f + eeRandf() * 0.1f + eeRandf() * 0.3f );
			break;
		case Atomic:
			radio = 10 + eesin( 2 * ( (eeFloat)P->Id() * 0.1f ) ) * 50;
			x = mX + radio * eecos( (eeFloat)P->Id() * (eeFloat)0.033333 );
			y = mY + radio * eesin( (eeFloat)P->Id() * (eeFloat)0.033333 );
			P->Reset(x, y, 1, 1, 0, 0);
			P->SetColor( eeColorAf(0.4f, 0.25f, 1.f, 1.f), 0.3f + eeRandf() * 0.2f + eeRandf() * 0.3f );
			break;
		case Callback:
			if ( mPC.IsSet() )
				mPC(P, this);
			break;
	}
}

void cParticleSystem::Draw() {
	cTextureFactory * TF = cTextureFactory::instance();

	TF->Bind( mTexId );
    TF->SetPreBlendFunc( ALPHA_BLENDONE );

	if ( mPointsSup ) {
		GLi->Enable( GL_POINT_SPRITE );
		GLi->PointSize( mSize );

		Uint32 alloc = mPCount * sizeof(cParticle);

		GLi->ColorPointer	( 4, GL_FLOAT, sizeof(cParticle), reinterpret_cast<char*>( &mParticle[0] ) + sizeof(eeFloat) * 2	, alloc );
		GLi->VertexPointer	( 2, GL_FLOAT, sizeof(cParticle), reinterpret_cast<char*>( &mParticle[0] )							, alloc );

		GLi->DrawArrays( GL_POINTS, 0, (GLsizei)mPCount );

		GLi->Disable( GL_POINT_SPRITE );
	} else {
		cTexture * Tex = TF->GetTexture( mTexId );

		if ( NULL == Tex )
			return;

		cParticle* P;

		cBatchRenderer * BR = cGlobalBatchRenderer::instance();
		BR->SetTexture( Tex );
		BR->SetPreBlendFunc( ALPHA_BLENDONE );
		BR->QuadsBegin();

		for ( Uint32 i = 0; i < mPCount; i++ ) {
			P = &mParticle[i];
			eeVector2f TL;

			if ( P->Used() ) {
				TL.x = P->X() - mHSize;
				TL.y = P->Y() - mHSize;

				/** FIXME: Optimize */
				BR->QuadsSetColor( eeColorA( static_cast<Uint8> ( P->R() * 255 ), static_cast<Uint8> ( P->G() * 255 ), static_cast<Uint8>( P->B() * 255 ), static_cast<Uint8>( P->A() * 255 ) ) );
				BR->BatchQuad( TL.x, TL.y, mSize, mSize );
			}
		}

		BR->DrawOpt();
	}
}

void cParticleSystem::Update( const eeFloat& Time ) {
	eeFloat Elapsed = ( Time == -99999.f ) ? cEngine::instance()->Elapsed() : Time;
	Uint32 i;

	for ( i = 0; i < mPCount; i++ ) {
		cParticle* P = &mParticle[i];

		if ( P->Used() || P->A() > 0.f ) {
			P->Update(Elapsed * mTime);

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
				} else Reset(P);
			}
		}
	}
}

void cParticleSystem::End() {
	mLoop = false;
	mLoops = 1;
}

void cParticleSystem::ReUse() {
	Uint32 i;

	mLoop = true;
	mLoops = 0;

	for ( i=0; i < mPCount; i++ )
		mParticle[i].Used( true );
}

void cParticleSystem::Kill() {
	mUsed = false;
}

void cParticleSystem::DrawUpdate() {
	if (mUsed) {
		Update();
		Draw();
	}
}

void cParticleSystem::UpdatePos(const eeFloat& x, const eeFloat& y) {
	mX2 = x + (mX2 - mX);
	mY2 = y + (mY2 - mY);
	mX = x;
	mY = y;
}

}}

