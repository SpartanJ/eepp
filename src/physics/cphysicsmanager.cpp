#include "cphysicsmanager.hpp"

namespace EE { namespace Physics {

cPhysicsManager::cPhysicsManager() {
	cpInitChipmunk();
}

cPhysicsManager::~cPhysicsManager() {
}

const cpFloat& cPhysicsManager::BiasCoef() const {
	return cp_bias_coef;
}

void cPhysicsManager::BiasCoef( const cpFloat& biasCoef ) {
	cp_bias_coef = biasCoef;
}

const cpFloat& cPhysicsManager::ConstraintBiasCoef() const {
	return cp_constraint_bias_coef;
}

void cPhysicsManager::ConstraintBiasCoef( const cpFloat& constraintBiasCoef ) {
	cp_constraint_bias_coef = constraintBiasCoef;
}

const cpTimestamp& cPhysicsManager::ContactPersistence() const {
	return cp_contact_persistence;
}

void cPhysicsManager::ContactPersistence( const cpTimestamp& timestamp ) {
	cp_contact_persistence = timestamp;
}

const cpFloat& cPhysicsManager::CollisionSlop() const {
	return cp_collision_slop;
}

void cPhysicsManager::CollisionSlop( const cpFloat& slop ) {
	cp_collision_slop = slop;
}

cPhysicsManager::cDrawSpaceOptions * cPhysicsManager::GetDrawOptions() {
	return &mOptions;
}

}}
