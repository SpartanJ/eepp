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
	offsetSet();
}

ShapeCircleSprite::~ShapeCircleSprite() {
	if ( mSpriteAutoDelete )
		eeSAFE_DELETE( mSprite );
}

void ShapeCircleSprite::draw( Space * space ) {
	cVect Pos = body()->pos();

	mSprite->setPosition( Pos.x, Pos.y );
	mSprite->setRotation( body()->angleDeg() );
	mSprite->draw();
}

void ShapeCircleSprite::offsetSet() {
	mSprite->setSize( Sizef( ShapeCircle::radius() * 2, ShapeCircle::radius() * 2 ) );
	mSprite->setOffset( Vector2i( -ShapeCircle::radius() + ShapeCircle::offset().x, -ShapeCircle::radius() + ShapeCircle::offset().y ) );
}

Sprite * ShapeCircleSprite::getSprite() const {
	return mSprite;
}

void ShapeCircleSprite::radius( const cpFloat& radius ) {
	ShapeCircle::radius( radius );
	offsetSet();
}

void ShapeCircleSprite::offset( const cVect &offset ) {
	ShapeCircle::offset( offset );
	offsetSet();
}

CP_NAMESPACE_END

#endif
