#include <eepp/physics/shapepoint.hpp>
#include <eepp/physics/space.hpp>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

ShapePoint * ShapePoint::New( Physics::Body * body, cpFloat radius, cVect offset ) {
	return cpNew( ShapePoint, ( body, radius, offset ) );
}

ShapePoint::ShapePoint( Physics::Body * body, cpFloat radius, cVect offset )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawRadius( radius )
#endif
{
	mShape	= cpCircleShapeNew( body->GetBody(), radius, tocpv( offset ) );
	SetData();
}

cVect ShapePoint::Offset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void ShapePoint::Offset( const cVect &offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat ShapePoint::Radius() {
	return cpCircleShapeGetRadius( mShape );
}

void ShapePoint::Radius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}

void ShapePoint::Draw( Space * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	BatchRenderer * BR = GlobalBatchRenderer::instance();

	BR->SetPointSize( mDrawRadius );

	BR->SetTexture( NULL );
	BR->PointsBegin();
	BR->PointSetColor( ColorForShape( mShape, space->GetSpace() ) );

	cpCircleShape * cs = (cpCircleShape*)mShape;

	BR->BatchPoint( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y );

	BR->DrawOpt();
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat ShapePoint::DrawRadius() {
	return mDrawRadius;
}

void ShapePoint::DrawRadius( const cpFloat& radius ) {
	mDrawRadius = radius;
}
#endif

CP_NAMESPACE_END
