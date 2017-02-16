#include <eepp/physics/space.hpp>
#include <eepp/physics/physicsmanager.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/window/engine.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

Space * Space::New() {
	return cpNew( Space, () );
}

void Space::Free( Space * space ) {
	cpSAFE_DELETE( space );
}

Space::Space() :
	mData( NULL )
{
	mSpace = cpSpaceNew();
	mSpace->data = (void*)this;
	mStatiBody = cpNew( Body, ( mSpace->staticBody ) );

	PhysicsManager::instance()->RemoveBodyFree( mStatiBody );
	PhysicsManager::instance()->AddSpace( this );
}

Space::~Space() {	
	cpSpaceFree( mSpace );

	std::list<Constraint*>::iterator itc = mConstraints.begin();
	for ( ; itc != mConstraints.end(); itc++ )
		cpSAFE_DELETE( *itc );

	std::list<Shape*>::iterator its = mShapes.begin();
	for ( ; its != mShapes.end(); its++ )
		cpSAFE_DELETE( *its );

	std::list<Body*>::iterator itb = mBodys.begin();
	for ( ; itb != mBodys.end(); itb++ )
		cpSAFE_DELETE( *itb );

	mStatiBody->mBody = NULL; // The body has been released by cpSpaceFree( mSpace )

	cpSAFE_DELETE( mStatiBody );

	PhysicsManager::instance()->RemoveSpace( this );
}

void Space::Data( void * data ) {
	mData = data;
}

void * Space::Data() const {
	return mData;
}

void Space::Step( const cpFloat& dt ) {
	cpSpaceStep( mSpace, dt );
}

void Space::Update() {
	#ifdef PHYSICS_RENDERER_ENABLED
	Step( Window::Engine::instance()->elapsed().asSeconds() );
	#else
	Step( 1 / 60 );
	#endif
}

const int& Space::Iterations() const {
	return mSpace->iterations;
}

void Space::Iterations( const int& iterations ) {
	mSpace->iterations = iterations;
}

cVect Space::Gravity() const {
	return tovect( mSpace->gravity );
}

void Space::Gravity( const cVect& gravity ) {
	mSpace->gravity = tocpv( gravity );
}

const cpFloat& Space::Damping() const {
	return mSpace->damping;
}

void Space::Damping( const cpFloat& damping ) {
	mSpace->damping = damping;
}

const cpFloat& Space::IdleSpeedThreshold() const {
	return mSpace->idleSpeedThreshold;
}

void Space::IdleSpeedThreshold( const cpFloat& idleSpeedThreshold ) {
	mSpace->idleSpeedThreshold = idleSpeedThreshold;
}

const cpFloat& Space::SleepTimeThreshold() const {
	return mSpace->sleepTimeThreshold;
}

void Space::SleepTimeThreshold( const cpFloat& sleepTimeThreshold ) {
	mSpace->sleepTimeThreshold = sleepTimeThreshold;
}

void Space::CollisionSlop( cpFloat slop ) {
	mSpace->collisionSlop = slop;
}

cpFloat Space::CollisionSlop() const {
	return mSpace->collisionSlop;
}

void Space::CollisionBias( cpFloat bias ) {
	mSpace->collisionBias = bias;
}

cpFloat Space::CollisionBias() const {
	return mSpace->collisionBias;
}

cpTimestamp Space::CollisionPersistence() {
	return cpSpaceGetCollisionPersistence( mSpace );
}

void Space::CollisionPersistence( cpTimestamp value ) {
	cpSpaceSetCollisionPersistence( mSpace, value );
}

bool Space::EnableContactGraph() {
	return cpTrue == cpSpaceGetEnableContactGraph( mSpace );
}

void Space::EnableContactGraph( bool value ) {
	cpSpaceSetEnableContactGraph( mSpace, value );
}

Body * Space::StatiBody() const {
	return mStatiBody;
}

bool Space::Contains( Shape * shape ) {
	return cpTrue == cpSpaceContainsShape( mSpace, shape->GetShape() );
}

bool Space::Contains( Body * body ) {
	return cpTrue == cpSpaceContainsBody( mSpace, body->GetBody() );
}

bool Space::Contains( Constraint * constraint ) {
	return cpTrue == cpSpaceContainsConstraint( mSpace, constraint->GetConstraint() );
}

Shape * Space::AddShape( Shape * shape ) {
	cpSpaceAddShape( mSpace, shape->GetShape() );

	mShapes.push_back( shape );

	PhysicsManager::instance()->RemoveShapeFree( shape );

	return shape;
}

Shape * Space::AddStatiShape( Shape * shape ) {
	cpSpaceAddStaticShape( mSpace, shape->GetShape() );

	mShapes.push_back( shape );

	PhysicsManager::instance()->RemoveShapeFree( shape );

	return shape;
}

Body * Space::AddBody( Body * body ) {
	cpSpaceAddBody( mSpace, body->GetBody() );

	mBodys.push_back( body );

	PhysicsManager::instance()->RemoveBodyFree( body );

	return body;
}

Constraint * Space::AddConstraint( Constraint * constraint ) {
	cpSpaceAddConstraint( mSpace, constraint->GetConstraint() );

	mConstraints.push_back( constraint );

	PhysicsManager::instance()->RemoveConstraintFree( constraint );

	return constraint;
}

void Space::RemoveShape( Shape * shape ) {
	if ( NULL != shape ) {
		cpSpaceRemoveShape( mSpace, shape->GetShape() );

		mShapes.remove( shape );

		PhysicsManager::instance()->AddShapeFree( shape );
	}
}

void Space::RemoveStatiShape( Shape * shape ) {
	if ( NULL != shape ) {
		cpSpaceRemoveStaticShape( mSpace, shape->GetShape() );

		mShapes.remove( shape );

		PhysicsManager::instance()->AddShapeFree( shape );
	}
}

void Space::RemoveBody( Body * body ) {
	if ( NULL != body ) {
		cpSpaceRemoveBody( mSpace, body->GetBody() );

		mBodys.remove( body );

		PhysicsManager::instance()->RemoveBodyFree( body );
	}
}

void Space::RemoveConstraint( Constraint * constraint ) {
	if ( NULL != constraint ) {
		cpSpaceRemoveConstraint( mSpace, constraint->GetConstraint() );

		mConstraints.remove( constraint );

		PhysicsManager::instance()->AddConstraintFree( constraint );
	}
}

Shape * Space::PointQueryFirst( cVect point, cpLayers layers, cpGroup group ) {
	cpShape * shape = cpSpacePointQueryFirst( mSpace, tocpv( point ), layers, group );

	if ( NULL != shape ) {
		return reinterpret_cast<Shape*> ( shape->data );
	}

	return NULL;
}

Shape * Space::SegmentQueryFirst( cVect start, cVect end, cpLayers layers, cpGroup group, cpSegmentQueryInfo * out ) {
	cpShape * shape = cpSpaceSegmentQueryFirst( mSpace, tocpv( start ), tocpv( end ), layers, group, out );

	if ( NULL != shape ) {
		return reinterpret_cast<Shape*> ( shape->data );
	}

	return NULL;
}

cpSpace * Space::GetSpace() const {
	return mSpace;
}

void Space::ActivateShapesTouchingShape( Shape * shape ) {
	cpSpaceActivateShapesTouchingShape( mSpace, shape->GetShape() );
}

#ifdef PHYSICS_RENDERER_ENABLED
static void drawObject( cpShape * shape, cpSpace * space ) {
	reinterpret_cast<Shape*> ( shape->data )->Draw( reinterpret_cast<Space*>( space->data ) );
}

static void drawObjectBorder( cpShape * shape, cpSpace * space ) {
	reinterpret_cast<Shape*> ( shape->data )->DrawBorder( reinterpret_cast<Space*>( space->data ) );
}

static void drawBB( cpShape *shape, void * unused ) {
	reinterpret_cast<Shape*> ( shape->data )->DrawBB();
}

static void drawConstraint( cpConstraint *constraint ) {
	reinterpret_cast<Constraint*> ( constraint->data )->Draw();
}
#endif

void Space::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED

	BatchRenderer * BR = GlobalBatchRenderer::instance();
	BR->setBlendMode( ALPHA_NORMAL );

	PhysicsManager::DrawSpaceOptions * options = PhysicsManager::instance()->GetDrawOptions();

	cpFloat lw = BR->getLineWidth();
	cpFloat ps = BR->getPointSize();

	BR->setLineWidth( options->LineThickness );

	if ( options->DrawShapes ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIteratorFunc)drawObject, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIteratorFunc)drawObject, mSpace );
	}

	if ( options->DrawShapesBorders ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIteratorFunc)drawObjectBorder, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIteratorFunc)drawObjectBorder, mSpace );
	}

	BR->setLineWidth( lw );

	if ( options->DrawBBs ){
		cpSpatialIndexEach( mSpace->CP_PRIVATE(activeShapes), (cpSpatialIndexIteratorFunc)drawBB, NULL );
		cpSpatialIndexEach( mSpace->CP_PRIVATE(staticShapes), (cpSpatialIndexIteratorFunc)drawBB, NULL );
		BR->draw();
	}

	cpArray * constraints = mSpace->CP_PRIVATE(constraints);

	for ( int i = 0, count = constraints->num; i < count; i++ ) {
		drawConstraint( (cpConstraint *)constraints->arr[i] );
	}

	if ( options->BodyPointSize ) {
		BR->setPointSize( options->BodyPointSize );
		BR->pointsBegin();
		BR->pointSetColor( ColorA( 255, 255, 255, 255 ) );

		cpArray * bodies = mSpace->CP_PRIVATE(bodies);

		for( int i=0, count = bodies->num; i<count; i++ ) {
			cpBody * body = (cpBody *)bodies->arr[i];

			BR->batchPoint( body->p.x, body->p.y );
		}

		BR->draw();
	}

	if ( options->CollisionPointSize ) {
		BR->setPointSize( options->CollisionPointSize );
		BR->pointsBegin();
		BR->pointSetColor( ColorA( 255, 0, 0, 255 ) );

		cpArray * arbiters = mSpace->CP_PRIVATE(arbiters);

		for( int i = 0; i < arbiters->num; i++ ){
			cpArbiter *arb = (cpArbiter*)arbiters->arr[i];

			for( int i=0; i< arb->CP_PRIVATE(numContacts); i++ ){
				cVect v = tovect( arb->CP_PRIVATE(contacts)[i].CP_PRIVATE(p) );
				BR->batchPoint( v.x, v.y );
			}
		}

		BR->draw();
	}

	BR->setLineWidth( lw );
	BR->setPointSize( ps );

	#endif
}

/** Collision Handling */

static cpBool RecieverCollisionBeginFunc( cpArbiter * arb, cpSpace * space, void * data ) {
	Space * tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	return tspace->OnCollisionBegin( &tarb, data );
}

static cpBool RecieverCollisionPreSolveFunc( cpArbiter * arb, cpSpace * space, void * data ) {
	Space * tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	return tspace->OnCollisionPreSolve( &tarb, data );
}

static void RecieverCollisionPostSolve( cpArbiter * arb, cpSpace * space, void * data ) {
	Space * tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	tspace->OnCollisionPostSolve( &tarb, data );
}

static void RecieverCollisionSeparateFunc( cpArbiter * arb, cpSpace * space, void * data ) {
	Space * tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	tspace->OnCollisionSeparate( &tarb, data );
}

static void RecieverPostStepCallback( cpSpace * space, void * obj, void * data ) {
	Space * tspace = reinterpret_cast<Space*>( space->data );

	tspace->OnPostStepCallback( obj, data );
}

static void RecieverBBQueryFunc( cpShape * shape, void * data ) {
	Space::cBBQuery * query = reinterpret_cast<Space::cBBQuery*>( data );

	query->Space->OnBBQuery( reinterpret_cast<Shape*>( shape->data ), query );
}

static void RecieverSegmentQueryFunc( cpShape *shape, cpFloat t, cpVect n, void * data ) {
	Space::cSegmentQuery * query = reinterpret_cast<Space::cSegmentQuery*>( data );

	query->Space->OnSegmentQuery( reinterpret_cast<Shape*>( shape->data ), t, tovect( n ), query );
}

static void RecieverPointQueryFunc( cpShape * shape, void * data ) {
	Space::cPointQuery * query = reinterpret_cast<Space::cPointQuery*>( data );

	query->Space->OnPointQuery( reinterpret_cast<Shape*>( shape->data ), query );
}

cpBool Space::OnCollisionBegin( Arbiter * arb, void * data ) {
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

cpBool Space::OnCollisionPreSolve( Arbiter * arb, void * data ) {
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

void Space::OnCollisionPostSolve( Arbiter * arb, void * data ) {
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

void Space::OnCollisionSeparate( Arbiter * arb, void * data ) {
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

void Space::OnPostStepCallback( void * obj, void * data ) {
	cPostStepCallback * Cb = reinterpret_cast<cPostStepCallback *> ( data );

	if ( Cb->Callback.IsSet() ) {
		Cb->Callback( this, obj, Cb->Data );
	}

	mPostStepCallbacks.remove( Cb );
	cpSAFE_DELETE( Cb );
}

void Space::OnBBQuery( Shape * shape, cBBQuery * query ) {
	if ( query->Func.IsSet() ) {
		query->Func( shape, query->Data );
	}
}

void Space::OnSegmentQuery( Shape * shape, cpFloat t, cVect n , cSegmentQuery * query ) {
	if ( query->Func.IsSet() ) {
		query->Func( shape, t, n, query->Data );
	}
}

void Space::OnPointQuery( Shape * shape, cPointQuery * query ) {
	if ( query->Func.IsSet() ) {
		query->Func( shape, query->Data );
	}
}

void Space::AddCollisionHandler( const cCollisionHandler& handler ) {
	cpHashValue hash = CP_HASH_PAIR( handler.a, handler.b );

	cpCollisionBeginFunc		f1 = ( handler.begin.IsSet() )		?	&RecieverCollisionBeginFunc		: NULL;
	cpCollisionPreSolveFunc		f2 = ( handler.preSolve.IsSet() )	?	&RecieverCollisionPreSolveFunc	: NULL;
	cpCollisionPostSolveFunc	f3 = ( handler.postSolve.IsSet() )	?	&RecieverCollisionPostSolve		: NULL;
	cpCollisionSeparateFunc		f4 = ( handler.separate.IsSet() )	?	&RecieverCollisionSeparateFunc	: NULL;

	cpSpaceAddCollisionHandler( mSpace, handler.a, handler.b, f1, f2, f3, f4, (void*)hash );

	mCollisions.erase( hash );
	mCollisions[ hash ] = handler;
}

void Space::RemoveCollisionHandler( cpCollisionType a, cpCollisionType b ) {
	cpSpaceRemoveCollisionHandler( mSpace, a, b );

	mCollisions.erase( CP_HASH_PAIR( a, b ) );
}

void Space::SetDefaultCollisionHandler( const cCollisionHandler& handler ) {
	cpCollisionBeginFunc		f1 = ( handler.begin.IsSet() )		?	&RecieverCollisionBeginFunc		: NULL;
	cpCollisionPreSolveFunc		f2 = ( handler.preSolve.IsSet() )	?	&RecieverCollisionPreSolveFunc	: NULL;
	cpCollisionPostSolveFunc	f3 = ( handler.postSolve.IsSet() )	?	&RecieverCollisionPostSolve		: NULL;
	cpCollisionSeparateFunc		f4 = ( handler.separate.IsSet() )	?	&RecieverCollisionSeparateFunc	: NULL;

	cpSpaceSetDefaultCollisionHandler( mSpace, f1, f2, f3, f4, NULL );

	mCollisionsDefault	= handler;
}

void Space::AddPostStepCallback( PostStepCallback postStep, void * obj, void * data ) {
	cPostStepCallback * PostStepCb	= cpNew( cPostStepCallback, () );
	PostStepCb->Callback			= postStep,
	PostStepCb->Data				= data;

	cpSpaceAddPostStepCallback( mSpace, &RecieverPostStepCallback, obj, PostStepCb );
	mPostStepCallbacks.push_back( PostStepCb );	
}

void Space::BBQuery( cBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void * data ) {
	cBBQuery tBBQuery;
	tBBQuery.Space	= this;
	tBBQuery.Data	= data;
	tBBQuery.Func	= func;

	cpSpaceBBQuery( mSpace, tocpbb( bb ), layers, group, &RecieverBBQueryFunc, reinterpret_cast<void*>( &tBBQuery ) );
}

void Space::SegmentQuery( cVect start, cVect end, cpLayers layers, cpGroup group, SegmentQueryFunc func, void * data ) {
	cSegmentQuery tSegmentQuery;

	tSegmentQuery.Space	= this;
	tSegmentQuery.Data	= data;
	tSegmentQuery.Func	= func;

	cpSpaceSegmentQuery( mSpace, tocpv( start ), tocpv( end ), layers, group, &RecieverSegmentQueryFunc, reinterpret_cast<void*>( &tSegmentQuery ) );
}

void Space::PointQuery( cVect point, cpLayers layers, cpGroup group, PointQueryFunc func, void * data ) {
	cPointQuery tPointQuery;
	tPointQuery.Space	= this;
	tPointQuery.Data	= data;
	tPointQuery.Func	= func;

	cpSpacePointQuery( mSpace, tocpv( point ), layers, group, &RecieverPointQueryFunc, reinterpret_cast<void*>( &tPointQuery ) );
}

void Space::ReindexShape( Shape * shape ) {
	cpSpaceReindexShape( mSpace, shape->GetShape() );
}

void Space::ReindexShapesForBody( Body *body ) {
	cpSpaceReindexShapesForBody( mSpace, body->GetBody() );
}

void Space::ReindexStatic() {
	cpSpaceReindexStatic( mSpace );
}

void Space::UseSpatialHash( cpFloat dim, int count ) {
	cpSpaceUseSpatialHash( mSpace, dim, count );
}

static void SpaceBodyIteratorFunc( cpBody * body, void *data ) {
	Space::BodyIterator * it = reinterpret_cast<Space::BodyIterator *> ( data );
	it->Space->OnEachBody( reinterpret_cast<Body*>( body->data ), it );
}

void Space::EachBody( BodyIteratorFunc Func, void * data ) {
	BodyIterator it( this, data, Func );
	cpSpaceEachBody( mSpace, &SpaceBodyIteratorFunc, (void*)&it );
}

void Space::OnEachBody( Body * Body, BodyIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( it->Space, Body, it->Data );
	}
}

static void SpaceShapeIteratorFunc ( cpShape * shape, void * data ) {
	Space::ShapeIterator * it = reinterpret_cast<Space::ShapeIterator *> ( data );
	it->Space->OnEachShape( reinterpret_cast<Shape*>( shape->data ), it );
}

void Space::EachShape( ShapeIteratorFunc Func, void * data ) {
	ShapeIterator it( this, data, Func );
	cpSpaceEachShape( mSpace, &SpaceShapeIteratorFunc, (void*)&it );
}

void Space::OnEachShape( Shape * Shape, ShapeIterator * it ) {
	if ( it->Func.IsSet() ) {
		it->Func( it->Space, Shape, it->Data );
	}
}

void Space::ConvertBodyToDynamic( Body * body, cpFloat mass, cpFloat moment ) {
	cpSpaceConvertBodyToDynamic( mSpace, body->GetBody(), mass, moment );
}

void Space::ConvertBodyToStatic(Body * body ) {
	cpSpaceConvertBodyToStatic( mSpace, body->GetBody() );
}

CP_NAMESPACE_END
