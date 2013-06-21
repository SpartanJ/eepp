#include <eepp/physics/cshapepoint.hpp>
#include <eepp/physics/cspace.hpp>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/cprimitives.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

cShapePoint * cShapePoint::New( cBody * body, cpFloat radius, cVect offset ) {
	return cpNew( cShapePoint, ( body, radius, offset ) );
}

cShapePoint::cShapePoint( cBody * body, cpFloat radius, cVect offset )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawRadius( radius )
#endif
{
	mShape	= cpCircleShapeNew( body->Body(), radius, tocpv( offset ) );
	SetData();
}

cVect cShapePoint::Offset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void cShapePoint::Offset( const cVect &offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat cShapePoint::Radius() {
	return cpCircleShapeGetRadius( mShape );
}

void cShapePoint::Radius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}

void cShapePoint::Draw( cSpace * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	BR->SetPointSize( mDrawRadius );

	BR->SetTexture( NULL );
	BR->PointsBegin();
	BR->PointSetColor( ColorForShape( mShape, space->Space() ) );

	cpCircleShape * cs = (cpCircleShape*)mShape;

	BR->BatchPoint( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y );

	BR->DrawOpt();
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat cShapePoint::DrawRadius() {
	return mDrawRadius;
}

void cShapePoint::DrawRadius( const cpFloat& radius ) {
	mDrawRadius = radius;
}
#endif

CP_NAMESPACE_END
