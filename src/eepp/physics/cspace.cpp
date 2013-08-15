#include <eepp/physics/cspace.hpp>
#include <eepp/physics/cphysicsmanager.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/window/cengine.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

cSpace * cSpace::New() {
	return cpNew( cSpace, () );
}

void cSpace::Free( cSpace * space ) {
	cpSAFE_DELETE( space );
}

cSpace::cSpace() :
	mData( NULL )
{
	mSpace = cpSpaceNew();
	mSpace->data = (void*)this;
	mStaticBody = cpNew( cBody, ( mSpace->staticBody ) );

	cPhysicsManager::instance()->RemoveBodyFree( mStaticBody );
	cPhysicsManager::instance()->AddSpace( this );
}

cSpace::~cSpace() {	
	cpSpaceFree( mSpace );

	std::list<cConstraint*>::iterator itc = mConstraints.begin();
	for ( ; itc != mConstraints.end(); itc++ )
		cpSAFE_DELETE( *itc );

	std::list<cShape*>::iterator its = mShapes.begin();
	for ( ; its != mShapes.end(); its++ )
		cpSAFE_DELETE( *its );

	std::list<cBody*>::iterator itb = mBodys.begin();
	for ( ; itb != mBodys.end(); itb++ )
		cpSAFE_DELETE( *itb );

	mStaticBody->mBody = NULL; // The body has been released by cpSpaceFree( mSpace )

	cpSAFE_DELETE( mStaticBody );

	cPhysicsManager::instance()->RemoveSpace( this );
}

void cSpace::Data( void * data ) {
	mData = data;
}

void * cSpace::Data() const {
	return mData;
}

void cSpace::Step( const cpFloat& dt ) {
	cpSpaceStep( mSpace, dt );
}

void cSpace::Update() {
	#ifdef PHYSICS_RENDERER_ENABLED
	Step( Window::cEngine::instance()->Elapsed().AsSeconds() );
	#else
	Step( 1 / 60 );
	#endif
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

void cSpace::CollisionSlop( cpFloat slop ) {
	mSpace->collisionSlop = slop;
}

cpFloat cSpace::CollisionSlop() const {
	return mSpace->collisionSlop;
}

void cSpace::CollisionBias( cpFloat bias ) {
	mSpace->collisionBias = bias;
}

cpFloat cSpace::CollisionBias() const {
	return mSpace->collisionBias;
}

cpTimestamp cSpace::CollisionPersistence() {
	return cpSpaceGetCollisionPersistence( mSpace );
}

void cSpace::CollisionPersistence( cpTimestamp value ) {
	cpSpaceSetCollisionPersistence( mSpace, value );
}

bool cSpace::EnableContactGraph() {
	return cpTrue == cpSpaceGetEnableContactGraph( mSpace );
}

void cSpace::EnableContactGraph( bool value ) {
	cpSpaceSetEnableContactGraph( mSpace, value );
}

cBody * cSpace::StaticBody() const {
	return mStaticBody;
}

bool cSpace::Contains( cShape * shape ) {
	return cpTrue == cpSpaceContainsShape( mSpace, shape->Shape() );
}

bool cSpace::Contains( cBody * body ) {
	return cpTrue == cpSpaceContainsBody( mSpace, body->Body() );
}

bool cSpace::Contains( cConstraint * constraint ) {
	return cpTrue == cpSpaceContainsConstraint( mSpace, constraint->Constraint() );
}

cShape * cSpace::AddShape( cShape * shape ) {
	cpSpaceAddShape( mSpace, shape->Shape() );

	mShapes.push_back( shape );

	cPhysicsManager::instance()->RemoveShapeFree( shape );

	return shape;
}

cShape * cSpace::AddStaticShape( cShape * shape ) {
	cpSpaceAddStaticShape( mSpace, shape->Shape() );

	mShapes.push_back( shape );

	cPhysicsManager::instance()->RemoveShapeFree( shape );

	return shape;
}

cBody * cSpace::AddBody( cBody * body ) {
	cpSpaceAddBody( mSpace, body->Body() );

	mBodys.push_back( body );

	cPhysicsManager::instance()->RemoveBodyFree( body );

	return body;
}

cConstraint * cSpace::AddConstraint( cConstraint * constraint ) {
	cpSpaceAddConstraint( mSpace, constraint->Constraint() );

	mConstraints.push_back( constraint );

	cPhysicsManager::instance()->RemoveConstraintFree( constraint );

	return constraint;
}

void cSpace::RemoveShape( cShape * shape ) {
	if ( NULL != shape ) {
		cpSpaceRemoveShape( mSpace, shape->Shape() );

		mShapes.remove( shape );

		cPhysicsManager::instance()->AddShapeFree( shape );
	}
}

void cSpace::RemoveStaticShape( cShape * shape ) {
	if ( NULL != shape ) {
		cpSpaceRemoveStaticShape( mSpace, shape->Shape() );

		mShapes.remove( shape );

		cPhysicsManager::instance()->AddShapeFree( shape );
	}
}

void cSpace::RemoveBody( cBody * body ) {
	if ( NULL != body ) {
		cpSpaceRemoveBody( mSpace, body->Body() );

		mBodys.remove( body );

		cPhysicsManager::instance()->RemoveBodyFree( body );
	}
}

void cSpace::RemoveConstraint( cConstraint * constraint ) {
	if ( NULL != constraint ) {
		cpSpaceRemoveConstraint( mSpace, constraint->Constraint() );

		mConstraints.remove( constraint );

		cPhysicsManager::instance()->AddConstraintFree( constraint );
	}
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

#ifdef PHYSICS_RENDERER_ENABLED
static void drawObject( cpShape * shape, cpSpace * space ) {
	reinterpret_cast<cShape*> ( shape->data )->Draw( reinterpret_cast<cSpace*>( space->data ) );
}

static void drawObjectBorder( cpShape * shape, cpSpace * space ) {
	reinterpret_cast<cShape*> ( shape->data )->DrawBorder( reinterpret_cast<cSpace*>( space->data ) );
}

static void drawBB( cpShape *shape, void * unused ) {
	reinterpret_cast<cShape*> ( shape->data )->DrawBB();
}

static void drawConstraint( cpConstraint *constraint ) {
	reinterpret_cast<cConstraint*> ( constraint->data )->Draw();
}
#endif

void cSpace::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED

	cBatchRenderer * BR = cGlobalBatchRenderer::instance();
	BR->SetBlendMode( ALPHA_NORMAL );

	cPhysicsManager::cDrawSpaceOptions * options = cPhysicsManager::instance()->GetDrawOptions();

	cpFloat lw = BR->GetLineWidth();
	cpFloat ps = BR->GetPointSize();

	BR->SetLineWidth( options->LineThickness );

	if ( options->DrawShapes ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIteratorFunc)drawObject, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIteratorFunc)drawObject, mSpace );
	}

	if ( options->DrawShapesBorders ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIteratorFunc)drawObjectBorder, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIteratorFunc)drawObjectBorder, mSpace );
	}

	BR->SetLineWidth( lw );

	if ( options->DrawBBs ){
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIteratorFunc)drawBB, NULL );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIteratorFunc)drawBB, NULL );
		BR->Draw();
	}

	cpArray * constraints = mSpace->CP_PRIVATE(constraints);

	for ( int i = 0, count = constraints->num; i < count; i++ ) {
		drawConstraint( (cpConstraint *)constraints->arr[i] );
	}

	if ( options->BodyPointSize ) {
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

	#endif
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
	cpHashValue hash = (cpHashValue)data;

	//if ( NULL != data ) {
		std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( hash );
		cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

		if ( it != mCollisions.end() && handler.begin.IsSet() ) {
			return handler.begin( arb, this, handler.data );
		}
	//}

	if ( mCollisionsDefault.begin.IsSet() ) {
		return mCollisionsDefault.begin( arb, this, mCollisionsDefault.data );
	}

	return 1;
}

cpBool cSpace::OnCollisionPreSolve( cArbiter * arb, void * data ) {
	cpHashValue hash = (cpHashValue)data;

	//if ( NULL != data ) {
		std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( hash );
		cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

		if ( it != mCollisions.end() && handler.preSolve.IsSet() ) {
			return handler.preSolve( arb, this, handler.data );
		}
	//}

	if ( mCollisionsDefault.preSolve.IsSet() ) {
		return mCollisionsDefault.preSolve( arb, this, mCollisionsDefault.data );
	}

	return 1;
}

void cSpace::OnCollisionPostSolve( cArbiter * arb, void * data ) {
	cpHashValue hash = (cpHashValue)data;

	//if ( NULL != data ) {
		std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( hash );
		cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

		if ( it != mCollisions.end() && handler.postSolve.IsSet() ) {
			handler.postSolve( arb, this, handler.data );
			return;
		}
	//}

	if ( mCollisionsDefault.begin.IsSet() ) {
		mCollisionsDefault.postSolve( arb, this, mCollisionsDefault.data );
	}
}

void cSpace::OnCollisionSeparate( cArbiter * arb, void * data ) {
	cpHashValue hash = (cpHashValue)data;

	//if ( NULL != data ) {
		std::map< cpHashValue, cCollisionHandler >::iterator it = mCollisions.find( hash );
		cCollisionHandler handler = static_cast<cCollisionHandler>( it->second );

		if ( it != mCollisions.end() && handler.separate.IsSet() ) {
			handler.separate( arb, this, handler.data );
			return;
		}
	//}

	if ( mCollisionsDefault.begin.IsSet() ) {
		mCollisionsDefault.separate( arb, this, mCollisionsDefault.data );
	}
}

void cSpace::OnPostStepCallback( void * obj, void * data ) {
	cPostStepCallback * Cb = reinterpret_cast<cPostStepCallback *> ( data );

	if ( Cb->Callback.IsSet() ) {
		Cb->Callback( this, obj, Cb->Data );
	}

	mPostStepCallbacks.remove( Cb );
	cpSAFE_DELETE( Cb );
}

void cSpace::OnBBQuery( cShape * shape, cBBQuery * query ) {
	if ( query->Func.IsSet() ) {
		query->Func( shape, query->Data );
	}
}

void cSpace::OnSegmentQuery( cShape * shape, cpFloat t, cVect n , cSegmentQuery * query ) {
	if ( query->Func.IsSet() ) {
		query->Func( shape, t, n, query->Data );
	}
}

void cSpace::OnPointQuery( cShape * shape, cPointQuery * query ) {
	if ( query->Func.IsSet() ) {
		query->Func( shape, query->Data );
	}
}

void cSpace::AddCollisionHandler( const cCollisionHandler& handler ) {
	cpHashValue hash = CP_HASH_PAIR( handler.a, handler.b );

	cpCollisionBeginFunc		f1 = ( handler.begin.IsSet() )		?	&RecieverCollisionBeginFunc		: NULL;
	cpCollisionPreSolveFunc		f2 = ( handler.preSolve.IsSet() )	?	&RecieverCollisionPreSolveFunc	: NULL;
	cpCollisionPostSolveFunc	f3 = ( handler.postSolve.IsSet() )	?	&RecieverCollisionPostSolve		: NULL;
	cpCollisionSeparateFunc		f4 = ( handler.separate.IsSet() )	?	&RecieverCollisionSeparateFunc	: NULL;

	cpSpaceAddCollisionHandler( mSpace, handler.a, handler.b, f1, f2, f3, f4, (void*)hash );

	mCollisions.erase( hash );
	mCollisions[ hash ] = handler;
}

void cSpace::RemoveCollisionHandler( cpCollisionType a, cpCollisionType b ) {
	cpSpaceRemoveCollisionHandler( mSpace, a, b );

	mCollisions.erase( CP_HASH_PAIR( a, b ) );
}

void cSpace::SetDefaultCollisionHandler( const cCollisionHandler& handler ) {
	cpCollisionBeginFunc		f1 = ( handler.begin.IsSet() )		?	&RecieverCollisionBeginFunc		: NULL;
	cpCollisionPreSolveFunc		f2 = ( handler.preSolve.IsSet() )	?	&RecieverCollisionPreSolveFunc	: NULL;
	cpCollisionPostSolveFunc	f3 = ( handler.postSolve.IsSet() )	?	&RecieverCollisionPostSolve		: NULL;
	cpCollisionSeparateFunc		f4 = ( handler.separate.IsSet() )	?	&RecieverCollisionSeparateFunc	: NULL;

	cpSpaceSetDefaultCollisionHandler( mSpace, f1, f2, f3, f4, NULL );

	mCollisionsDefault	= handler;
}

void cSpace::AddPostStepCallback( PostStepCallback postStep, void * obj, void * data ) {
	cPostStepCallback * PostStepCb	= cpNew( cPostStepCallback, () );
	PostStepCb->Callback			= postStep,
	PostStepCb->Data				= data;

	cpSpaceAddPostStepCallback( mSpace, &RecieverPostStepCallback, obj, PostStepCb );
	mPostStepCallbacks.push_back( PostStepCb );	
}

void cSpace::BBQuery( cBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void * data ) {
	cBBQuery tBBQuery;
	tBBQuery.Space	= this;
	tBBQuery.Data	= data;
	tBBQuery.Func	= func;

	cpSpaceBBQuery( mSpace, tocpbb( bb ), layers, group, &RecieverBBQueryFunc, reinterpret_cast<void*>( &tBBQuery ) );
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

void cSpace::ReindexShape( cShape * shape ) {
	cpSpaceReindexShape( mSpace, shape->Shape() );
}

void cSpace::ReindexShapesForBody( cBody *body ) {
	cpSpaceReindexShapesForBody( mSpace, body->Body() );
}

void cSpace::ReindexStatic() {
	cpSpaceReindexStatic( mSpace );
}

void cSpace::UseSpatialHash( cpFloat dim, int count ) {
	cpSpaceUseSpatialHash( mSpace, dim, count );
}

static void SpaceBodyIteratorFunc( cpBody * body, void *data ) {
	cSpace::cBodyIterator * it = reinterpret_cast<cSpace::cBodyIterator *> ( data );
	it->Space->OnEachBody( reinterpret_cast<cBody*>( body->data ), it );
}

void cSpace::EachBody( BodyIteratorFunc Func, void * data ) {
	cBodyIterator it( this, data, Func );
	cpSpaceEachBody( mSpace, &SpaceBodyIteratorFunc, (void*)&it );
}

void cSpace::OnEachBody( cBody * Body, cBodyIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( it->Space, Body, it->Data );
	}
}

static void SpaceShapeIteratorFunc ( cpShape * shape, void * data ) {
	cSpace::cShapeIterator * it = reinterpret_cast<cSpace::cShapeIterator *> ( data );
	it->Space->OnEachShape( reinterpret_cast<cShape*>( shape->data ), it );
}

void cSpace::EachShape( ShapeIteratorFunc Func, void * data ) {
	cShapeIterator it( this, data, Func );
	cpSpaceEachShape( mSpace, &SpaceShapeIteratorFunc, (void*)&it );
}

void cSpace::OnEachShape( cShape * Shape, cShapeIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( it->Space, Shape, it->Data );
	}
}

void cSpace::ConvertBodyToDynamic( cBody * body, cpFloat mass, cpFloat moment ) {
	cpSpaceConvertBodyToDynamic( mSpace, body->Body(), mass, moment );
}

void cSpace::ConvertBodyToStatic(cBody * body ) {
	cpSpaceConvertBodyToStatic( mSpace, body->Body() );
}

CP_NAMESPACE_END
