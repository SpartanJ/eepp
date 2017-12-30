#include <eepp/physics/shapecircle.hpp>
#include <eepp/physics/space.hpp>
#include <chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

ShapeCircle * ShapeCircle::New( Physics::Body * body, cpFloat radius, cVect offset ) {
	return cpNew( ShapeCircle, ( body, radius, offset ) );
}

ShapeCircle::ShapeCircle( Physics::Body * body, cpFloat radius, cVect offset ) {
	mShape	= cpCircleShapeNew( body->getBody(), radius, tocpv( offset ) );
	setData();
}

cVect ShapeCircle::getOffset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void ShapeCircle::setOffset( const cVect &offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat ShapeCircle::getRadius() {
	return cpCircleShapeGetRadius( mShape );
}

void ShapeCircle::setRadius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}


void ShapeCircle::draw( Space * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	Primitives p;

	cpCircleShape * cs = (cpCircleShape*)mShape;
	p.setColor( colorForShape( mShape, space->getSpace() ) );

	p.drawCircle( Vector2f( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y ), cs->CP_PRIVATE(r) );
	#endif
}

CP_NAMESPACE_END
