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

void Shape::ResetShapeIdCounter() {
	cpResetShapeIdCounter();
}

void Shape::Free( Shape * shape, bool DeleteBody ) {
	if ( DeleteBody ) {
		Physics::Body * b = shape->Body();
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

	PhysicsManager::instance()->RemoveShapeFree( this );
}

void Shape::SetData() {
	mShape->data	= (void*)this;
	PhysicsManager::instance()->AddShapeFree( this );
}

cpShape * Shape::GetShape() const {
	return mShape;
}

cBB Shape::CacheBB() {
	return tocbb( cpShapeCacheBB( mShape ) );
}

cBB Shape::Update( cVect pos, cVect rot ) {
	return tocbb( cpShapeUpdate( mShape, tocpv( pos ), tocpv( rot ) ) );
}

bool Shape::PointQuery( cVect p ) {
	return 0 != cpShapePointQuery( mShape, tocpv( p ) );
}

Physics::Body * Shape::Body() const {
	return reinterpret_cast<Physics::Body*>( mShape->body->data );
}

void Shape::Body( Physics::Body * body ) {
	mShape->body = body->GetBody();
}

cBB Shape::BB() const {
	return tocbb( mShape->bb );
}

void Shape::BB( const cBB& bb ) {
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

cpFloat Shape::Elasticity() const {
	return e();
}

void Shape::Elasticity( const cpFloat& e ) {
	this->e( e );
}

cpFloat Shape::u() const {
	return mShape->u;
}

void Shape::u( const cpFloat& u ) {
	mShape->u = u;
}

cpFloat Shape::Friction() const {
	return u();
}

void Shape::Friction( const cpFloat& u ) {
	this->u( u );
}

cVect Shape::SurfaceVel() const {
	return tovect( mShape->surface_v );
}

void Shape::SurfaceVel( const cVect& vel ) {
	mShape->surface_v = tocpv( vel );
}

cpCollisionType Shape::CollisionType()	 const {
	return mShape->collision_type;
}

void Shape::CollisionType( const cpCollisionType& type ) {
	mShape->collision_type = type;
}

cpGroup Shape::Group() const {
	return mShape->group;
}

void Shape::Group( const cpGroup& group ) {
	mShape->group = group;
}

cpLayers Shape::Layers() const {
	return mShape->layers;
}

void Shape::Layers( const cpLayers& layers ) {
	mShape->layers = layers;
}

cpShapeType Shape::Type() const {
	return mShape->CP_PRIVATE(klass)->type;
}

ShapePoly * Shape::GetAsPoly() {
	eeASSERT( Type() == CP_POLY_SHAPE );

	return reinterpret_cast<ShapePoly*>( this );
}

ShapeCircle * Shape::GetAsCircle() {
	eeASSERT( Type() == CP_CIRCLE_SHAPE );

	return reinterpret_cast<ShapeCircle*>( this );
}

ShapeSegment * Shape::GetAsSegment() {
	eeASSERT( Type() == CP_SEGMENT_SHAPE );

	return reinterpret_cast<ShapeSegment*>( this );
}

void Shape::DrawBB() {
	#ifdef PHYSICS_RENDERER_ENABLED
	Primitives P;
	P.SetColor( ColorA( 76, 128, 76, 255 ) );
	P.ForceDraw( false );
	P.DrawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.t ), Vector2f( mShape->bb.r, mShape->bb.t ) ) );
	P.DrawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.t ), Vector2f( mShape->bb.l, mShape->bb.b ) ) );
	P.DrawLine( Line2f( Vector2f( mShape->bb.l, mShape->bb.b ), Vector2f( mShape->bb.r, mShape->bb.b ) ) );
	P.DrawLine( Line2f( Vector2f( mShape->bb.r, mShape->bb.t ), Vector2f( mShape->bb.r, mShape->bb.b ) ) );
	#endif
}

void * Shape::Data() const {
	return mData;
}

void Shape::Data( void * data ) {
	mData = data;
}

void Shape::Draw( Space * space ) {
}

void Shape::DrawBorder( Space * space ) {
}

CP_NAMESPACE_END
