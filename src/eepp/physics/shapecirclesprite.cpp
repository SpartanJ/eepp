#include <eepp/physics/shapecirclesprite.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/sprite.hpp>

CP_NAMESPACE_BEGIN

ShapeCircleSprite * ShapeCircleSprite::New( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite ) {
	return cpNew( ShapeCircleSprite, ( body, radius, offset, Sprite, AutoDeleteSprite ) );
}

ShapeCircleSprite::ShapeCircleSprite( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite ) :
	ShapeCircle( body, radius, offset ),
	mSprite( Sprite ),
	mSpriteAutoDelete( AutoDeleteSprite )
{
	OffsetSet();
}

ShapeCircleSprite::~ShapeCircleSprite() {
	if ( mSpriteAutoDelete )
		eeSAFE_DELETE( mSprite );
}

void ShapeCircleSprite::Draw( Space * space ) {
	cVect Pos = Body()->Pos();

	mSprite->Position( Pos.x, Pos.y );
	mSprite->Angle( Body()->AngleDeg() );
	mSprite->Draw();
}

void ShapeCircleSprite::OffsetSet() {
	mSprite->Size( Sizef( ShapeCircle::Radius() * 2, ShapeCircle::Radius() * 2 ) );
	mSprite->Offset( Vector2i( -ShapeCircle::Radius() + ShapeCircle::Offset().x, -ShapeCircle::Radius() + ShapeCircle::Offset().y ) );
}

Sprite * ShapeCircleSprite::GetSprite() const {
	return mSprite;
}

void ShapeCircleSprite::Radius( const cpFloat& radius ) {
	ShapeCircle::Radius( radius );
	OffsetSet();
}

void ShapeCircleSprite::Offset( const cVect &offset ) {
	ShapeCircle::Offset( offset );
	OffsetSet();
}

CP_NAMESPACE_END

#endif
