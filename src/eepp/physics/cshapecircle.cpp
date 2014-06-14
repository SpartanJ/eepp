#include <eepp/physics/cshapecircle.hpp>
#include <eepp/physics/cspace.hpp>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/cprimitives.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

cShapeCircle * cShapeCircle::New( cBody * body, cpFloat radius, cVect offset ) {
	return cpNew( cShapeCircle, ( body, radius, offset ) );
}

cShapeCircle::cShapeCircle( cBody * body, cpFloat radius, cVect offset ) {
	mShape	= cpCircleShapeNew( body->Body(), radius, tocpv( offset ) );
	SetData();
}

cVect cShapeCircle::Offset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void cShapeCircle::Offset( const cVect &offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat cShapeCircle::Radius() {
	return cpCircleShapeGetRadius( mShape );
}

void cShapeCircle::Radius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}


void cShapeCircle::Draw( cSpace * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	cPrimitives p;

	cpCircleShape * cs = (cpCircleShape*)mShape;
	p.SetColor( ColorForShape( mShape, space->Space() ) );

	p.DrawCircle( Vector2f( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y ), cs->CP_PRIVATE(r) );
	#endif
}

CP_NAMESPACE_END
