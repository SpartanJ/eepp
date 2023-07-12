#include <eepp/physics/shapecircle.hpp>
#include <eepp/physics/space.hpp>
#include <eepp/thirdparty/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
using namespace EE::Graphics;
#endif

namespace EE { namespace Physics {

ShapeCircle* ShapeCircle::New( Physics::Body* body, cpFloat radius, cVect offset ) {
	return eeNew( ShapeCircle, ( body, radius, offset ) );
}

ShapeCircle::ShapeCircle( Physics::Body* body, cpFloat radius, cVect offset ) {
	mShape = cpCircleShapeNew( body->getBody(), radius, tocpv( offset ) );
	setData();
}

cVect ShapeCircle::getOffset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void ShapeCircle::setOffset( const cVect& offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat ShapeCircle::getRadius() {
	return cpCircleShapeGetRadius( mShape );
}

void ShapeCircle::setRadius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}

void ShapeCircle::draw( Space* space ) {
#ifdef PHYSICS_RENDERER_ENABLED
	Primitives p;

	cpCircleShape* cs = (cpCircleShape*)mShape;
	p.setColor( colorForShape( mShape, space->getSpace() ) );

	p.drawCircle( Vector2f( cs->CP_PRIVATE( tc ).x, cs->CP_PRIVATE( tc ).y ), cs->CP_PRIVATE( r ) );
#endif
}

}} // namespace EE::Physics
