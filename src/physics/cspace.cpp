#include "cspace.hpp"
#include "cphysicsmanager.hpp"

namespace EE { namespace Physics {

cSpace::cSpace() {
	mSpace = cpSpaceNew();
	mSpace->data = (void*)this;
	mStaticBody = eeNew( cBody, ( mSpace->staticBody ) );
}

cSpace::~cSpace() {
	cpSpaceFreeChildren( mSpace );
	cpSpaceFree( mSpace );

	std::list<cConstraint*>::iterator itc = mConstraints.begin();
	for ( ; itc != mConstraints.end(); itc++ )
		eeSAFE_DELETE( *itc );

	std::list<cShape*>::iterator its = mShapes.begin();
	for ( ; its != mShapes.end(); its++ )
		eeSAFE_DELETE( *its );

	std::list<cBody*>::iterator itb = mBodys.begin();
	for ( ; itb != mBodys.end(); itb++ )
		eeSAFE_DELETE( *itb );

	eeSAFE_DELETE( mStaticBody );
}

void cSpace::Update( const cpFloat& dt ) {
	cpSpaceStep( mSpace, dt );
}

void cSpace::Update() {
	Update( cEngine::instance()->Elapsed() / 1000 );
}

void cSpace::ResizeStaticHash( cpFloat dim, int count ) {
	cpSpaceResizeStaticHash( mSpace, dim, count );
}

void cSpace::ResizeActiveHash( cpFloat dim, int count ) {
	cpSpaceResizeActiveHash( mSpace, dim, count );
}

void cSpace::RehashStatic() {
	cpSpaceRehashStatic( mSpace );
}

const int& cSpace::Iterations() const {
	return mSpace->iterations;
}

void cSpace::Iterations( const int& iterations ) {
	mSpace->iterations = iterations;
}

const cpVect& cSpace::Gravity() const {
	return mSpace->gravity;
}

void cSpace::Gravity( const cpVect& gravity ) {
	mSpace->gravity = gravity;
}

const cpFloat& cSpace::Damping() const {
	return mSpace->damping;
}

void cSpace::Damping( const cpFloat& damping ) {
	mSpace->damping = damping;
}

const cpFloat& cSpace::IdleSpeedThreshold() const {
	return mSpace->idleSpeedThreshold;
}

void cSpace::IdleSpeedThreshold( const cpFloat& idleSpeedThreshold ) {
	mSpace->idleSpeedThreshold = idleSpeedThreshold;
}

const cpFloat& cSpace::SleepTimeThreshold() const {
	return mSpace->sleepTimeThreshold;
}

void cSpace::SleepTimeThreshold( const cpFloat& sleepTimeThreshold ) {
	mSpace->sleepTimeThreshold = sleepTimeThreshold;
}

cBody * cSpace::StaticBody() const {
	return mStaticBody;
}

cShape * cSpace::AddShape( cShape * shape ) {
	cpSpaceAddShape( mSpace, shape->Shape() );
	mShapes.push_back( shape );
	return shape;
}

cShape * cSpace::AddStaticShape( cShape * shape ) {
	cpSpaceAddStaticShape( mSpace, shape->Shape() );
	mShapes.push_back( shape );
	return shape;
}

cBody * cSpace::AddBody( cBody * body ) {
	cpSpaceAddBody( mSpace, body->Body() );
	mBodys.push_back( body );
	return body;
}

cConstraint * cSpace::AddConstraint( cConstraint * constraint ) {
	cpSpaceAddConstraint( mSpace, constraint->Constraint() );
	mConstraints.push_back( constraint );
	return constraint;
}

void cSpace::RemoveShape( cShape * shape ) {
	cpSpaceRemoveShape( mSpace, shape->Shape() );
	mShapes.remove( shape );
}

void cSpace::RemoveStaticShape( cShape * shape ) {
	cpSpaceRemoveStaticShape( mSpace, shape->Shape() );
	mShapes.remove( shape );
}

void cSpace::RemoveBody( cBody * body ) {
	cpSpaceRemoveBody( mSpace, body->Body() );
	mBodys.remove( body );
}

void cSpace::RemoveConstraint( cConstraint * constraint ) {
	cpSpaceRemoveConstraint( mSpace, constraint->Constraint() );
	mConstraints.remove( constraint );
}

cpSpace * cSpace::Space() const {
	return mSpace;
}

void cSpace::ActivateShapesTouchingShape( cShape * shape ) {
	cpSpaceActivateShapesTouchingShape( mSpace, shape->Shape() );
}

void cSpace::RehashShape( cShape * shape ) {
	cpSpaceRehashShape( mSpace, shape->Shape() );
}

static void drawObject( cpShape * shape, cpSpace * space ) {
	reinterpret_cast<cShape*> ( shape->data )->Draw( reinterpret_cast<cSpace*>( space->data ) );
}

static void drawBB( cpShape *shape, void * unused ) {
	reinterpret_cast<cShape*> ( shape->data )->DrawBB();
}

static void drawConstraint( cpConstraint *constraint ) {
	reinterpret_cast<cConstraint*> ( constraint->data )->Draw();
}

void cSpace::Draw() {
	cPhysicsManager::cDrawSpaceOptions * options = cPhysicsManager::instance()->GetDrawOptions();

	if( options->DrawHash ) {
		glColor3f(0.5, 0.5, 0.5);
		//cpBBTreeRenderDebug( mSpace->staticShapes );

		glColor3f(0, 1, 0);
		//cpBBTreeRenderDebug( mSpace->activeShapes );
	}

	glLineWidth( options->LineThickness );

	if( options->DrawShapes ) {
		cpSpatialIndexEach( mSpace->activeShapes, (cpSpatialIndexIterator)drawObject, mSpace );
		cpSpatialIndexEach( mSpace->staticShapes, (cpSpatialIndexIterator)drawObject, mSpace );
	}

	glLineWidth( 1.0f );

	if( options->DrawBBs ){
		glColor3f( 0.3f, 0.5f, 0.3f );

		cpSpatialIndexEach(mSpace->activeShapes, (cpSpatialIndexIterator)drawBB, NULL);
		cpSpatialIndexEach(mSpace->staticShapes, (cpSpatialIndexIterator)drawBB, NULL);
	}

	cpArray * constraints = mSpace->constraints;

	glColor3f(0.5f, 1.0f, 0.5f);

	for( int i=0, count = constraints->num; i<count; i++ ) {
		drawConstraint( (cpConstraint *)constraints->arr[i] );
	}

	if( options->BodyPointSize ){
		glPointSize( options->BodyPointSize );

		glBegin(GL_POINTS);

		glColor3f(LINE_COLOR);

		cpArray * bodies = mSpace->bodies;

		for( int i=0, count = bodies->num; i<count; i++ ) {
			cpBody * body = (cpBody *)bodies->arr[i];

			glVertex2f( body->p.x, body->p.y );
		}

		 glEnd();
	}

	if( options->CollisionPointSize ){
		glPointSize( options->CollisionPointSize );

		glBegin( GL_POINTS );

		cpArray * arbiters = mSpace->arbiters;

		for( int i = 0; i < arbiters->num; i++ ){
			cpArbiter *arb = (cpArbiter*)arbiters->arr[i];

			glColor3f( COLLISION_COLOR );

			for(int i=0; i<arb->numContacts; i++){
				cpVect v = arb->contacts[i].p;
				glVertex2f( v.x, v.y );
			}
		}

		glEnd();
	}
}

}}
