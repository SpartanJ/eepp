#include <eepp/physics/body.hpp>
#include <eepp/physics/physicsmanager.hpp>
#include <eepp/physics/shape.hpp>
#include <eepp/physics/constraints/constraint.hpp>
#include <eepp/physics/arbiter.hpp>

CP_NAMESPACE_BEGIN

Body * Body::New( cpFloat m, cpFloat i ) {
	return cpNew( Body, ( m, i ) );
}

Body * Body::New( cpBody * body ) {
	return cpNew( Body, ( body ) );
}

Body * Body::New() {
	return cpNew( Body, () );
}

void Body::Free( Body * body ) {
	cpSAFE_DELETE( body );
}

Body::Body( cpBody * body ) :
	mBody( body ),
	mData( NULL )
{
	setData();
}

Body::Body( cpFloat m, cpFloat i ) :
	mBody( cpBodyNew( m, i ) ),
	mData( NULL )
{
	setData();
}

Body::Body() :
	mBody( cpBodyNewStatic() ),
	mData( NULL )
{
	setData();
}

Body::~Body() {
	if ( NULL != mBody )
		cpBodyFree( mBody );

	PhysicsManager::instance()->removeBodyFree( this );
}

void Body::setData() {
	mBody->data = (void*)this;

	PhysicsManager::instance()->addBodyFree( this );
}

void Body::activate() {
	cpBodyActivate( mBody );
}

void Body::activateStatic( Body *body, Shape * filter ) {
	cpBodyActivateStatic( mBody, filter->getShape() );
}

void Body::sleep() {
	cpBodySleep( mBody );
}

void Body::sleepWithGroup( Body * Group ) {
	cpBodySleepWithGroup( mBody, Group->getBody() );
}

bool Body::isSleeping() {
	return cpFalse != cpBodyIsSleeping( mBody );
}

bool Body::isStatic() {
	return cpFalse != cpBodyIsStatic( mBody );
}

bool Body::isRogue() {
	return cpFalse != cpBodyIsRogue( mBody );
}

cpBody * Body::getBody() const {
	return mBody;
}

cpFloat Body::mass() const {
	return cpBodyGetMass( mBody );
}

void Body::mass( const cpFloat& mass ) {
	cpBodySetMass( mBody, mass );
}

cpFloat Body::moment() const {
	return cpBodyGetMoment( mBody );
}

void Body::moment( const cpFloat& i ) {
	cpBodySetMoment( mBody, i );
}

cVect Body::pos() const {
	return tovect( cpBodyGetPos( mBody ) );
}

void Body::pos( const cVect& pos ) {
	cpBodySetPos( mBody, tocpv( pos ) );
}

cVect Body::vel() const {
	return tovect( cpBodyGetVel( mBody ) );
}

void Body::vel( const cVect& vel ) {
	cpBodySetVel( mBody, tocpv( vel ) );
}

cVect Body::force() const {
	return tovect( cpBodyGetForce( mBody ) );
}

void Body::force( const cVect& force ) {
	cpBodySetForce( mBody, tocpv( force ) );
}

cpFloat Body::angle() const {
	return cpBodyGetAngle( mBody );
}

void Body::angle( const cpFloat& rads ) {
	cpBodySetAngle( mBody, rads );
}

cpFloat Body::angleDeg() {
	return cpDegrees( mBody->a );
}

void Body::angleDeg( const cpFloat& angle ) {
	this->angle( cpRadians( angle ) );
}

cpFloat Body::angVel() const {
	return cpBodyGetAngVel( mBody );
}

void Body::angVel( const cpFloat& rotVel ) {
	cpBodySetAngVel( mBody, rotVel );
}

cpFloat Body::torque() const {
	return cpBodyGetTorque( mBody );
}

void Body::torque( const cpFloat& torque ) {
	cpBodySetTorque( mBody, torque );
}

cVect Body::rot() const {
	return tovect( cpBodyGetRot( mBody ) );
}

cpFloat Body::velLimit() const {
	return cpBodyGetVelLimit( mBody );
}

void Body::velLimit( const cpFloat& speed ) {
	cpBodySetVelLimit( mBody, speed );
}

cpFloat Body::angVelLimit() const {
	return cpBodyGetAngVelLimit( mBody );
}

void Body::angVelLimit( const cpFloat& speed ) {
	cpBodySetAngVelLimit( mBody, speed );
}

void Body::updateVelocity( cVect gravity, cpFloat damping, cpFloat dt ) {
	cpBodyUpdateVelocity( mBody, tocpv( gravity ), damping, dt );
}

void Body::updatePosition( cpFloat dt ) {
	cpBodyUpdatePosition( mBody, dt );
}

void Body::bodyVelocityFuncWrapper( cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt ) {
	Body * tBody = reinterpret_cast<Body*>( body->data );

	if ( tBody->mVelocityFunc.IsSet() ) {
		tBody->mVelocityFunc( reinterpret_cast<Body*>( body->data ), tovect( gravity ), damping, dt );
	}
}

void Body::velocityFunc( BodyVelocityFunc func ) {
	mBody->velocity_func = &Body::bodyVelocityFuncWrapper;

	mVelocityFunc = func;
}

void Body::bodyPositionFuncWrapper( cpBody* body, cpFloat dt ) {
	Body * tBody = reinterpret_cast<Body*>( body->data );

	if ( tBody->mPositionFunc.IsSet() ) {
		tBody->mPositionFunc( reinterpret_cast<Body*>( body->data ), dt );
	}
}

void Body::positionFunc( BodyPositionFunc func ) {
	mBody->position_func = &Body::bodyPositionFuncWrapper;

	mPositionFunc = func;
}

cVect Body::local2World( const cVect v ) {
	return tovect( cpBodyLocal2World( mBody, tocpv( v ) ) );
}

cVect Body::world2Local( const cVect v ) {
	return tovect( cpBodyWorld2Local( mBody, tocpv( v ) ) );
}

void Body::applyImpulse( const cVect j, const cVect r ) {
	cpBodyApplyImpulse( mBody, tocpv( j ), tocpv( r ) );
}

void Body::resetForces() {
	cpBodyResetForces( mBody );
}

void Body::applyForce( const cVect f, const cVect r ) {
	cpBodyApplyForce( mBody, tocpv( f ), tocpv( r ) );
}

cpFloat Body::kineticEnergy() {
	return cpBodyKineticEnergy( mBody );
}

void * Body::data() const {
	return mData;
}

void Body::data( void * data ) {
	mData = data;
}

static void BodyShapeIteratorFunc ( cpBody * body, cpShape * shape, void * data ) {
	Body::ShapeIterator * it = reinterpret_cast<Body::ShapeIterator *> ( data );
	it->Body->onEachShape( reinterpret_cast<Shape*>( shape->data ), it );
}

void Body::eachShape( ShapeIteratorFunc Func, void * data ) {
	ShapeIterator it( this, data, Func );
	cpBodyEachShape( mBody, &BodyShapeIteratorFunc, (void*)&it );
}

void Body::onEachShape( Shape * Shape, ShapeIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( it->Body, Shape, it->Data );
	}
}

static void BodyConstraintIteratorFunc( cpBody * body, cpConstraint * constraint, void * data ) {
	Body::ConstraintIterator * it = reinterpret_cast<Body::ConstraintIterator *> ( data );
	it->Body->onEachConstraint( reinterpret_cast<Constraint*> ( constraint->data ), it );
}

void Body::eachConstraint( ConstraintIteratorFunc Func, void * data ) {
	ConstraintIterator it( this, data, Func );
	cpBodyEachConstraint( mBody, &BodyConstraintIteratorFunc, (void*)&it );
}

void Body::onEachConstraint( Constraint * Constraint, ConstraintIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( this, Constraint, it->Data );
	}
}

static void BodyArbiterIteratorFunc( cpBody * body, cpArbiter * arbiter, void * data ) {
	Body::ArbiterIterator * it = reinterpret_cast<Body::ArbiterIterator *> ( data );
	Arbiter tarb( arbiter );
	it->Body->onEachArbiter( &tarb, it );
}

void Body::eachArbiter( ArbiterIteratorFunc Func, void * data ) {
	ArbiterIterator it( this, data, Func );
	cpBodyEachArbiter( mBody, &BodyArbiterIteratorFunc, (void*)&it );
}

void Body::onEachArbiter( Arbiter * Arbiter, ArbiterIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( this, Arbiter, it->Data );
	}
}

CP_NAMESPACE_END
