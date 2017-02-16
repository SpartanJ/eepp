#include <eepp/graphics/scrollparallax.hpp>

namespace EE { namespace Graphics {

ScrollParallax::ScrollParallax() :
	mSubTexture( NULL ),
	mBlend( ALPHA_NORMAL ),
	mColor( 255, 255, 255, 255 )
{
}

ScrollParallax::~ScrollParallax()
{}

ScrollParallax::ScrollParallax( Graphics::SubTexture * SubTexture, const Vector2f& Position, const Sizef& Size, const Vector2f& Speed, const ColorA& Color, const EE_BLEND_MODE& Blend ) {
	create( SubTexture, Position, Size, Speed, Color, Blend );
}

Graphics::SubTexture * ScrollParallax::subTexture() const {
	return mSubTexture;
}

void ScrollParallax::subTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;

	setSubTexture();
}

void ScrollParallax::setSubTexture() {
	if ( NULL != mSubTexture ) {
		mRect		= mSubTexture->srcRect();
		mRealSize	= Vector2f( (Float)mSubTexture->realSize().width(), (Float)mSubTexture->realSize().height() );

		mTiles.x	= ( (Int32)mSize.width() / mSubTexture->realSize().width() ) + 1;
		mTiles.y	= ( (Int32)mSize.height() / mSubTexture->realSize().height() ) + 1;
	}
}

void ScrollParallax::setAABB() {
	mAABB		= Rectf( mInitPos.x, mInitPos.y, mInitPos.x + mSize.width(), mInitPos.y + mSize.height() );
}

bool ScrollParallax::create( Graphics::SubTexture * SubTexture, const Vector2f& Position, const Sizef& Size, const Vector2f& Speed, const ColorA& Color, const EE_BLEND_MODE& Blend ) {
	mSubTexture		= SubTexture;
	mPos		= Position;
	mSize 		= Size;
	mInitPos	= mPos;
	mSpeed		= Speed;
	mColor		= Color;
	mBlend		= Blend;

	setAABB();
	setSubTexture();

	return true;
}

void ScrollParallax::size( const Sizef& size ) {
	mSize = size;

	setSubTexture();
	setAABB();
}

void ScrollParallax::position( const Vector2f& Pos ) {
	Vector2f Diff = mPos - mInitPos;

	mInitPos = Pos;

	mPos = Pos + Diff;

	setAABB();
}

const Sizef& ScrollParallax::size() const {
	return mSize;
}

const Vector2f& ScrollParallax::position() const {
	return mInitPos;
}

void ScrollParallax::draw() {
	if ( NULL != mSubTexture && mAABB.Left != mAABB.Right && mAABB.Top != mAABB.Bottom && 0 != mColor.Alpha ) {
		mPos += mSpeed * (Float)mElapsed.elapsed().asSeconds();

		if ( mPos.x > mAABB.Left + mRealSize.width() || mPos.x < mAABB.Left - mRealSize.width() )
			mPos.x = mAABB.Left;

		if ( mPos.y > mAABB.Top + mRealSize.height() || mPos.y < mAABB.Top - mRealSize.height() )
			mPos.y = mAABB.Top;

		Vector2f Pos 	= mPos;

		Pos.x = (Float)(Int32)Pos.x;
		Pos.y = (Float)(Int32)Pos.y;

		if ( mSpeed.x > 0.f )
			Pos.x -= mRealSize.width();

		if ( mSpeed.y > 0.f )
			Pos.y -= mRealSize.height();

		for ( Int32 y = -1; y < mTiles.y; y++ ) {
			for ( Int32 x = -1; x < mTiles.x; x++ ) {
				Recti Rect 	= mRect;
				Rectf AABB( Pos.x, Pos.y, Pos.x + mRealSize.width(), Pos.y + mRealSize.height() );

				if ( AABB.intersect( mAABB ) ) {
					if ( Pos.x < mAABB.Left ) {
						Rect.Left += (Int32)( mAABB.Left - Pos.x );
						AABB.Left = mAABB.Left;
					}

					if ( Pos.x + mRealSize.width() > mAABB.Right ) {
						Rect.Right -= (Int32)( ( Pos.x + mRealSize.width() ) - mAABB.Right );
					}

					if ( Pos.y < mAABB.Top ) {
						Rect.Top += (Int32)( mAABB.Top - Pos.y );
						AABB.Top = mAABB.Top;
					}

					if ( Pos.y + mRealSize.height() > mAABB.Bottom ) {
						Rect.Bottom -= (Int32)( ( Pos.y + mRealSize.height() ) - mAABB.Bottom );
					}

					mSubTexture->srcRect( Rect );
					mSubTexture->resetDestSize();

					if ( !( Rect.Right == 0 || Rect.Bottom == 0 ) )
						mSubTexture->draw( AABB.Left, AABB.Top, mColor, 0.f, Vector2f::One, mBlend );
				}

				Pos.x += mRealSize.width();
			}

			Pos.x = (Float)(Int32)mPos.x;

			if ( mSpeed.x > 0.f )
				Pos.x -= mRealSize.width();

			Pos.y += mRealSize.height();
		}

		mSubTexture->srcRect( mRect );
		mSubTexture->resetDestSize();
	}
}

void ScrollParallax::speed( const Vector2f& speed ) {
	mSpeed = speed;
}

const Vector2f& ScrollParallax::speed() const {
	return mSpeed;
}

}}
