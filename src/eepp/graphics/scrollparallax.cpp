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

Graphics::SubTexture * ScrollParallax::getSubTexture() const {
	return mSubTexture;
}

void ScrollParallax::setSubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;

	setSubTexture();
}

void ScrollParallax::setSubTexture() {
	if ( NULL != mSubTexture ) {
		mRect		= mSubTexture->getSrcRect();
		mRealSize	= Vector2f( (Float)mSubTexture->getPxSize().getWidth(), (Float)mSubTexture->getPxSize().getHeight() );

		mTiles.x	= ( (Int32)mSize.getWidth() / (Int32)mRealSize.getWidth() ) + 1;
		mTiles.y	= ( (Int32)mSize.getHeight() / (Int32)mRealSize.getHeight() ) + 1;
	}
}

void ScrollParallax::setAABB() {
	mAABB		= Rectf( mInitPos.x, mInitPos.y, mInitPos.x + mSize.getWidth(), mInitPos.y + mSize.getHeight() );
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

void ScrollParallax::setSize( const Sizef& size ) {
	mSize = size;

	setSubTexture();
	setAABB();
}

void ScrollParallax::setPosition( const Vector2f& Pos ) {
	Vector2f Diff = mPos - mInitPos;

	mInitPos = Pos;

	mPos = Pos + Diff;

	setAABB();
}

const Sizef& ScrollParallax::getSize() const {
	return mSize;
}

const Vector2f& ScrollParallax::getPosition() const {
	return mInitPos;
}

void ScrollParallax::draw() {
	if ( NULL != mSubTexture && mAABB.Left != mAABB.Right && mAABB.Top != mAABB.Bottom && 0 != mColor.Alpha ) {
		mPos += mSpeed * (Float)mElapsed.getElapsed().asSeconds();

		if ( mPos.x > mAABB.Left + mRealSize.getWidth() || mPos.x < mAABB.Left - mRealSize.getWidth() )
			mPos.x = mAABB.Left;

		if ( mPos.y > mAABB.Top + mRealSize.getHeight() || mPos.y < mAABB.Top - mRealSize.getHeight() )
			mPos.y = mAABB.Top;

		Vector2f Pos 	= mPos;

		Pos.x = (Float)(Int32)Pos.x;
		Pos.y = (Float)(Int32)Pos.y;

		if ( mSpeed.x > 0.f )
			Pos.x -= mRealSize.getWidth();

		if ( mSpeed.y > 0.f )
			Pos.y -= mRealSize.getHeight();

		Float pd = mSubTexture->getPixelDensity() / PixelDensity::getPixelDensity();
		Float ps = PixelDensity::getPixelDensity() / mSubTexture->getPixelDensity();

		for ( Int32 y = -1; y < mTiles.y; y++ ) {
			for ( Int32 x = -1; x < mTiles.x; x++ ) {
				Recti Rect 	= mRect;
				Rectf AABB( Pos.x, Pos.y, Pos.x + mRealSize.getWidth(), Pos.y + mRealSize.getHeight() );

				if ( AABB.intersect( mAABB ) ) {
					if ( Pos.x < mAABB.Left ) {
						Rect.Left += (Int32)( ( mAABB.Left - Pos.x ) * pd );
						AABB.Left = mAABB.Left;
					}

					if ( Pos.x + mRealSize.getWidth() > mAABB.Right ) {
						Rect.Right -= (Int32)( ( ( Pos.x + mRealSize.getWidth() ) - mAABB.Right ) * pd );
					}

					if ( Pos.y < mAABB.Top ) {
						Rect.Top += (Int32)( ( mAABB.Top - Pos.y ) * pd );
						AABB.Top = mAABB.Top;
					}

					if ( Pos.y + mRealSize.getHeight() > mAABB.Bottom ) {
						Rect.Bottom -= (Int32)( ( ( Pos.y + mRealSize.getHeight() ) - mAABB.Bottom ) * pd );
					}

					mSubTexture->setSrcRect( Rect );
					mSubTexture->setDestSize( Vector2f( Rect.getSize().x * ps, Rect.getSize().y * ps ) );

					if ( !( Rect.Right == 0 || Rect.Bottom == 0 ) )
						mSubTexture->draw( AABB.Left, AABB.Top, mColor, 0.f, Vector2f::One, mBlend );
				}

				Pos.x += mRealSize.getWidth();
			}

			Pos.x = (Float)(Int32)mPos.x;

			if ( mSpeed.x > 0.f )
				Pos.x -= mRealSize.getWidth();

			Pos.y += mRealSize.getHeight();
		}

		mSubTexture->setSrcRect( mRect );
		mSubTexture->resetDestSize();
	}
}

void ScrollParallax::setSpeed( const Vector2f& speed ) {
	mSpeed = speed;
}

const Vector2f& ScrollParallax::getSpeed() const {
	return mSpeed;
}

}}
