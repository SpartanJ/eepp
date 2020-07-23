#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/scrollparallax.hpp>

namespace EE { namespace Graphics {

ScrollParallax::ScrollParallax() :
	mTextureRegion( NULL ), mBlend( BlendAlpha ), mColor( 255, 255, 255, 255 ) {}

ScrollParallax::~ScrollParallax() {}

ScrollParallax::ScrollParallax( TextureRegion* textureRegion, const Vector2f& Position,
								const Sizef& Size, const Vector2f& Speed, const Color& Color,
								const BlendMode& Blend ) {
	create( textureRegion, Position, Size, Speed, Color, Blend );
}

TextureRegion* ScrollParallax::getTextureRegion() const {
	return mTextureRegion;
}

void ScrollParallax::setTextureRegion( TextureRegion* textureRegion ) {
	mTextureRegion = textureRegion;

	setTextureRegion();
}

void ScrollParallax::setTextureRegion() {
	if ( NULL != mTextureRegion ) {
		mRect = mTextureRegion->getSrcRect();
		mRealSize = Vector2f( (Float)mTextureRegion->getPixelsSize().getWidth(),
							  (Float)mTextureRegion->getPixelsSize().getHeight() );

		mTiles.x = ( (Int32)mSize.getWidth() / (Int32)mRealSize.getWidth() ) + 1;
		mTiles.y = ( (Int32)mSize.getHeight() / (Int32)mRealSize.getHeight() ) + 1;
	}
}

void ScrollParallax::setAABB() {
	mAABB = Rectf( mInitPos.x, mInitPos.y, mInitPos.x + mSize.getWidth(),
				   mInitPos.y + mSize.getHeight() );
}

bool ScrollParallax::create( TextureRegion* textureRegion, const Vector2f& Position,
							 const Sizef& Size, const Vector2f& Speed, const Color& Color,
							 const BlendMode& Blend ) {
	mTextureRegion = textureRegion;
	mPos = Position;
	mSize = Size;
	mInitPos = mPos;
	mSpeed = Speed;
	mColor = Color;
	mBlend = Blend;

	setAABB();
	setTextureRegion();

	return true;
}

void ScrollParallax::setSize( const Sizef& size ) {
	mSize = size;

	setTextureRegion();
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
	if ( NULL != mTextureRegion && mAABB.Left != mAABB.Right && mAABB.Top != mAABB.Bottom &&
		 0 != mColor.a ) {
		mPos += mSpeed * (Float)mElapsed.getElapsed().asSeconds();

		if ( mPos.x > mAABB.Left + mRealSize.getWidth() ||
			 mPos.x < mAABB.Left - mRealSize.getWidth() )
			mPos.x = mAABB.Left;

		if ( mPos.y > mAABB.Top + mRealSize.getHeight() ||
			 mPos.y < mAABB.Top - mRealSize.getHeight() )
			mPos.y = mAABB.Top;

		Vector2f Pos = mPos;

		Pos.x = ( Float )(Int32)Pos.x;
		Pos.y = ( Float )(Int32)Pos.y;

		if ( mSpeed.x > 0.f )
			Pos.x -= mRealSize.getWidth();

		if ( mSpeed.y > 0.f )
			Pos.y -= mRealSize.getHeight();

		Float pd = mTextureRegion->getPixelDensity() / PixelDensity::getPixelDensity();
		Float ps = PixelDensity::getPixelDensity() / mTextureRegion->getPixelDensity();

		for ( Int32 y = -1; y < mTiles.y; y++ ) {
			for ( Int32 x = -1; x < mTiles.x; x++ ) {
				Rect Rect = mRect;
				Rectf AABB( Pos.x, Pos.y, Pos.x + mRealSize.getWidth(),
							Pos.y + mRealSize.getHeight() );

				if ( AABB.intersect( mAABB ) ) {
					if ( Pos.x < mAABB.Left ) {
						Rect.Left += ( Int32 )( ( mAABB.Left - Pos.x ) * pd );
						AABB.Left = mAABB.Left;
					}

					if ( Pos.x + mRealSize.getWidth() > mAABB.Right ) {
						Rect.Right -=
							( Int32 )( ( ( Pos.x + mRealSize.getWidth() ) - mAABB.Right ) * pd );
					}

					if ( Pos.y < mAABB.Top ) {
						Rect.Top += ( Int32 )( ( mAABB.Top - Pos.y ) * pd );
						AABB.Top = mAABB.Top;
					}

					if ( Pos.y + mRealSize.getHeight() > mAABB.Bottom ) {
						Rect.Bottom -=
							( Int32 )( ( ( Pos.y + mRealSize.getHeight() ) - mAABB.Bottom ) * pd );
					}

					mTextureRegion->setSrcRect( Rect );
					mTextureRegion->setDestSize(
						Vector2f( Rect.getSize().x * ps, Rect.getSize().y * ps ) );

					if ( !( Rect.Right == 0 || Rect.Bottom == 0 ) )
						mTextureRegion->draw( AABB.Left, AABB.Top, mColor, 0.f, Vector2f::One,
											  mBlend );
				}

				Pos.x += mRealSize.getWidth();
			}

			Pos.x = ( Float )(Int32)mPos.x;

			if ( mSpeed.x > 0.f )
				Pos.x -= mRealSize.getWidth();

			Pos.y += mRealSize.getHeight();
		}

		mTextureRegion->setSrcRect( mRect );
		mTextureRegion->resetDestSize();
	}
}

void ScrollParallax::setSpeed( const Vector2f& speed ) {
	mSpeed = speed;
}

const Vector2f& ScrollParallax::getSpeed() const {
	return mSpeed;
}

}} // namespace EE::Graphics
