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
	mShape	= cpCircleShapeNew( body->getBody(), radius, tocpv( offset ) );
	setData();
}

cVect ShapePoint::getOffset() {
	return tovect( cpCircleShapeGetOffset( mShape ) );
}

void ShapePoint::setOffset( const cVect &offset ) {
	cpCircleShapeSetOffset( mShape, tocpv( offset ) );
}

cpFloat ShapePoint::getRadius() {
	return cpCircleShapeGetRadius( mShape );
}

void ShapePoint::setRadius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}

void ShapePoint::draw( Space * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	BatchRenderer * BR = GlobalBatchRenderer::instance();

	BR->setPointSize( mDrawRadius );

	BR->setTexture( NULL );
	BR->pointsBegin();
	BR->pointSetColor( colorForShape( mShape, space->getSpace() ) );

	cpCircleShape * cs = (cpCircleShape*)mShape;

	BR->batchPoint( cs->CP_PRIVATE(tc).x, cs->CP_PRIVATE(tc).y );

	BR->drawOpt();
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat ShapePoint::getDrawRadius() {
	return mDrawRadius;
}

void ShapePoint::setDrawRadius( const cpFloat& radius ) {
	mDrawRadius = radius;
}
#endif

CP_NAMESPACE_END
