#include <eepp/physics/shapecircle.hpp>
#include <eepp/physics/space.hpp>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

ShapeCircle * ShapeCircle::New( Physics::Body * body, cpFloat radius, cVect offset ) {
	return cpNew( ShapeCircle, ( body, radius, offset ) );
}

ShapeCircle::ShapeCircle( Physics::Body * body, cpFloat radius, cVect offset ) {
	mShape	= cpCircleShapeNew( body->GetBody(), radius, tocpv( offset ) );
	SetData();
}

cVect ShapeCircle::Offset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void ShapeCircle::Offset( const cVect &offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat ShapeCircle::Radius() {
	return cpCircleShapeGetRadius( mShape );
}

void ShapeCircle::Radius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}


void ShapeCircle::Draw( Space * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	Primitives p;

	cpCircleShape * cs = (cpCircleShape*)mShape;
	p.setColor( ColorForShape( mShape, space->GetSpace() ) );

	p.drawCircle( Vector2f( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y ), cs->CP_PRIVATE(r) );
	#endif
}

CP_NAMESPACE_END
