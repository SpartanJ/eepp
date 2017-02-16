#include <eepp/physics/shape.hpp>
#include <eepp/physics/shapecircle.hpp>
#include <eepp/physics/shapesegment.hpp>
#include <eepp/physics/shapepoly.hpp>
#include <eepp/physics/physicsmanager.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

void Shape::resetShapeIdCounter() {
	cpResetShapeIdCounter();
}

void Shape::Free( Shape * shape, bool DeleteBody ) {
	if ( DeleteBody ) {
		Physics::Body * b = shape->body();
		cpSAFE_DELETE( b );
	}

	cpSAFE_DELETE( shape );
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

Physics::Body * Shape::body() const {
	return reinterpret_cast<Physics::Body*>( mShape->body->data );
}

void Shape::body( Physics::Body * body ) {
	mShape->body = body->getBody();
}

cBB Shape::bb() const {
	return tocbb( mShape->bb );
}

void Shape::bb( const cBB& bb ) {
	mShape->bb = tocpbb( bb );
}

bool Shape::Sensor() {
	return 0 != mShape->sensor;
}

void Shape::Sensor( const bool& sensor ) {
	mShape->sensor = sensor ? 1 : 0;
}

cpFloat Shape::e() const {
	return mShape->e;
}

void Shape::e( const cpFloat& e ) {
	mShape->e = e;
}

cpFloat Shape::elasticity() const {
	return e();
}

void Shape::elasticity( const cpFloat& e ) {
	this->e( e );
}

cpFloat Shape::u() const {
	return mShape->u;
}

void Shape::u( const cpFloat& u ) {
	mShape->u = u;
}

cpFloat Shape::friction() const {
	return u();
}

void Shape::friction( const cpFloat& u ) {
	this->u( u );
}

cVect Shape::surfaceVel() const {
	return tovect( mShape->surface_v );
}

void Shape::surfaceVel( const cVect& vel ) {
	mShape->surface_v = tocpv( vel );
}

cpCollisionType Shape::collisionType()	 const {
	return mShape->collision_type;
}

void Shape::collisionType( const cpCollisionType& type ) {
	mShape->collision_type = type;
}

cpGroup Shape::group() const {
	return mShape->group;
}

void Shape::group( const cpGroup& group ) {
	mShape->group = group;
}

cpLayers Shape::layers() const {
	return mShape->layers;
}

void Shape::layers( const cpLayers& layers ) {
	mShape->layers = layers;
}

cpShapeType Shape::type() const {
	return mShape->CP_PRIVATE(klass)->type;
}

ShapePoly * Shape::getAsPoly() {
	eeASSERT( type() == CP_POLY_SHAPE );

	return reinterpret_cast<ShapePoly*>( this );
}

ShapeCircle * Shape::getAsCircle() {
	eeASSERT( type() == CP_CIRCLE_SHAPE );

	return reinterpret_cast<ShapeCircle*>( this );
}

ShapeSegment * Shape::getAsSegment() {
	eeASSERT( type() == CP_SEGMENT_SHAPE );

	return reinterpret_cast<ShapeSegment*>( this );
}

void Shape::drawBB() {
	#ifdef PHYSICS_RENDERER_ENABLED
	Primitives P;
	P.setColor( ColorA( 76, 128, 76, 255 ) );
	P.forceDraw( false );
	P.drawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.t ), Vector2f( mShape->bb.r, mShape->bb.t ) ) );
	P.drawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.t ), Vector2f( mShape->bb.l, mShape->bb.b ) ) );
	P.drawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.b ), Vector2f( mShape->bb.r, mShape->bb.b ) ) );
	P.drawLine( Line2f( Vector2f( mShape->bb.r, mShape->bb.t ), Vector2f( mShape->bb.r, mShape->bb.b ) ) );
	#endif
}

void * Shape::data() const {
	return mData;
}

void Shape::data( void * data ) {
	mData = data;
}

void Shape::draw( Space * space ) {
}

void Shape::drawBorder( Space * space ) {
}

CP_NAMESPACE_END
