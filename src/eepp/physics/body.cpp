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
	SetData();
}

Body::Body( cpFloat m, cpFloat i ) :
	mBody( cpBodyNew( m, i ) ),
	mData( NULL )
{
	SetData();
}

Body::Body() :
	mBody( cpBodyNewStatic() ),
	mData( NULL )
{
	SetData();
}

Body::~Body() {
	if ( NULL != mBody )
		cpBodyFree( mBody );

	PhysicsManager::instance()->RemoveBodyFree( this );
}

void Body::SetData() {
	mBody->data = (void*)this;

	PhysicsManager::instance()->AddBodyFree( this );
}

void Body::Activate() {
	cpBodyActivate( mBody );
}

void Body::ActivateStatic( Body *body, Shape * filter ) {
	cpBodyActivateStatic( mBody, filter->GetShape() );
}

void Body::Sleep() {
	cpBodySleep( mBody );
}

void Body::SleepWithGroup( Body * Group ) {
	cpBodySleepWithGroup( mBody, Group->GetBody() );
}

bool Body::IsSleeping() {
	return cpFalse != cpBodyIsSleeping( mBody );
}

bool Body::IsStatic() {
	return cpFalse != cpBodyIsStatic( mBody );
}

bool Body::IsRogue() {
	return cpFalse != cpBodyIsRogue( mBody );
}

cpBody * Body::GetBody() const {
	return mBody;
}

cpFloat Body::Mass() const {
	return cpBodyGetMass( mBody );
}

void Body::Mass( const cpFloat& mass ) {
	cpBodySetMass( mBody, mass );
}

cpFloat Body::Moment() const {
	return cpBodyGetMoment( mBody );
}

void Body::Moment( const cpFloat& i ) {
	cpBodySetMoment( mBody, i );
}

cVect Body::Pos() const {
	return tovect( cpBodyGetPos( mBody ) );
}

void Body::Pos( const cVect& pos ) {
	cpBodySetPos( mBody, tocpv( pos ) );
}

cVect Body::Vel() const {
	return tovect( cpBodyGetVel( mBody ) );
}

void Body::Vel( const cVect& vel ) {
	cpBodySetVel( mBody, tocpv( vel ) );
}

cVect Body::Force() const {
	return tovect( cpBodyGetForce( mBody ) );
}

void Body::Force( const cVect& force ) {
	cpBodySetForce( mBody, tocpv( force ) );
}

cpFloat Body::Angle() const {
	return cpBodyGetAngle( mBody );
}

void Body::Angle( const cpFloat& rads ) {
	cpBodySetAngle( mBody, rads );
}

cpFloat Body::AngleDeg() {
	return cpDegrees( mBody->a );
}

void Body::AngleDeg( const cpFloat& angle ) {
	Angle( cpRadians( angle ) );
}

cpFloat Body::AngVel() const {
	return cpBodyGetAngVel( mBody );
}

void Body::AngVel( const cpFloat& rotVel ) {
	cpBodySetAngVel( mBody, rotVel );
}

cpFloat Body::Torque() const {
	return cpBodyGetTorque( mBody );
}

void Body::Torque( const cpFloat& torque ) {
	cpBodySetTorque( mBody, torque );
}

cVect Body::Rot() const {
	return tovect( cpBodyGetRot( mBody ) );
}

cpFloat Body::VelLimit() const {
	return cpBodyGetVelLimit( mBody );
}

void Body::VelLimit( const cpFloat& speed ) {
	cpBodySetVelLimit( mBody, speed );
}

cpFloat Body::AngVelLimit() const {
	return cpBodyGetAngVelLimit( mBody );
}

void Body::AngVelLimit( const cpFloat& speed ) {
	cpBodySetAngVelLimit( mBody, speed );
}

void Body::UpdateVelocity( cVect gravity, cpFloat damping, cpFloat dt ) {
	cpBodyUpdateVelocity( mBody, tocpv( gravity ), damping, dt );
}

void Body::UpdatePosition( cpFloat dt ) {
	cpBodyUpdatePosition( mBody, dt );
}

void Body::BodyVelocityFuncWrapper( cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt ) {
	Body * tBody = reinterpret_cast<Body*>( body->data );

	if ( tBody->mVelocityFunc.IsSet() ) {
		tBody->mVelocityFunc( reinterpret_cast<Body*>( body->data ), tovect( gravity ), damping, dt );
	}
}

void Body::VelocityFunc( BodyVelocityFunc func ) {
	mBody->velocity_func = &Body::BodyVelocityFuncWrapper;

	mVelocityFunc = func;
}

void Body::BodyPositionFuncWrapper( cpBody* body, cpFloat dt ) {
	Body * tBody = reinterpret_cast<Body*>( body->data );

	if ( tBody->mPositionFunc.IsSet() ) {
		tBody->mPositionFunc( reinterpret_cast<Body*>( body->data ), dt );
	}
}

void Body::PositionFunc( BodyPositionFunc func ) {
	mBody->position_func = &Body::BodyPositionFuncWrapper;

	mPositionFunc = func;
}

cVect Body::Local2World( const cVect v ) {
	return tovect( cpBodyLocal2World( mBody, tocpv( v ) ) );
}

cVect Body::World2Local( const cVect v ) {
	return tovect( cpBodyWorld2Local( mBody, tocpv( v ) ) );
}

void Body::ApplyImpulse( const cVect j, const cVect r ) {
	cpBodyApplyImpulse( mBody, tocpv( j ), tocpv( r ) );
}

void Body::ResetForces() {
	cpBodyResetForces( mBody );
}

void Body::ApplyForce( const cVect f, const cVect r ) {
	cpBodyApplyForce( mBody, tocpv( f ), tocpv( r ) );
}

cpFloat Body::KineticEnergy() {
	return cpBodyKineticEnergy( mBody );
}

void * Body::Data() const {
	return mData;
}

void Body::Data( void * data ) {
	mData = data;
}

static void BodyShapeIteratorFunc ( cpBody * body, cpShape * shape, void * data ) {
	Body::ShapeIterator * it = reinterpret_cast<Body::ShapeIterator *> ( data );
	it->Body->OnEachShape( reinterpret_cast<Shape*>( shape->data ), it );
}

void Body::EachShape( ShapeIteratorFunc Func, void * data ) {
	ShapeIterator it( this, data, Func );
	cpBodyEachShape( mBody, &BodyShapeIteratorFunc, (void*)&it );
}

void Body::OnEachShape( Shape * Shape, ShapeIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( it->Body, Shape, it->Data );
	}
}

static void BodyConstraintIteratorFunc( cpBody * body, cpConstraint * constraint, void * data ) {
	Body::ConstraintIterator * it = reinterpret_cast<Body::ConstraintIterator *> ( data );
	it->Body->OnEachConstraint( reinterpret_cast<Constraint*> ( constraint->data ), it );
}

void Body::EachConstraint( ConstraintIteratorFunc Func, void * data ) {
	ConstraintIterator it( this, data, Func );
	cpBodyEachConstraint( mBody, &BodyConstraintIteratorFunc, (void*)&it );
}

void Body::OnEachConstraint( Constraint * Constraint, ConstraintIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( this, Constraint, it->Data );
	}
}

static void BodyArbiterIteratorFunc( cpBody * body, cpArbiter * arbiter, void * data ) {
	Body::ArbiterIterator * it = reinterpret_cast<Body::ArbiterIterator *> ( data );
	Arbiter tarb( arbiter );
	it->Body->OnEachArbiter( &tarb, it );
}

void Body::EachArbiter( ArbiterIteratorFunc Func, void * data ) {
	ArbiterIterator it( this, data, Func );
	cpBodyEachArbiter( mBody, &BodyArbiterIteratorFunc, (void*)&it );
}

void Body::OnEachArbiter( Arbiter * Arbiter, ArbiterIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( this, Arbiter, it->Data );
	}
}

CP_NAMESPACE_END
