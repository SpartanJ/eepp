#include <eepp/physics/cshapecirclesprite.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/csprite.hpp>

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

	mSprite->Position( Pos.x, Pos.y );
	mSprite->Angle( Body()->AngleDeg() );
	mSprite->Draw();
}

void cShapeCircleSprite::OffsetSet() {
	mSprite->Size( eeSizef( cShapeCircle::Radius() * 2, cShapeCircle::Radius() * 2 ) );
	mSprite->Offset( eeVector2i( -cShapeCircle::Radius() + cShapeCircle::Offset().x, -cShapeCircle::Radius() + cShapeCircle::Offset().y ) );
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
