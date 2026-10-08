#include <eepp/physics/shapecirclesprite.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/sprite.hpp>

namespace EE { namespace Physics {

ShapeCircleSprite* ShapeCircleSprite::New( Physics::Body* body, cpFloat radius, cVect offset,
										   SpritePtr sprite ) {
	return eeNew( ShapeCircleSprite, ( body, radius, offset, std::move( sprite ) ) );
}

ShapeCircleSprite::ShapeCircleSprite( Physics::Body* body, cpFloat radius, cVect offset,
									  SpritePtr sprite ) :
	ShapeCircle( body, radius, offset ), mSprite( std::move( sprite ) ) {
	offsetSet();
}

ShapeCircleSprite::~ShapeCircleSprite() {}

void ShapeCircleSprite::draw( Space* space ) {
	cVect Pos = getBody()->getPos();

	mSprite->setPosition( Vector2f( Pos.x, Pos.y ) );
	mSprite->setRotation( getBody()->getAngleDeg() );
	mSprite->draw();
}

void ShapeCircleSprite::offsetSet() {
	mSprite->setSize( Sizef( ShapeCircle::getRadius() * 2, ShapeCircle::getRadius() * 2 ) );
	mSprite->setOffset( Vector2i( -ShapeCircle::getRadius() + ShapeCircle::getOffset().x,
								  -ShapeCircle::getRadius() + ShapeCircle::getOffset().y ) );
}

const SpritePtr& ShapeCircleSprite::getSprite() const {
	return mSprite;
}

void ShapeCircleSprite::setRadius( const cpFloat& radius ) {
	ShapeCircle::setRadius( radius );
	offsetSet();
}

void ShapeCircleSprite::setOffset( const cVect& offset ) {
	ShapeCircle::setOffset( offset );
	offsetSet();
}

}} // namespace EE::Physics

#endif
