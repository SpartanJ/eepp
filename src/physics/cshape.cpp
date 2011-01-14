#include "cshape.hpp"
#include "cshapecircle.hpp"
#include "cshapesegment.hpp"
#include "cshapepoly.hpp"

namespace EE { namespace Physics {

void cShape::ResetShapeIdCounter() {
	cpResetShapeIdCounter();
}

cShape::cShape() {
}

cShape::~cShape() {
}

void cShape::SetData() {
	mShape->data	= (void*)this;
}

cpShape * cShape::Shape() const {
	return mShape;
}

cpBB cShape::CacheBB() {
	return cpShapeCacheBB( mShape );
}

cpBB cShape::Update( cVect pos, cVect rot ) {
	return cpShapeUpdate( mShape, tocpv( pos ), tocpv( rot ) );
}

bool cShape::PointQuery( cVect p ) {
	return 0 != cpShapePointQuery( mShape, tocpv( p ) );
}

cBody * cShape::Body() const {
	return reinterpret_cast<cBody*>( mShape->body->data );
}

void cShape::Body( cBody * body ) {
	mShape->body = body->Body();
}

cpBB cShape::BB() const {
	return mShape->bb;
}

void cShape::BB( const cpBB& bb ) {
	mShape->bb = bb;
}

bool cShape::Sensor() {
	return 0 != mShape->sensor;
}

void cShape::Sensor( const bool& sensor ) {
	mShape->sensor = sensor ? 1 : 0;
}

cpFloat cShape::e() const {
	return mShape->e;
}

void cShape::e( const cpFloat& e ) {
	mShape->e = e;
}

cpFloat cShape::Elasticity() const {
	return e();
}

void cShape::Elasticity( const cpFloat& e ) {
	this->e( e );
}

cpFloat cShape::u() const {
	return mShape->u;
}

void cShape::u( const cpFloat& u ) {
	mShape->u = u;
}

cpFloat cShape::Friction() const {
	return u();
}

void cShape::Friction( const cpFloat& u ) {
	this->u( u );
}

cVect cShape::SurfaceVel() const {
	return tovect( mShape->surface_v );
}

void cShape::SurfaceVel( const cVect& vel ) {
	mShape->surface_v = tocpv( vel );
}

cpCollisionType cShape::CollisionType()	 const {
	return mShape->collision_type;
}

void cShape::CollisionType( const cpCollisionType& type ) {
	mShape->collision_type = type;
}

cpGroup cShape::Group() const {
	return mShape->group;
}

void cShape::Group( const cpGroup& group ) {
	mShape->group = group;
}

cpLayers cShape::Layers() const {
	return mShape->layers;
}

void cShape::Layers( const cpLayers& layers ) {
	mShape->layers = layers;
}

cpShapeType cShape::Type() const {
	return mShape->CP_PRIVATE(klass)->type;
}

cShapePoly * cShape::GetAsPoly() {
	eeASSERT( Type() == CP_POLY_SHAPE );

	return reinterpret_cast<cShapePoly*>( this );
}

cShapeCircle * cShape::GetAsCircle() {
	eeASSERT( Type() == CP_CIRCLE_SHAPE );

	return reinterpret_cast<cShapeCircle*>( this );
}

cShapeSegment * cShape::GetAsSegment() {
	eeASSERT( Type() == CP_SEGMENT_SHAPE );

	return reinterpret_cast<cShapeSegment*>( this );
}

void cShape::DrawBB() {
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();
	BR->LineLoopBegin();
	BR->LineLoopSetColor( eeColorA( 76, 128, 76, 255  ) );
	BR->BatchLineLoop( mShape->bb.l, mShape->bb.b, mShape->bb.l, mShape->bb.t );
	BR->BatchLineLoop( mShape->bb.r, mShape->bb.t, mShape->bb.r, mShape->bb.b );
	BR->DrawOpt();
}

}}
