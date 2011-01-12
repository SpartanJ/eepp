#include "cbody.hpp"

namespace EE { namespace Physics {

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

cpVect cBody::Pos() const {
	return cpBodyGetPos( mBody );
}

void cBody::Pos( const cpVect& pos ) {
	cpBodySetPos( mBody, pos );
}

cpVect cBody::Vel() const {
	return cpBodyGetVel( mBody );
}

void cBody::Vel( const cpVect& vel ) {
	cpBodySetVel( mBody, vel );
}

cpVect cBody::Force() const {
	return cpBodyGetForce( mBody );
}

void cBody::Force( const cpVect& force ) {
	cpBodySetForce( mBody, force );
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

cpVect cBody::Rot() const {
	return cpBodyGetRot( mBody );
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

void cBody::Slew( cpVect pos, cpFloat dt ) {
	cpBodySlew( mBody, pos, dt );
}

void cBody::UpdateVelocity( cpVect gravity, cpFloat damping, cpFloat dt ) {
	cpBodyUpdateVelocity( mBody, gravity, damping, dt );
}

void cBody::UpdatePosition( cpFloat dt ) {
	cpBodyUpdatePosition( mBody, dt );
}

cpVect cBody::Local2World( const cpVect v ) {
	return cpBodyLocal2World( mBody, v );
}

cpVect cBody::World2Local( const cpVect v ) {
	return cpBodyWorld2Local( mBody, v );
}

void cBody::ApplyImpulse( const cpVect j, const cpVect r ) {
	cpBodyApplyImpulse( mBody, j, r );
}

void cBody::ResetForces() {
	cpBodyResetForces( mBody );
}

void cBody::ApplyForce( const cpVect f, const cpVect r ) {
	cpBodyApplyForce( mBody, f, r );
}

cpFloat cBody::KineticEnergy() {
	return cpBodyKineticEnergy( mBody );
}

}}
