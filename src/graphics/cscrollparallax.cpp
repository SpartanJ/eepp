#include "cscrollparallax.hpp"

namespace EE { namespace Graphics {

cScrollParallax::cScrollParallax() :
	mShape( NULL ),
	mBlend( ALPHA_NORMAL ),
	mColor( 0xFFFFFFFF )
{
}

cScrollParallax::~cScrollParallax() {}

cScrollParallax::cScrollParallax( cShape * Shape, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeVector2f& Speed, const eeRGBA& Color, const EE_PRE_BLEND_FUNC& Blend ) {
	Create( Shape, DestX, DestY, DestWidth, DestHeight, Speed, Color, Blend );
}

cShape * cScrollParallax::Shape() const {
	return mShape;
}

void cScrollParallax::Shape( cShape * shape ) {
	mShape = shape;

	SetShape();
}

void cScrollParallax::SetShape() {
	if ( NULL != mShape ) {
		mRect		= mShape->SrcRect();
		mRealSize	= eeVector2f( (eeFloat)mShape->RealSize().Width(), (eeFloat)mShape->RealSize().Height() );

		mTiles.x	= ( (Int32)mSize.Width() / mShape->RealSize().Width() ) + 1;
		mTiles.y	= ( (Int32)mSize.Height() / mShape->RealSize().Height() ) + 1;
	}
}

void cScrollParallax::SetAABB() {
	mAABB		= eeRectf( mInitPos.x, mInitPos.y, mInitPos.x + mSize.Width(), mInitPos.y + mSize.Height() );
}

bool cScrollParallax::Create( cShape * Shape, const eeFloat& DestX, const eeFloat& DestY, const eeFloat& DestWidth, const eeFloat& DestHeight, const eeVector2f& Speed, const eeRGBA& Color, const EE_PRE_BLEND_FUNC& Blend ) {
	mShape		= Shape;
	mPos		= eeVector2f( DestX, DestY );
	mSize 		= eeSizef( DestWidth, DestHeight );
	mInitPos	= mPos;
	mSpeed		= Speed;
	mColor		= Color;
	mBlend		= Blend;

	SetAABB();
	SetShape();

	return true;
}

void cScrollParallax::Size( const eeFloat& DestWidth, const eeFloat& DestHeight ) {
	mSize = eeSizef( DestWidth, DestHeight );

	SetShape();
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
	if ( NULL != mShape && mAABB.Left != mAABB.Right && mAABB.Top != mAABB.Bottom && 0 != mColor.Alpha ) {
		mPos += ( ( mSpeed * (eeFloat)mElapsed.Elapsed() ) / 1000.f );

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

					mShape->SrcRect( Rect );
					mShape->ResetDestWidthAndHeight();

					if ( !( Rect.Right == 0 || Rect.Bottom == 0 ) )
						mShape->Draw( AABB.Left, AABB.Top, mColor, 0.f, 1.f, mBlend );
				}

				Pos.x += mRealSize.Width();
			}

			Pos.x = (eeFloat)(Int32)mPos.x;

			if ( mSpeed.x > 0.f )
				Pos.x -= mRealSize.Width();

			Pos.y += mRealSize.Height();
		}

		mShape->SrcRect( mRect );
		mShape->ResetDestWidthAndHeight();
	}
}

void cScrollParallax::Speed( const eeVector2f& speed ) {
	mSpeed = speed;
}

const eeVector2f& cScrollParallax::Speed() const {
	return mSpeed;
}

}}
