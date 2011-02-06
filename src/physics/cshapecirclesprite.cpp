#include "cshapecirclesprite.hpp"

#ifdef PHYSICS_RENDERER_ENABLED

CP_NAMESPACE_BEGIN

cShapeCircleSprite * cShapeCircleSprite::New( cBody * body, cpFloat radius, cVect offset, cSprite * Sprite, bool AutoDeleteSprite ) {
	return cpNew( cShapeCircleSprite, ( body, radius, offset, Sprite, AutoDeleteSprite ) );
}

cShapeCircleSprite::cShapeCircleSprite( cBody * body, cpFloat radius, cVect offset, cSprite * Sprite, bool AutoDeleteSprite ) :
	cShapeCircle( body, radius, offset ),
	mSprite( Sprite ),
	mSpriteAutoDelete( AutoDeleteSprite )
{
	OffsetSet();
}

cShapeCircleSprite::~cShapeCircleSprite() {
	if ( mSpriteAutoDelete )
		eeSAFE_DELETE( mSprite );
}

void cShapeCircleSprite::Draw( cSpace * space ) {
	cVect Pos = Body()->Pos();

	mSprite->Update( Pos.x, Pos.y, 1.0f, Body()->AngleDeg() );
	mSprite->Draw();
}

void cShapeCircleSprite::OffsetSet() {
	mSprite->UpdateSize( cShapeCircle::Radius() * 2, cShapeCircle::Radius() * 2 );
	mSprite->OffSetX( -cShapeCircle::Radius() + cShapeCircle::Offset().x );
	mSprite->OffSetY( -cShapeCircle::Radius() + cShapeCircle::Offset().y );
}

cSprite * cShapeCircleSprite::GetSprite() const {
	return mSprite;
}

void cShapeCircleSprite::Radius( const cpFloat& radius ) {
	cShapeCircle::Radius( radius );
	OffsetSet();
}

void cShapeCircleSprite::Offset( const cVect &offset ) {
	cShapeCircle::Offset( offset );
	OffsetSet();
}

CP_NAMESPACE_END

#endif
