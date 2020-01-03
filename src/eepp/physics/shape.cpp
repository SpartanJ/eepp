#include <eepp/physics/shape.hpp>
#include <eepp/physics/shapecircle.hpp>
#include <eepp/physics/shapesegment.hpp>
#include <eepp/physics/shapepoly.hpp>
#include <eepp/physics/physicsmanager.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;
#endif

namespace EE { namespace Physics {

void Shape::resetShapeIdCounter() {
	cpResetShapeIdCounter();
}

void Shape::Free( Shape * shape, bool DeleteBody ) {
	if ( DeleteBody ) {
		Physics::Body * b = shape->getBody();
		eeSAFE_DELETE( b );
	}

	eeSAFE_DELETE( shape );
}

Shape::Shape() :
	mData( NULL )
{
}

Shape::~Shape() {
	cpShapeFree( mShape );

	PhysicsManager::instance()->removeShapeFree( this );
}

void Shape::setData() {
	mShape->data	= (void*)this;
	PhysicsManager::instance()->addShapeFree( this );
}

cpShape * Shape::getShape() const {
	return mShape;
}

cBB Shape::cacheBB() {
	return tocbb( cpShapeCacheBB( mShape ) );
}

cBB Shape::update( cVect pos, cVect rot ) {
	return tocbb( cpShapeUpdate( mShape, tocpv( pos ), tocpv( rot ) ) );
}

bool Shape::pointQuery( cVect p ) {
	return 0 != cpShapePointQuery( mShape, tocpv( p ) );
}

Physics::Body * Shape::getBody() const {
	return reinterpret_cast<Physics::Body*>( mShape->body->data );
}

void Shape::setBody( Physics::Body * body ) {
	mShape->body = body->getBody();
}

cBB Shape::getBB() const {
	return tocbb( mShape->bb );
}

void Shape::setBB( const cBB& bb ) {
	mShape->bb = tocpbb( bb );
}

bool Shape::isSensor() {
	return 0 != mShape->sensor;
}

void Shape::setSensor( const bool& sensor ) {
	mShape->sensor = sensor ? 1 : 0;
}

cpFloat Shape::getE() const {
	return mShape->e;
}

void Shape::setE( const cpFloat& e ) {
	mShape->e = e;
}

cpFloat Shape::getElasticity() const {
	return getE();
}

void Shape::setElasticity( const cpFloat& e ) {
	this->setE( e );
}

cpFloat Shape::getU() const {
	return mShape->u;
}

void Shape::setU( const cpFloat& u ) {
	mShape->u = u;
}

cpFloat Shape::getFriction() const {
	return getU();
}

void Shape::setFriction( const cpFloat& u ) {
	this->setU( u );
}

cVect Shape::getSurfaceVel() const {
	return tovect( mShape->surface_v );
}

void Shape::getSurfaceVel( const cVect& vel ) {
	mShape->surface_v = tocpv( vel );
}

cpCollisionType Shape::getCollisionType()	 const {
	return mShape->collision_type;
}

void Shape::setCollisionType( const cpCollisionType& type ) {
	mShape->collision_type = type;
}

cpGroup Shape::getGroup() const {
	return mShape->group;
}

void Shape::setGroup( const cpGroup& group ) {
	mShape->group = group;
}

cpLayers Shape::getLayers() const {
	return mShape->layers;
}

void Shape::setLayers( const cpLayers& layers ) {
	mShape->layers = layers;
}

cpShapeType Shape::getType() const {
	return mShape->CP_PRIVATE(klass)->type;
}

ShapePoly * Shape::getAsPoly() {
	eeASSERT( getType() == CP_POLY_SHAPE );

	return reinterpret_cast<ShapePoly*>( this );
}

ShapeCircle * Shape::getAsCircle() {
	eeASSERT( getType() == CP_CIRCLE_SHAPE );

	return reinterpret_cast<ShapeCircle*>( this );
}

ShapeSegment * Shape::getAsSegment() {
	eeASSERT( getType() == CP_SEGMENT_SHAPE );

	return reinterpret_cast<ShapeSegment*>( this );
}

void Shape::drawBB() {
	#ifdef PHYSICS_RENDERER_ENABLED
	Primitives P;
	P.setColor( Color( 76, 128, 76, 255 ) );
	P.setForceDraw( false );
	P.drawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.t ), Vector2f( mShape->bb.r, mShape->bb.t ) ) );
	P.drawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.t ), Vector2f( mShape->bb.l, mShape->bb.b ) ) );
	P.drawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.b ), Vector2f( mShape->bb.r, mShape->bb.b ) ) );
	P.drawLine( Line2f( Vector2f( mShape->bb.r, mShape->bb.t ), Vector2f( mShape->bb.r, mShape->bb.b ) ) );
	#endif
}

void * Shape::getData() const {
	return mData;
}

void Shape::setData( void * data ) {
	mData = data;
}

void Shape::draw( Space * space ) {
}

void Shape::drawBorder( Space * space ) {
}

}}
