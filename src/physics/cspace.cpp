#include "cspace.hpp"
#include "cphysicsmanager.hpp"

namespace EE { namespace Physics {

cSpace * cSpace::New() {
	return eeNew( cSpace, () );
}

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

void cSpace::Step( const cpFloat& dt ) {
	cpSpaceStep( mSpace, dt );
}

void cSpace::Update() {
	Step( cEngine::instance()->Elapsed() / 1000 );
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

cVect cSpace::Gravity() const {
	return tovect( mSpace->gravity );
}

void cSpace::Gravity( const cVect& gravity ) {
	mSpace->gravity = tocpv( gravity );
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

cShape * cSpace::PointQueryFirst( cVect point, cpLayers layers, cpGroup group ) {
	cpShape * shape = cpSpacePointQueryFirst( mSpace, tocpv( point ), layers, group );

	if ( NULL != shape ) {
		return reinterpret_cast<cShape*> ( shape->data );
	}

	return NULL;
}

cShape * cSpace::SegmentQueryFirst( cVect start, cVect end, cpLayers layers, cpGroup group, cpSegmentQueryInfo * out ) {
	cpShape * shape = cpSpaceSegmentQueryFirst( mSpace, tocpv( start ), tocpv( end ), layers, group, out );

	if ( NULL != shape ) {
		return reinterpret_cast<cShape*> ( shape->data );
	}

	return NULL;
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
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();
	cPhysicsManager::cDrawSpaceOptions * options = cPhysicsManager::instance()->GetDrawOptions();

	cpFloat lw = BR->GetLineWidth();
	cpFloat ps = BR->GetPointSize();

	if( options->DrawHash ) {
		//glColor3f(0.5, 0.5, 0.5);
		//cpBBTreeRenderDebug( mSpace->staticShapes );

		//glColor3f(0, 1, 0);
		//cpBBTreeRenderDebug( mSpace->activeShapes );
	}

	BR->SetLineWidth( options->LineThickness );

	if( options->DrawShapes ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIterator)drawObject, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIterator)drawObject, mSpace );
	}

	BR->SetLineWidth( lw );

	if( options->DrawBBs ){
		cpSpatialIndexEach(mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIterator)drawBB, NULL);
		cpSpatialIndexEach(mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIterator)drawBB, NULL);
	}

	cpArray * constraints = mSpace->CP_PRIVATE(constraints);

	for( int i=0, count = constraints->num; i<count; i++ ) {
		drawConstraint( (cpConstraint *)constraints->arr[i] );
	}

	if( options->BodyPointSize ) {
		BR->SetPointSize( options->BodyPointSize );
		BR->PointsBegin();
		BR->PointSetColor( eeColorA( 255, 255, 255, 255 ) );

		cpArray * bodies = mSpace->CP_PRIVATE(bodies);

		for( int i=0, count = bodies->num; i<count; i++ ) {
			cpBody * body = (cpBody *)bodies->arr[i];

			BR->BatchPoint( body->p.x, body->p.y );
		}

		BR->Draw();
	}

	if ( options->CollisionPointSize ) {
		BR->SetPointSize( options->CollisionPointSize );
		BR->PointsBegin();
		BR->PointSetColor( eeColorA( 255, 0, 0, 255 ) );

		cpArray * arbiters = mSpace->CP_PRIVATE(arbiters);

		for( int i = 0; i < arbiters->num; i++ ){
			cpArbiter *arb = (cpArbiter*)arbiters->arr[i];

			for( int i=0; i< arb->CP_PRIVATE(numContacts); i++ ){
				cVect v = tovect( arb->CP_PRIVATE(contacts)[i].CP_PRIVATE(p) );
				BR->BatchPoint( v.x, v.y );
			}
		}

		BR->Draw();
	}

	BR->SetLineWidth( lw );
	BR->SetPointSize( ps );
}

/** Collision Handling */

static cpBool RecieverCollisionBeginFunc( cpArbiter * arb, cpSpace * space, void * data ) {
	cSpace * tspace = reinterpret_cast<cSpace*>( space->data );
	cArbiter tarb( arb );

	return tspace->OnCollisionBegin( &tarb, data );
}

static cpBool RecieverCollisionPreSolveFunc( cpArbiter * arb, cpSpace * space, void * data ) {
	cSpace * tspace = reinterpret_cast<cSpace*>( space->data );
	cArbiter tarb( arb );

	return tspace->OnCollisionPreSolve( &tarb, data );
}

static void RecieverCollisionPostSolve( cpArbiter * arb, cpSpace * space, void * data ) {
	cSpace * tspace = reinterpret_cast<cSpace*>( space->data );
	cArbiter tarb( arb );

	tspace->OnCollisionPostSolve( &tarb, data );
}

static void RecieverCollisionSeparateFunc( cpArbiter * arb, cpSpace * space, void * data ) {
	cSpace * tspace = reinterpret_cast<cSpace*>( space->data );
	cArbiter tarb( arb );

	tspace->OnCollisionSeparate( &tarb, data );
}

static void RecieverPostStepCallback( cpSpace * space, void * obj, void * data ) {
	cSpace * tspace = reinterpret_cast<cSpace*>( space->data );

	tspace->OnPostStepCallback( obj, data );
}

static void RecieverBBQueryFunc( cpShape * shape, void * data ) {
	cSpace::cBBQuery * query = reinterpret_cast<cSpace::cBBQuery*>( data );

	query->Space->OnBBQuery( reinterpret_cast<cShape*>( shape->data ), query );
}

static void RecieverSegmentQueryFunc( cpShape *shape, cpFloat t, cpVect n, void * data ) {
	cSpace::cSegmentQuery * query = reinterpret_cast<cSpace::cSegmentQuery*>( data );

	query->Space->OnSegmentQuery( reinterpret_cast<cShape*>( shape->data ), t, tovect( n ), query );
}

static void RecieverPointQueryFunc( cpShape * shape, void * data ) {
	cSpace::cPointQuery * query = reinterpret_cast<cSpace::cPointQuery*>( data );

	query->Space->OnPointQuery( reinterpret_cast<cShape*>( shape->data ), query );
}

cpBool cSpace::OnCollisionBegin( cArbiter * arb, void * data ) {
	std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( arb->Arbiter()->CP_PRIVATE(contacts)->CP_PRIVATE(hash) );
	cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.begin.IsSet() )
		return handler.begin( arb, this, data );
	else if ( mCollisionsDefault.begin.IsSet() )
		return mCollisionsDefault.begin( arb, this, data );

	return 1;
}

cpBool cSpace::OnCollisionPreSolve( cArbiter * arb, void * data ) {
	std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( arb->Arbiter()->CP_PRIVATE(contacts)->CP_PRIVATE(hash) );
	cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.preSolve.IsSet() )
		return handler.preSolve( arb, this, data );
	else if ( mCollisionsDefault.preSolve.IsSet() )
		return mCollisionsDefault.preSolve( arb, this, data );

	return 1;
}

void cSpace::OnCollisionPostSolve( cArbiter * arb, void * data ) {
	std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( arb->Arbiter()->CP_PRIVATE(contacts)->CP_PRIVATE(hash) );
	cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.postSolve.IsSet() )
		handler.postSolve( arb, this, data );
	else if ( mCollisionsDefault.begin.IsSet() )
		mCollisionsDefault.postSolve( arb, this, data );
}

void cSpace::OnCollisionSeparate( cArbiter * arb, void * data ) {
	std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( arb->Arbiter()->CP_PRIVATE(contacts)->CP_PRIVATE(hash) );
	cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.separate.IsSet() )
		handler.separate( arb, this, data );
	else if ( mCollisionsDefault.begin.IsSet() )
		mCollisionsDefault.separate( arb, this, data );
}

void cSpace::OnPostStepCallback( void * obj, void * data ) {
	cPostStepCallback * Cb = reinterpret_cast<cPostStepCallback *> ( data );

	if ( Cb->Callback.IsSet() )
		Cb->Callback( this, obj, Cb->Data );

	mPostStepCallbacks.remove( Cb );
	eeSAFE_DELETE( Cb );
}

void cSpace::OnBBQuery( cShape * shape, cBBQuery * query ) {
	if ( query->Func.IsSet() )
		query->Func( shape, query->Data );
}

void cSpace::OnSegmentQuery( cShape * shape, cpFloat t, cVect n , cSegmentQuery * query ) {
	if ( query->Func.IsSet() )
		query->Func( shape, t, n, query->Data );
}

void cSpace::OnPointQuery( cShape * shape, cPointQuery * query ) {
	if ( query->Func.IsSet() )
		query->Func( shape, query->Data );
}

void cSpace::AddCollisionHandler( cpCollisionType a, cpCollisionType b, CollisionBeginFunc begin, CollisionPreSolveFunc preSolve, CollisionPostSolveFunc postSolve, CollisionSeparateFunc separate, void * data ) {
	cpSpaceAddCollisionHandler( mSpace, a, b, &RecieverCollisionBeginFunc, &RecieverCollisionPreSolveFunc, &RecieverCollisionPostSolve, &RecieverCollisionSeparateFunc, data );

	cCollisionHandler handler;
	handler.a			= a;
	handler.b			= b;
	handler.begin		= begin;
	handler.preSolve	= preSolve;
	handler.postSolve	= postSolve;
	handler.separate	= separate;
	handler.data		= data;

	mCollisions.erase( CP_HASH_PAIR( a, b ) );
	mCollisions[ CP_HASH_PAIR( a, b ) ] = handler;
}

void cSpace::RemoveCollisionHandler( cpCollisionType a, cpCollisionType b ) {
	cpSpaceRemoveCollisionHandler( mSpace, a, b );

	mCollisions.erase( CP_HASH_PAIR( a, b ) );
}

void cSpace::SetDefaultCollisionHandler( CollisionBeginFunc begin, CollisionPreSolveFunc preSolve, CollisionPostSolveFunc postSolve, CollisionSeparateFunc separate, void * data ) {
	cpSpaceSetDefaultCollisionHandler( mSpace, &RecieverCollisionBeginFunc, &RecieverCollisionPreSolveFunc, &RecieverCollisionPostSolve, &RecieverCollisionSeparateFunc, data );

	mCollisionsDefault.begin		= begin;
	mCollisionsDefault.preSolve		= preSolve;
	mCollisionsDefault.postSolve	= postSolve;
	mCollisionsDefault.separate		= separate;
	mCollisionsDefault.data			= data;
}

void cSpace::AddPostStepCallback( PostStepCallback postStep, void * obj, void * data ) {
	cPostStepCallback * PostStepCb	= eeNew( cPostStepCallback, () );
	PostStepCb->Callback			= postStep,
	PostStepCb->Data				= data;

	cpSpaceAddPostStepCallback( mSpace, &RecieverPostStepCallback, obj, PostStepCb );
	mPostStepCallbacks.push_back( PostStepCb );	
}

void cSpace::BBQuery( cpBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void * data ) {
	cBBQuery tBBQuery;
	tBBQuery.Space	= this;
	tBBQuery.Data	= data;
	tBBQuery.Func	= func;

	cpSpaceBBQuery( mSpace, bb, layers, group, &RecieverBBQueryFunc, reinterpret_cast<void*>( &tBBQuery ) );
}

void cSpace::SegmentQuery( cVect start, cVect end, cpLayers layers, cpGroup group, SegmentQueryFunc func, void * data ) {
	cSegmentQuery tSegmentQuery;

	tSegmentQuery.Space	= this;
	tSegmentQuery.Data	= data;
	tSegmentQuery.Func	= func;

	cpSpaceSegmentQuery( mSpace, tocpv( start ), tocpv( end ), layers, group, &RecieverSegmentQueryFunc, reinterpret_cast<void*>( &tSegmentQuery ) );
}

void cSpace::PointQuery( cVect point, cpLayers layers, cpGroup group, PointQueryFunc func, void * data ) {
	cPointQuery tPointQuery;
	tPointQuery.Space	= this;
	tPointQuery.Data	= data;
	tPointQuery.Func	= func;

	cpSpacePointQuery( mSpace, tocpv( point ), layers, group, &RecieverPointQueryFunc, reinterpret_cast<void*>( &tPointQuery ) );
}

}}
