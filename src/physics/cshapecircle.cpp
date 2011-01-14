#include "cshapecircle.hpp"
#include "cspace.hpp"

namespace EE { namespace Physics {

cShapeCircle * cShapeCircle::New( cBody * body, cpFloat radius, cVect offset ) {
	return eeNew( cShapeCircle, ( body, radius, offset ) );
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
	cPrimitives p;

	cpCircleShape * cs = (cpCircleShape*)mShape;
	p.SetColor( ColorForShape( mShape, space->Space() ) );

	p.DrawCircle( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y, cs->CP_PRIVATE(r) );
}

}}
