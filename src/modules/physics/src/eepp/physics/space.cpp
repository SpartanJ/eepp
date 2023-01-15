#include <eepp/physics/physicsmanager.hpp>
#include <eepp/physics/space.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>
using namespace EE::Graphics;
#endif

namespace EE { namespace Physics {

Space* Space::New() {
	return eeNew( Space, () );
}

void Space::Free( Space* space ) {
	eeSAFE_DELETE( space );
}

Space::Space() : mData( NULL ) {
	mSpace = cpSpaceNew();
	mSpace->data = (void*)this;
	mStatiBody = eeNew( Body, ( mSpace->staticBody ) );

	PhysicsManager::instance()->removeBodyFree( mStatiBody );
	PhysicsManager::instance()->addSpace( this );
}

Space::~Space() {
	cpSpaceFree( mSpace );

	std::list<Constraint*>::iterator itc = mConstraints.begin();
	for ( ; itc != mConstraints.end(); ++itc )
		eeSAFE_DELETE( *itc );

	std::list<Shape*>::iterator its = mShapes.begin();
	for ( ; its != mShapes.end(); ++its )
		eeSAFE_DELETE( *its );

	std::list<Body*>::iterator itb = mBodys.begin();
	for ( ; itb != mBodys.end(); ++itb )
		eeSAFE_DELETE( *itb );

	mStatiBody->mBody = NULL; // The body has been released by cpSpaceFree( mSpace )

	eeSAFE_DELETE( mStatiBody );

	PhysicsManager::instance()->removeSpace( this );
}

void Space::setData( void* data ) {
	mData = data;
}

void* Space::getData() const {
	return mData;
}

void Space::step( const cpFloat& dt ) {
	cpSpaceStep( mSpace, dt );
}

void Space::update() {
#ifdef PHYSICS_RENDERER_ENABLED
	step( Window::Engine::instance()->getCurrentWindow()->getElapsed().asSeconds() );
#else
	Step( 1 / 60 );
#endif
}

const int& Space::getIterations() const {
	return mSpace->iterations;
}

void Space::setIterations( const int& iterations ) {
	mSpace->iterations = iterations;
}

cVect Space::getGravity() const {
	return tovect( mSpace->gravity );
}

void Space::setGravity( const cVect& gravity ) {
	mSpace->gravity = tocpv( gravity );
}

const cpFloat& Space::getDamping() const {
	return mSpace->damping;
}

void Space::setDamping( const cpFloat& damping ) {
	mSpace->damping = damping;
}

const cpFloat& Space::getIdleSpeedThreshold() const {
	return mSpace->idleSpeedThreshold;
}

void Space::setIdleSpeedThreshold( const cpFloat& idleSpeedThreshold ) {
	mSpace->idleSpeedThreshold = idleSpeedThreshold;
}

const cpFloat& Space::getSleepTimeThreshold() const {
	return mSpace->sleepTimeThreshold;
}

void Space::setSleepTimeThreshold( const cpFloat& sleepTimeThreshold ) {
	mSpace->sleepTimeThreshold = sleepTimeThreshold;
}

void Space::setCollisionSlop( cpFloat slop ) {
	mSpace->collisionSlop = slop;
}

cpFloat Space::getCollisionSlop() const {
	return mSpace->collisionSlop;
}

void Space::setCollisionBias( cpFloat bias ) {
	mSpace->collisionBias = bias;
}

cpFloat Space::getCollisionBias() const {
	return mSpace->collisionBias;
}

cpTimestamp Space::getCollisionPersistence() {
	return cpSpaceGetCollisionPersistence( mSpace );
}

void Space::setCollisionPersistence( cpTimestamp value ) {
	cpSpaceSetCollisionPersistence( mSpace, value );
}

bool Space::getEnableContactGraph() {
	return cpTrue == cpSpaceGetEnableContactGraph( mSpace );
}

void Space::setEnableContactGraph( bool value ) {
	cpSpaceSetEnableContactGraph( mSpace, value );
}

Body* Space::getStaticBody() const {
	return mStatiBody;
}

bool Space::contains( Shape* shape ) {
	return cpTrue == cpSpaceContainsShape( mSpace, shape->getShape() );
}

bool Space::contains( Body* body ) {
	return cpTrue == cpSpaceContainsBody( mSpace, body->getBody() );
}

bool Space::contains( Constraint* constraint ) {
	return cpTrue == cpSpaceContainsConstraint( mSpace, constraint->getConstraint() );
}

Shape* Space::addShape( Shape* shape ) {
	cpSpaceAddShape( mSpace, shape->getShape() );

	mShapes.push_back( shape );

	PhysicsManager::instance()->removeShapeFree( shape );

	return shape;
}

Shape* Space::addStaticShape( Shape* shape ) {
	cpSpaceAddStaticShape( mSpace, shape->getShape() );

	mShapes.push_back( shape );

	PhysicsManager::instance()->removeShapeFree( shape );

	return shape;
}

Body* Space::addBody( Body* body ) {
	cpSpaceAddBody( mSpace, body->getBody() );

	mBodys.push_back( body );

	PhysicsManager::instance()->removeBodyFree( body );

	return body;
}

Constraint* Space::addConstraint( Constraint* constraint ) {
	cpSpaceAddConstraint( mSpace, constraint->getConstraint() );

	mConstraints.push_back( constraint );

	PhysicsManager::instance()->removeConstraintFree( constraint );

	return constraint;
}

void Space::removeShape( Shape* shape ) {
	if ( NULL != shape ) {
		cpSpaceRemoveShape( mSpace, shape->getShape() );

		mShapes.remove( shape );

		PhysicsManager::instance()->addShapeFree( shape );
	}
}

void Space::removeStatiShape( Shape* shape ) {
	if ( NULL != shape ) {
		cpSpaceRemoveStaticShape( mSpace, shape->getShape() );

		mShapes.remove( shape );

		PhysicsManager::instance()->addShapeFree( shape );
	}
}

void Space::removeBody( Body* body ) {
	if ( NULL != body ) {
		cpSpaceRemoveBody( mSpace, body->getBody() );

		mBodys.remove( body );

		PhysicsManager::instance()->removeBodyFree( body );
	}
}

void Space::removeConstraint( Constraint* constraint ) {
	if ( NULL != constraint ) {
		cpSpaceRemoveConstraint( mSpace, constraint->getConstraint() );

		mConstraints.remove( constraint );

		PhysicsManager::instance()->addConstraintFree( constraint );
	}
}

Shape* Space::pointQueryFirst( cVect point, cpLayers layers, cpGroup group ) {
	cpShape* shape = cpSpacePointQueryFirst( mSpace, tocpv( point ), layers, group );

	if ( NULL != shape ) {
		return reinterpret_cast<Shape*>( shape->data );
	}

	return NULL;
}

Shape* Space::segmentQueryFirst( cVect start, cVect end, cpLayers layers, cpGroup group,
								 cpSegmentQueryInfo* out ) {
	cpShape* shape =
		cpSpaceSegmentQueryFirst( mSpace, tocpv( start ), tocpv( end ), layers, group, out );

	if ( NULL != shape ) {
		return reinterpret_cast<Shape*>( shape->data );
	}

	return NULL;
}

cpSpace* Space::getSpace() const {
	return mSpace;
}

void Space::activateShapesTouchingShape( Shape* shape ) {
	cpSpaceActivateShapesTouchingShape( mSpace, shape->getShape() );
}

#ifdef PHYSICS_RENDERER_ENABLED
static void drawObject( cpShape* shape, cpSpace* space ) {
	reinterpret_cast<Shape*>( shape->data )->draw( reinterpret_cast<Space*>( space->data ) );
}

static void drawObjectBorder( cpShape* shape, cpSpace* space ) {
	reinterpret_cast<Shape*>( shape->data )->drawBorder( reinterpret_cast<Space*>( space->data ) );
}

static void drawBB( cpShape* shape, void* unused ) {
	reinterpret_cast<Shape*>( shape->data )->drawBB();
}

static void drawConstraint( cpConstraint* constraint ) {
	reinterpret_cast<Constraint*>( constraint->data )->draw();
}
#endif

void Space::draw() {
#ifdef PHYSICS_RENDERER_ENABLED

	BatchRenderer* BR = GlobalBatchRenderer::instance();
	BR->setBlendMode( BlendMode::Alpha() );

	PhysicsManager::DrawSpaceOptions* options = PhysicsManager::instance()->getDrawOptions();

	cpFloat lw = BR->getLineWidth();
	cpFloat ps = BR->getPointSize();

	BR->setLineWidth( options->LineThickness );

	if ( options->DrawShapes ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE( activeShapes ),
							(cpSpatialIndexIteratorFunc)drawObject, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE( staticShapes ),
							(cpSpatialIndexIteratorFunc)drawObject, mSpace );
	}

	if ( options->DrawShapesBorders ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE( activeShapes ),
							(cpSpatialIndexIteratorFunc)drawObjectBorder, mSpace );
		cpSpatialIndexEach( mSpace->CP_PRIVATE( staticShapes ),
							(cpSpatialIndexIteratorFunc)drawObjectBorder, mSpace );
	}

	BR->setLineWidth( lw );

	if ( options->DrawBBs ) {
		cpSpatialIndexEach( mSpace->CP_PRIVATE( activeShapes ), (cpSpatialIndexIteratorFunc)drawBB,
							NULL );
		cpSpatialIndexEach( mSpace->CP_PRIVATE( staticShapes ), (cpSpatialIndexIteratorFunc)drawBB,
							NULL );
		BR->draw();
	}

	cpArray* constraints = mSpace->CP_PRIVATE( constraints );

	for ( int i = 0, count = constraints->num; i < count; i++ ) {
		drawConstraint( (cpConstraint*)constraints->arr[i] );
	}

	if ( options->BodyPointSize ) {
		BR->setPointSize( options->BodyPointSize );
		BR->pointsBegin();
		BR->pointSetColor( Color( 255, 255, 255, 255 ) );

		cpArray* bodies = mSpace->CP_PRIVATE( bodies );

		for ( int i = 0, count = bodies->num; i < count; i++ ) {
			cpBody* body = (cpBody*)bodies->arr[i];

			BR->batchPoint( body->p.x, body->p.y );
		}

		BR->draw();
	}

	if ( options->CollisionPointSize ) {
		BR->setPointSize( options->CollisionPointSize );
		BR->pointsBegin();
		BR->pointSetColor( Color( 255, 0, 0, 255 ) );

		cpArray* arbiters = mSpace->CP_PRIVATE( arbiters );

		for ( int i = 0; i < arbiters->num; i++ ) {
			cpArbiter* arb = (cpArbiter*)arbiters->arr[i];

			for ( int i = 0; i < arb->CP_PRIVATE( numContacts ); i++ ) {
				cVect v = tovect( arb->CP_PRIVATE( contacts )[i].CP_PRIVATE( p ) );
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

static cpBool RecieverCollisionBeginFunc( cpArbiter* arb, cpSpace* space, void* data ) {
	Space* tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	return tspace->onCollisionBegin( &tarb, data );
}

static cpBool RecieverCollisionPreSolveFunc( cpArbiter* arb, cpSpace* space, void* data ) {
	Space* tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	return tspace->onCollisionPreSolve( &tarb, data );
}

static void RecieverCollisionPostSolve( cpArbiter* arb, cpSpace* space, void* data ) {
	Space* tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	tspace->onCollisionPostSolve( &tarb, data );
}

static void RecieverCollisionSeparateFunc( cpArbiter* arb, cpSpace* space, void* data ) {
	Space* tspace = reinterpret_cast<Space*>( space->data );
	Arbiter tarb( arb );

	tspace->onCollisionSeparate( &tarb, data );
}

static void RecieverPostStepCallback( cpSpace* space, void* obj, void* data ) {
	Space* tspace = reinterpret_cast<Space*>( space->data );

	tspace->onPostStepCallback( obj, data );
}

static void RecieverBBQueryFunc( cpShape* shape, void* data ) {
	Space::BBQuery* query = reinterpret_cast<Space::BBQuery*>( data );

	query->Space->onBBQuery( reinterpret_cast<Shape*>( shape->data ), query );
}

static void RecieverSegmentQueryFunc( cpShape* shape, cpFloat t, cpVect n, void* data ) {
	Space::SegmentQuery* query = reinterpret_cast<Space::SegmentQuery*>( data );

	query->Space->onSegmentQuery( reinterpret_cast<Shape*>( shape->data ), t, tovect( n ), query );
}

static void RecieverPointQueryFunc( cpShape* shape, void* data ) {
	Space::PointQuery* query = reinterpret_cast<Space::PointQuery*>( data );

	query->Space->onPointQuery( reinterpret_cast<Shape*>( shape->data ), query );
}

cpBool Space::onCollisionBegin( Arbiter* arb, void* data ) {
	cpHashValue hash = (cpHashValue)data;

	// if ( NULL != data ) {
	std::map<cpHashValue, CollisionHandler>::iterator it = mCollisions.find( hash );
	CollisionHandler handler = static_cast<CollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.begin ) {
		return handler.begin( arb, this, handler.data );
	}
	//}

	if ( mCollisionsDefault.begin ) {
		return mCollisionsDefault.begin( arb, this, mCollisionsDefault.data );
	}

	return 1;
}

cpBool Space::onCollisionPreSolve( Arbiter* arb, void* data ) {
	cpHashValue hash = (cpHashValue)data;

	// if ( NULL != data ) {
	std::map<cpHashValue, CollisionHandler>::iterator it = mCollisions.find( hash );
	CollisionHandler handler = static_cast<CollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.preSolve ) {
		return handler.preSolve( arb, this, handler.data );
	}
	//}

	if ( mCollisionsDefault.preSolve ) {
		return mCollisionsDefault.preSolve( arb, this, mCollisionsDefault.data );
	}

	return 1;
}

void Space::onCollisionPostSolve( Arbiter* arb, void* data ) {
	cpHashValue hash = (cpHashValue)data;

	// if ( NULL != data ) {
	std::map<cpHashValue, CollisionHandler>::iterator it = mCollisions.find( hash );
	CollisionHandler handler = static_cast<CollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.postSolve ) {
		handler.postSolve( arb, this, handler.data );
		return;
	}
	//}

	if ( mCollisionsDefault.begin ) {
		mCollisionsDefault.postSolve( arb, this, mCollisionsDefault.data );
	}
}

void Space::onCollisionSeparate( Arbiter* arb, void* data ) {
	cpHashValue hash = (cpHashValue)data;

	// if ( NULL != data ) {
	std::map<cpHashValue, CollisionHandler>::iterator it = mCollisions.find( hash );
	CollisionHandler handler = static_cast<CollisionHandler>( it->second );

	if ( it != mCollisions.end() && handler.separate ) {
		handler.separate( arb, this, handler.data );
		return;
	}
	//}

	if ( mCollisionsDefault.begin ) {
		mCollisionsDefault.separate( arb, this, mCollisionsDefault.data );
	}
}

void Space::onPostStepCallback( void* obj, void* data ) {
	PostStepCallbackCont* Cb = reinterpret_cast<PostStepCallbackCont*>( data );

	if ( Cb->Callback ) {
		Cb->Callback( this, obj, Cb->Data );
	}

	mPostStepCallbacks.remove( Cb );
	eeSAFE_DELETE( Cb );
}

void Space::onBBQuery( Shape* shape, BBQuery* query ) {
	if ( query->Func ) {
		query->Func( shape, query->Data );
	}
}

void Space::onSegmentQuery( Shape* shape, cpFloat t, cVect n, SegmentQuery* query ) {
	if ( query->Func ) {
		query->Func( shape, t, n, query->Data );
	}
}

void Space::onPointQuery( Shape* shape, PointQuery* query ) {
	if ( query->Func ) {
		query->Func( shape, query->Data );
	}
}

void Space::addCollisionHandler( const CollisionHandler& handler ) {
	cpHashValue hash = CP_HASH_PAIR( handler.a, handler.b );

	cpCollisionBeginFunc f1 = ( handler.begin ) ? &RecieverCollisionBeginFunc : NULL;
	cpCollisionPreSolveFunc f2 = ( handler.preSolve ) ? &RecieverCollisionPreSolveFunc : NULL;
	cpCollisionPostSolveFunc f3 = ( handler.postSolve ) ? &RecieverCollisionPostSolve : NULL;
	cpCollisionSeparateFunc f4 = ( handler.separate ) ? &RecieverCollisionSeparateFunc : NULL;

	cpSpaceAddCollisionHandler( mSpace, handler.a, handler.b, f1, f2, f3, f4, (void*)hash );

	mCollisions.erase( hash );
	mCollisions[hash] = handler;
}

void Space::removeCollisionHandler( cpCollisionType a, cpCollisionType b ) {
	cpSpaceRemoveCollisionHandler( mSpace, a, b );

	mCollisions.erase( CP_HASH_PAIR( a, b ) );
}

void Space::setDefaultCollisionHandler( const CollisionHandler& handler ) {
	cpCollisionBeginFunc f1 = ( handler.begin ) ? &RecieverCollisionBeginFunc : NULL;
	cpCollisionPreSolveFunc f2 = ( handler.preSolve ) ? &RecieverCollisionPreSolveFunc : NULL;
	cpCollisionPostSolveFunc f3 = ( handler.postSolve ) ? &RecieverCollisionPostSolve : NULL;
	cpCollisionSeparateFunc f4 = ( handler.separate ) ? &RecieverCollisionSeparateFunc : NULL;

	cpSpaceSetDefaultCollisionHandler( mSpace, f1, f2, f3, f4, NULL );

	mCollisionsDefault = handler;
}

void Space::addPostStepCallback( PostStepCallback postStep, void* obj, void* data ) {
	PostStepCallbackCont* PostStepCb = eeNew( PostStepCallbackCont, () );
	PostStepCb->Callback = postStep, PostStepCb->Data = data;

	cpSpaceAddPostStepCallback( mSpace, &RecieverPostStepCallback, obj, PostStepCb );
	mPostStepCallbacks.push_back( PostStepCb );
}

void Space::bbQuery( cBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void* data ) {
	BBQuery tBBQuery;
	tBBQuery.Space = this;
	tBBQuery.Data = data;
	tBBQuery.Func = func;

	cpSpaceBBQuery( mSpace, tocpbb( bb ), layers, group, &RecieverBBQueryFunc,
					reinterpret_cast<void*>( &tBBQuery ) );
}

void Space::segmentQuery( cVect start, cVect end, cpLayers layers, cpGroup group,
						  SegmentQueryFunc func, void* data ) {
	SegmentQuery tSegmentQuery;

	tSegmentQuery.Space = this;
	tSegmentQuery.Data = data;
	tSegmentQuery.Func = func;

	cpSpaceSegmentQuery( mSpace, tocpv( start ), tocpv( end ), layers, group,
						 &RecieverSegmentQueryFunc, reinterpret_cast<void*>( &tSegmentQuery ) );
}

void Space::pointQuery( cVect point, cpLayers layers, cpGroup group, PointQueryFunc func,
						void* data ) {
	PointQuery tPointQuery;
	tPointQuery.Space = this;
	tPointQuery.Data = data;
	tPointQuery.Func = func;

	cpSpacePointQuery( mSpace, tocpv( point ), layers, group, &RecieverPointQueryFunc,
					   reinterpret_cast<void*>( &tPointQuery ) );
}

void Space::reindexShape( Shape* shape ) {
	cpSpaceReindexShape( mSpace, shape->getShape() );
}

void Space::reindexShapesForBody( Body* body ) {
	cpSpaceReindexShapesForBody( mSpace, body->getBody() );
}

void Space::reindexStatic() {
	cpSpaceReindexStatic( mSpace );
}

void Space::useSpatialHash( cpFloat dim, int count ) {
	cpSpaceUseSpatialHash( mSpace, dim, count );
}

static void SpaceBodyIteratorFunc( cpBody* body, void* data ) {
	Space::BodyIterator* it = reinterpret_cast<Space::BodyIterator*>( data );
	it->Space->onEachBody( reinterpret_cast<Body*>( body->data ), it );
}

void Space::eachBody( BodyIteratorFunc Func, void* data ) {
	BodyIterator it( this, data, Func );
	cpSpaceEachBody( mSpace, &SpaceBodyIteratorFunc, (void*)&it );
}

void Space::onEachBody( Body* Body, BodyIterator* it ) {
	if ( it->Func ) {
		it->Func( it->Space, Body, it->Data );
	}
}

static void SpaceShapeIteratorFunc( cpShape* shape, void* data ) {
	Space::ShapeIterator* it = reinterpret_cast<Space::ShapeIterator*>( data );
	it->Space->onEachShape( reinterpret_cast<Shape*>( shape->data ), it );
}

void Space::eachShape( ShapeIteratorFunc Func, void* data ) {
	ShapeIterator it( this, data, Func );
	cpSpaceEachShape( mSpace, &SpaceShapeIteratorFunc, (void*)&it );
}

void Space::onEachShape( Shape* Shape, ShapeIterator* it ) {
	if ( it->Func ) {
		it->Func( it->Space, Shape, it->Data );
	}
}

void Space::convertBodyToDynamic( Body* body, cpFloat mass, cpFloat moment ) {
	cpSpaceConvertBodyToDynamic( mSpace, body->getBody(), mass, moment );
}

void Space::convertBodyToStatic( Body* body ) {
	cpSpaceConvertBodyToStatic( mSpace, body->getBody() );
}

}} // namespace EE::Physics
