#include "cbody.hpp"

namespace EE { namespace Physics {

cBody * cBody::New( cpFloat m, cpFloat i ) {
	return eeNew( cBody, ( m, i ) );
}

cBody * cBody::New( cpBody * body ) {
	return eeNew( cBody, ( body ) );
}

cBody * cBody::New() {
	return eeNew( cBody, () );
}

cBody::cBody( cpBody * body ) {
	mBody = body;
	mBody->data = (void*)this;
}

cBody::cBody( cpFloat m, cpFloat i ) {
	mBody = cpBodyNew( m, i );
	mBody->data = (void*)this;
}

cBody::cBody() {
	mBody = cpBodyNewStatic();
}

cBody::~cBody() {
}

void cBody::Activate() {
	cpBodyActivate( mBody );
}

void cBody::Sleep() {
	cpBodySleep( mBody );
}

void cBody::SleepWithGroup( cBody * Group ) {
	cpBodySleepWithGroup( mBody, Group->Body() );
}

bool cBody::IsSleeping() {
	return cpFalse != cpBodyIsSleeping( mBody );
}

bool cBody::IsStatic() {
	return cpFalse != cpBodyIsStatic( mBody );
}

bool cBody::IsRogue() {
	return cpFalse != cpBodyIsRogue( mBody );
}

cpBody * cBody::Body() const {
	return mBody;
}

cpFloat cBody::Mass() const {
	return cpBodyGetMass( mBody );
}

void cBody::Mass( const cpFloat& mass ) {
	cpBodySetMass( mBody, mass );
}

cpFloat cBody::Moment() const {
	return cpBodyGetMoment( mBody );
}

void cBody::Moment( const cpFloat& i ) {
	cpBodySetMoment( mBody, i );
}

cVect cBody::Pos() const {
	return tovect( cpBodyGetPos( mBody ) );
}

void cBody::Pos( const cVect& pos ) {
	cpBodySetPos( mBody, tocpv( pos ) );
}

cVect cBody::Vel() const {
	return tovect( cpBodyGetVel( mBody ) );
}

void cBody::Vel( const cVect& vel ) {
	cpBodySetVel( mBody, tocpv( vel ) );
}

cVect cBody::Force() const {
	return tovect( cpBodyGetForce( mBody ) );
}

void cBody::Force( const cVect& force ) {
	cpBodySetForce( mBody, tocpv( force ) );
}

cpFloat cBody::Angle() const {
	return cpBodyGetAngle( mBody );
}

void cBody::Angle( const cpFloat& rads ) {
	cpBodySetAngle( mBody, rads );
}

cpFloat cBody::AngleDeg() {
	return Degrees( mBody->a );
}

void cBody::AngleDeg( const cpFloat& angle ) {
	Angle( Radians( angle ) );
}

cpFloat cBody::AngVel() const {
	return cpBodyGetAngVel( mBody );
}

void cBody::AngVel( const cpFloat& rotVel ) {
	cpBodySetAngVel( mBody, rotVel );
}

cVect cBody::Rot() const {
	return tovect( cpBodyGetRot( mBody ) );
}

cpFloat cBody::VelLimit() const {
	return cpBodyGetVelLimit( mBody );
}

void cBody::VelLimit( const cpFloat& speed ) {
	cpBodySetVelLimit( mBody, speed );
}

cpFloat cBody::AngVelLimit() const {
	return cpBodyGetAngVelLimit( mBody );
}

void cBody::AngVelLimit( const cpFloat& speed ) {
	cpBodySetAngVelLimit( mBody, speed );
}

void cBody::Slew( cVect pos, cpFloat dt ) {
	cpBodySlew( mBody, tocpv( pos ), dt );
}

void cBody::UpdateVelocity( cVect gravity, cpFloat damping, cpFloat dt ) {
	cpBodyUpdateVelocity( mBody, tocpv( gravity ), damping, dt );
}

void cBody::UpdatePosition( cpFloat dt ) {
	cpBodyUpdatePosition( mBody, dt );
}

cVect cBody::Local2World( const cVect v ) {
	return tovect( cpBodyLocal2World( mBody, tocpv( v ) ) );
}

cVect cBody::World2Local( const cVect v ) {
	return tovect( cpBodyWorld2Local( mBody, tocpv( v ) ) );
}

void cBody::ApplyImpulse( const cVect j, const cVect r ) {
	cpBodyApplyImpulse( mBody, tocpv( j ), tocpv( r ) );
}

void cBody::ResetForces() {
	cpBodyResetForces( mBody );
}

void cBody::ApplyForce( const cVect f, const cVect r ) {
	cpBodyApplyForce( mBody, tocpv( f ), tocpv( r ) );
}

cpFloat cBody::KineticEnergy() {
	return cpBodyKineticEnergy( mBody );
}

}}
