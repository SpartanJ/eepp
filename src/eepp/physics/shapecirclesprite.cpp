#include <eepp/physics/shapecirclesprite.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/sprite.hpp>

namespace EE { namespace Physics {

ShapeCircleSprite * ShapeCircleSprite::New( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite ) {
	return eeNew( ShapeCircleSprite, ( body, radius, offset, Sprite, AutoDeleteSprite ) );
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
	cVect Pos = getBody()->getPos();

	mSprite->setPosition( Vector2f( Pos.x, Pos.y ) );
	mSprite->setRotation( getBody()->getAngleDeg() );
	mSprite->draw();
}

void ShapeCircleSprite::offsetSet() {
	mSprite->setSize( Sizef( ShapeCircle::getRadius() * 2, ShapeCircle::getRadius() * 2 ) );
	mSprite->setOffset( Vector2i( -ShapeCircle::getRadius() + ShapeCircle::getOffset().x, -ShapeCircle::getRadius() + ShapeCircle::getOffset().y ) );
}

Sprite * ShapeCircleSprite::getSprite() const {
	return mSprite;
}

void ShapeCircleSprite::setRadius( const cpFloat& radius ) {
	ShapeCircle::setRadius( radius );
	offsetSet();
}

void ShapeCircleSprite::setOffset( const cVect &offset ) {
	ShapeCircle::setOffset( offset );
	offsetSet();
}

}}

#endif
