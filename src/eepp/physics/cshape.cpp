#include <eepp/physics/cshape.hpp>
#include <eepp/physics/cshapecircle.hpp>
#include <eepp/physics/cshapesegment.hpp>
#include <eepp/physics/cshapepoly.hpp>
#include <eepp/physics/cphysicsmanager.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/cprimitives.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

void cShape::ResetShapeIdCounter() {
	cpResetShapeIdCounter();
}

void cShape::Free( cShape * shape, bool DeleteBody ) {
	if ( DeleteBody ) {
		cBody * b = shape->Body();
		cpSAFE_DELETE( b );
	}

	cpSAFE_DELETE( shape );
}

cShape::cShape() :
	mData( NULL )
{
}

cShape::~cShape() {
	cpShapeFree( mShape );

	cPhysicsManager::instance()->RemoveShapeFree( this );
}

void cShape::SetData() {
	mShape->data	= (void*)this;
	cPhysicsManager::instance()->AddShapeFree( this );
}

cpShape * cShape::Shape() const {
	return mShape;
}

cBB cShape::CacheBB() {
	return tocbb( cpShapeCacheBB( mShape ) );
}

cBB cShape::Update( cVect pos, cVect rot ) {
	return tocbb( cpShapeUpdate( mShape, tocpv( pos ), tocpv( rot ) ) );
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

cBB cShape::BB() const {
	return tocbb( mShape->bb );
}

void cShape::BB( const cBB& bb ) {
	mShape->bb = tocpbb( bb );
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
	#ifdef PHYSICS_RENDERER_ENABLED
	cPrimitives P;
	P.SetColor( eeColorA( 76, 128, 76, 255 ) );
	P.ForceDraw( false );
	P.DrawLine( eeVector2f( mShape->bb.l, mShape->bb.t ), eeVector2f( mShape->bb.r, mShape->bb.t ) );
	P.DrawLine( eeVector2f( mShape->bb.l, mShape->bb.t ), eeVector2f( mShape->bb.l, mShape->bb.b ) );
	P.DrawLine( eeVector2f( mShape->bb.l, mShape->bb.b ), eeVector2f( mShape->bb.r, mShape->bb.b ) );
	P.DrawLine( eeVector2f( mShape->bb.r, mShape->bb.t ), eeVector2f( mShape->bb.r, mShape->bb.b ) );
	#endif
}

void * cShape::Data() const {
	return mData;
}

void cShape::Data( void * data ) {
	mData = data;
}

void cShape::Draw( cSpace * space ) {
}

void cShape::DrawBorder( cSpace * space ) {
}

CP_NAMESPACE_END
