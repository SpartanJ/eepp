#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/texturefactory.hpp>

namespace EE { namespace Graphics {

NinePatch::NinePatch( const Uint32& TexId, int left, int top, int right, int bottom, const std::string& name ) :
	DrawableResource( DRAWABLE_NINEPATCH, name ),
	mRect( left, top, right, bottom ),
	mPixelDensity(1)
{
	for ( Int32 i = 0; i < SideCount; i++ )
		mDrawable[ i ] = NULL;

	Texture * tex = TextureFactory::instance()->getTexture( TexId );

	if ( NULL != tex ) {
		mSize = tex->getPixelSize();

		createFromTexture( TexId, left, top, right, bottom );
	}
}

NinePatch::NinePatch( SubTexture * subTexture, int left, int top, int right, int bottom, const std::string& name ):
	DrawableResource( DRAWABLE_NINEPATCH, name ),
	mRect( left, top, right, bottom ),
	mPixelDensity(1)
{
	for ( Int32 i = 0; i < SideCount; i++ )
		mDrawable[ i ] = NULL;

	Texture * tex;

	if ( NULL != subTexture && ( tex = subTexture->getTexture() ) != NULL ) {
		mPixelDensity = subTexture->getPixelDensity();

		Rect r( subTexture->getSrcRect() );

		mSize = r.getSize();

		createFromTexture( tex->getId(), left, top, right, bottom );

		for ( int i = 0; i  < SideCount; i++ ) {
			SubTexture * side = static_cast<SubTexture*>( mDrawable[i] );

			side->setPixelDensity( subTexture->getPixelDensity() );

			Rect sideRect = side->getSrcRect();

			sideRect.Left += r.Left;
			sideRect.Right += r.Left;
			sideRect.Top += r.Top;
			sideRect.Bottom += r.Top;

			side->setSrcRect( sideRect );
		}
	}
}

NinePatch::~NinePatch() {
	for ( Int32 i = 0; i < SideCount; i++ )
		eeSAFE_DELETE( mDrawable[ i ] );
}

Sizef NinePatch::getSize() {
	return Sizef( (Float)((Int32)( mSize.getWidth() / mPixelDensity ) ), (Float)((Int32)( mSize.getHeight() / mPixelDensity ) ) );
}

void NinePatch::draw() {
	draw( mPosition );
}

void NinePatch::draw( const Vector2f& position ) {
	draw( position, mDestSize );
}

void NinePatch::draw( const Vector2f& position, const Sizef& size ) {
	bool sizeChanged = false;

	if ( size != mDestSize ) {
		sizeChanged = true;
		mDestSize = size;
		updateSize();
	}

	if ( position != mPosition || sizeChanged ) {
		mPosition = position;
		updatePosition();
	}

	for ( Int32 i = 0; i < SideCount; i++ ) {
		Sizef destSize( mDrawable[i]->getDestSize() );

		if ( destSize.getWidth() > 0 && destSize.getHeight() > 0 )
			mDrawable[ i ]->draw();
	}
}

SubTexture * NinePatch::getSubTexture( const int& side ) {
	if ( side < SideCount )
		return mDrawable[ side ];
	return NULL;
}

void NinePatch::createFromTexture(const Uint32 & TexId, int left, int top, int right, int bottom) {
	mDrawable[ Left ] = eeNew( SubTexture, ( TexId, Rect( 0, top, left, mSize.getHeight() - bottom ) ) );
	mDrawable[ Right ] = eeNew( SubTexture, ( TexId, Rect( mSize.getWidth() - right, top, mSize.getWidth(), mSize.getHeight() - bottom ) ) );
	mDrawable[ Down ] = eeNew( SubTexture, ( TexId, Rect( left, mSize.getHeight() - bottom, mSize.getWidth() - right, mSize.getHeight() ) ) );
	mDrawable[ Up ] = eeNew( SubTexture, ( TexId, Rect( left, 0, mSize.getWidth() - right, top ) ) );
	mDrawable[ UpLeft ] = eeNew( SubTexture, ( TexId, Rect( 0, 0, left, top ) ) );
	mDrawable[ UpRight] = eeNew( SubTexture, ( TexId, Rect( mSize.getWidth() - right, 0, mSize.getWidth(), top ) ) );
	mDrawable[ DownLeft] = eeNew( SubTexture, ( TexId, Rect( 0, mSize.getHeight() - bottom, left, mSize.getHeight() ) ) );
	mDrawable[ DownRight ] = eeNew( SubTexture, ( TexId, Rect( mSize.getWidth() - right, mSize.getHeight() - bottom, mSize.getWidth(), mSize.getHeight() ) ) );
	mDrawable[ Center ] = eeNew( SubTexture, ( TexId, Rect( left, top, mSize.getWidth() - right, mSize.getHeight() - bottom ) ) );

	mRect = Rect( left, top, right, bottom );

	mRectf = Rectf( eeceil( mRect.Left / mPixelDensity * PixelDensity::getPixelDensity() ),
				eeceil( mRect.Top / mPixelDensity * PixelDensity::getPixelDensity() ),
				eeceil( mRect.Right / mPixelDensity * PixelDensity::getPixelDensity() ),
				eeceil( mRect.Bottom / mPixelDensity * PixelDensity::getPixelDensity() )
	);

	mDestSize = getSize();

	updatePosition();
	updateSize();
}

void NinePatch::onAlphaChange() {
	for ( Int32 i = 0; i < SideCount; i++ )
		mDrawable[ i ]->setAlpha( mColor.a );
}

void NinePatch::onColorFilterChange() {
	for ( Int32 i = 0; i < SideCount; i++ )
		mDrawable[ i ]->setColor( mColor );
}

void NinePatch::updateSize() {
	mDrawable[ UpLeft ]->setDestSize( Sizef( mRectf.Left, mRectf.Top ) );
	mDrawable[ Left ]->setDestSize( Sizef( mRectf.Left, mDestSize.getHeight() - mRectf.Top - mRectf.Bottom ) );
	mDrawable[ DownLeft ]->setDestSize( Sizef( mRectf.Left, mRectf.Bottom ) );
	mDrawable[ Up ]->setDestSize( Sizef( mDestSize.getWidth() - mRectf.Left - mRectf.Right, mRectf.Top ) );
	mDrawable[ Center ]->setDestSize( Sizef( mDestSize.getWidth() - mRectf.Left - mRectf.Right, mDestSize.getHeight() - mRectf.Top - mRectf.Bottom ) );
	mDrawable[ Down ]->setDestSize( Sizef( mDestSize.getWidth() - mRectf.Left - mRectf.Right, mRectf.Bottom ) );
	mDrawable[ UpRight ]->setDestSize( Sizef( mRectf.Right, mRectf.Top ) );
	mDrawable[ Right ]->setDestSize( Sizef( mRectf.Right, mDestSize.getHeight() - mRectf.Top - mRectf.Bottom ) );
	mDrawable[ DownRight ]->setDestSize( Sizef( mRectf.Right, mRectf.Bottom ) );
}

void NinePatch::updatePosition() {
	mDrawable[ UpLeft ]->setPosition( mPosition );
	mDrawable[ Left ]->setPosition( Vector2f( mPosition.x, mPosition.y + mRectf.Top ) );
	mDrawable[ DownLeft ]->setPosition( Vector2f( mPosition.x, mPosition.y + mDestSize.getHeight() - mRectf.Bottom ) );
	mDrawable[ Up ]->setPosition( Vector2f( mPosition.x + mRectf.Left, mPosition.y ) );
	mDrawable[ Center ]->setPosition( Vector2f( mPosition.x + mRectf.Left, mPosition.y + mRectf.Top ) );
	mDrawable[ Down ]->setPosition( Vector2f( mPosition.x + mRectf.Left, mPosition.y + mDestSize.getHeight() - mRectf.Bottom ) );
	mDrawable[ UpRight ]->setPosition( Vector2f( mPosition.x + mDestSize.getWidth() - mRectf.Right, mPosition.y ) );
	mDrawable[ Right ]->setPosition( Vector2f( mPosition.x + mDestSize.getWidth() - mRectf.Right, mPosition.y + mRectf.Top ) );
	mDrawable[ DownRight ]->setPosition( Vector2f( mPosition.x + mDestSize.getWidth() - mRectf.Right, mPosition.y + mDestSize.getHeight() - mRectf.Bottom ) );
}

}}
