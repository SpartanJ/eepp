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
	draw( position, getSize() );
}

void NinePatch::draw( const Vector2f& position, const Sizef& size ) {
	mDrawable[ UpLeft ]->draw( position );
	mDrawable[ Left ]->draw( Vector2f( position.x, position.y + mRect.Top ), Sizef( mRect.Left, size.getHeight() - mRect.Top - mRect.Bottom ) );
	mDrawable[ DownLeft ]->draw( Vector2f( position.x, position.y + size.getHeight() - mRect.Bottom ) );
	mDrawable[ Up ]->draw( Vector2f( position.x + mRect.Left, position.y ), Sizef( size.getWidth() - mRect.Left - mRect.Right, mRect.Top ) );
	mDrawable[ Center ]->draw( Vector2f( position.x + mRect.Left, position.y + mRect.Top ), Sizef( size.getWidth() - mRect.Left - mRect.Right, size.getHeight() - mRect.Top - mRect.Bottom ) );
	mDrawable[ Down ]->draw( Vector2f( position.x + mRect.Left, position.y + size.getHeight() - mRect.Bottom ), Sizef( size.getWidth() - mRect.Left - mRect.Right, mRect.Bottom ) );
	mDrawable[ UpRight ]->draw( Vector2f( position.x + size.getWidth() - mRect.Right, position.y ) );
	mDrawable[ Right ]->draw( Vector2f( position.x + size.getWidth() - mRect.Right, position.y + mRect.Top ), Vector2f( mRect.Right, size.getHeight() - mRect.Top - mRect.Bottom ) );
	mDrawable[ DownRight ]->draw( Vector2f( position.x + size.getWidth() - mRect.Right, position.y + size.getHeight() - mRect.Bottom ) );
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
}

void NinePatch::onAlphaChange() {
	for ( Int32 i = 0; i < SideCount; i++ )
		mDrawable[ i ]->setAlpha( mColor.a );
}

void NinePatch::onColorFilterChange() {
	for ( Int32 i = 0; i < SideCount; i++ )
		mDrawable[ i ]->setColor( mColor );
}

}}
