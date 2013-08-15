#include <eepp/graphics/cscrollparallax.hpp>

namespace EE { namespace Graphics {

cScrollParallax::cScrollParallax() :
	mSubTexture( NULL ),
	mBlend( ALPHA_NORMAL ),
	mColor( 255, 255, 255, 255 )
{
}

cScrollParallax::~cScrollParallax()
{}

cScrollParallax::cScrollParallax( cSubTexture * SubTexture, const eeVector2f& Position, const eeSizef& Size, const eeVector2f& Speed, const eeColorA& Color, const EE_BLEND_MODE& Blend ) {
	Create( SubTexture, Position, Size, Speed, Color, Blend );
}

cSubTexture * cScrollParallax::SubTexture() const {
	return mSubTexture;
}

void cScrollParallax::SubTexture( cSubTexture * subTexture ) {
	mSubTexture = subTexture;

	SetSubTexture();
}

void cScrollParallax::SetSubTexture() {
	if ( NULL != mSubTexture ) {
		mRect		= mSubTexture->SrcRect();
		mRealSize	= eeVector2f( (eeFloat)mSubTexture->RealSize().Width(), (eeFloat)mSubTexture->RealSize().Height() );

		mTiles.x	= ( (Int32)mSize.Width() / mSubTexture->RealSize().Width() ) + 1;
		mTiles.y	= ( (Int32)mSize.Height() / mSubTexture->RealSize().Height() ) + 1;
	}
}

void cScrollParallax::SetAABB() {
	mAABB		= eeRectf( mInitPos.x, mInitPos.y, mInitPos.x + mSize.Width(), mInitPos.y + mSize.Height() );
}

bool cScrollParallax::Create( cSubTexture * SubTexture, const eeVector2f& Position, const eeSizef& Size, const eeVector2f& Speed, const eeColorA& Color, const EE_BLEND_MODE& Blend ) {
	mSubTexture		= SubTexture;
	mPos		= Position;
	mSize 		= Size;
	mInitPos	= mPos;
	mSpeed		= Speed;
	mColor		= Color;
	mBlend		= Blend;

	SetAABB();
	SetSubTexture();

	return true;
}

void cScrollParallax::Size( const eeSizef& size ) {
	mSize = size;

	SetSubTexture();
	SetAABB();
}

void cScrollParallax::Position( const eeVector2f& Pos ) {
	eeVector2f Diff = mPos - mInitPos;

	mInitPos = Pos;

	mPos = Pos + Diff;

	SetAABB();
}

const eeSizef& cScrollParallax::Size() const {
	return mSize;
}

const eeVector2f& cScrollParallax::Position() const {
	return mInitPos;
}

void cScrollParallax::Draw() {
	if ( NULL != mSubTexture && mAABB.Left != mAABB.Right && mAABB.Top != mAABB.Bottom && 0 != mColor.Alpha ) {
		mPos += mSpeed * (eeFloat)mElapsed.Elapsed().AsSeconds();

		if ( mPos.x > mAABB.Left + mRealSize.Width() || mPos.x < mAABB.Left - mRealSize.Width() )
			mPos.x = mAABB.Left;

		if ( mPos.y > mAABB.Top + mRealSize.Height() || mPos.y < mAABB.Top - mRealSize.Height() )
			mPos.y = mAABB.Top;

		eeVector2f Pos 	= mPos;

		Pos.x = (eeFloat)(Int32)Pos.x;
		Pos.y = (eeFloat)(Int32)Pos.y;

		if ( mSpeed.x > 0.f )
			Pos.x -= mRealSize.Width();

		if ( mSpeed.y > 0.f )
			Pos.y -= mRealSize.Height();

		for ( Int32 y = -1; y < mTiles.y; y++ ) {
			for ( Int32 x = -1; x < mTiles.x; x++ ) {
				eeRecti Rect 	= mRect;
				eeRectf AABB( Pos.x, Pos.y, Pos.x + mRealSize.Width(), Pos.y + mRealSize.Height() );

				if ( AABB.Intersect( mAABB ) ) {
					if ( Pos.x < mAABB.Left ) {
						Rect.Left += (Int32)( mAABB.Left - Pos.x );
						AABB.Left = mAABB.Left;
					}

					if ( Pos.x + mRealSize.Width() > mAABB.Right ) {
						Rect.Right -= (Int32)( ( Pos.x + mRealSize.Width() ) - mAABB.Right );
					}

					if ( Pos.y < mAABB.Top ) {
						Rect.Top += (Int32)( mAABB.Top - Pos.y );
						AABB.Top = mAABB.Top;
					}

					if ( Pos.y + mRealSize.Height() > mAABB.Bottom ) {
						Rect.Bottom -= (Int32)( ( Pos.y + mRealSize.Height() ) - mAABB.Bottom );
					}

					mSubTexture->SrcRect( Rect );
					mSubTexture->ResetDestSize();

					if ( !( Rect.Right == 0 || Rect.Bottom == 0 ) )
						mSubTexture->Draw( AABB.Left, AABB.Top, mColor, 0.f, 1.f, mBlend );
				}

				Pos.x += mRealSize.Width();
			}

			Pos.x = (eeFloat)(Int32)mPos.x;

			if ( mSpeed.x > 0.f )
				Pos.x -= mRealSize.Width();

			Pos.y += mRealSize.Height();
		}

		mSubTexture->SrcRect( mRect );
		mSubTexture->ResetDestSize();
	}
}

void cScrollParallax::Speed( const eeVector2f& speed ) {
	mSpeed = speed;
}

const eeVector2f& cScrollParallax::Speed() const {
	return mSpeed;
}

}}
