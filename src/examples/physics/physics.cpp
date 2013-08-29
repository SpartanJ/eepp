#include <eepp/ee.hpp>

/**
The physics module is a OOP wrapper for Chipmunk Physics.
To understand the conceptos of space, body, shapes, etc you can read the
Chipmunk documentation:
http://chipmunk-physics.net/release/ChipmunkLatest-Docs/



*/
typedef cb::Callback0<void> SceneCb;

struct physicDemo {
	SceneCb init;
	SceneCb update;
	SceneCb destroy;
};

std::vector<physicDemo> mDemo;
eeInt mCurDemo = eeINDEX_NOT_FOUND;
cSpace * mSpace;
cBody * mMouseBody;
cVect mMousePoint;
cVect mMousePoint_last;
cConstraint * mMouseJoint;

#define GRABABLE_MASK_BIT (1<<31)
#define NOT_GRABABLE_MASK (~GRABABLE_MASK_BIT)

void CreateJointAndBody() {
	mMouseJoint	= NULL;
	mMouseBody	= eeNew( cBody, ( INFINITY, INFINITY ) );
}

cWindow * mWindow;
cInput * KM;

void DefaultDrawOptions() {
	cPhysicsManager::cDrawSpaceOptions * DSO = cPhysicsManager::instance()->GetDrawOptions();
	DSO->DrawBBs			= false;
	DSO->DrawShapes			= true;
	DSO->DrawShapesBorders	= true;
	DSO->CollisionPointSize	= 4;
	DSO->BodyPointSize		= 0;
	DSO->LineThickness		= 1;
}

void DestroyDemo() {
	eeSAFE_DELETE( mMouseBody );
	eeSAFE_DELETE( mSpace );
}

static const int image_width = 188;
static const int image_height = 35;
static const int image_row_length = 24;

static const char image_bitmap[] = {
	15,-16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,-64,15,63,-32,-2,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,31,-64,15,127,-125,-1,-128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,127,-64,15,127,15,-1,-64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,-1,-64,15,-2,
	31,-1,-64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,-1,-64,0,-4,63,-1,-32,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,1,-1,-64,15,-8,127,-1,-32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,-1,-64,0,-8,-15,-1,-32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,-31,-1,-64,15,-8,-32,
	-1,-32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,-15,-1,-64,9,-15,-32,-1,-32,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,31,-15,-1,-64,0,-15,-32,-1,-32,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,63,-7,-1,-64,9,-29,-32,127,-61,-16,63,15,-61,-1,-8,31,-16,15,-8,126,7,-31,
	-8,31,-65,-7,-1,-64,9,-29,-32,0,7,-8,127,-97,-25,-1,-2,63,-8,31,-4,-1,15,-13,
	-4,63,-1,-3,-1,-64,9,-29,-32,0,7,-8,127,-97,-25,-1,-2,63,-8,31,-4,-1,15,-13,
	-2,63,-1,-3,-1,-64,9,-29,-32,0,7,-8,127,-97,-25,-1,-1,63,-4,63,-4,-1,15,-13,
	-2,63,-33,-1,-1,-32,9,-25,-32,0,7,-8,127,-97,-25,-1,-1,63,-4,63,-4,-1,15,-13,
	-1,63,-33,-1,-1,-16,9,-25,-32,0,7,-8,127,-97,-25,-1,-1,63,-4,63,-4,-1,15,-13,
	-1,63,-49,-1,-1,-8,9,-57,-32,0,7,-8,127,-97,-25,-8,-1,63,-2,127,-4,-1,15,-13,
	-1,-65,-49,-1,-1,-4,9,-57,-32,0,7,-8,127,-97,-25,-8,-1,63,-2,127,-4,-1,15,-13,
	-1,-65,-57,-1,-1,-2,9,-57,-32,0,7,-8,127,-97,-25,-8,-1,63,-2,127,-4,-1,15,-13,
	-1,-1,-57,-1,-1,-1,9,-57,-32,0,7,-1,-1,-97,-25,-8,-1,63,-1,-1,-4,-1,15,-13,-1,
	-1,-61,-1,-1,-1,-119,-57,-32,0,7,-1,-1,-97,-25,-8,-1,63,-1,-1,-4,-1,15,-13,-1,
	-1,-61,-1,-1,-1,-55,-49,-32,0,7,-1,-1,-97,-25,-8,-1,63,-1,-1,-4,-1,15,-13,-1,
	-1,-63,-1,-1,-1,-23,-49,-32,127,-57,-1,-1,-97,-25,-1,-1,63,-1,-1,-4,-1,15,-13,
	-1,-1,-63,-1,-1,-1,-16,-49,-32,-1,-25,-1,-1,-97,-25,-1,-1,63,-33,-5,-4,-1,15,
	-13,-1,-1,-64,-1,-9,-1,-7,-49,-32,-1,-25,-8,127,-97,-25,-1,-1,63,-33,-5,-4,-1,
	15,-13,-1,-1,-64,-1,-13,-1,-32,-49,-32,-1,-25,-8,127,-97,-25,-1,-2,63,-49,-13,
	-4,-1,15,-13,-1,-1,-64,127,-7,-1,-119,-17,-15,-1,-25,-8,127,-97,-25,-1,-2,63,
	-49,-13,-4,-1,15,-13,-3,-1,-64,127,-8,-2,15,-17,-1,-1,-25,-8,127,-97,-25,-1,
	-8,63,-49,-13,-4,-1,15,-13,-3,-1,-64,63,-4,120,0,-17,-1,-1,-25,-8,127,-97,-25,
	-8,0,63,-57,-29,-4,-1,15,-13,-4,-1,-64,63,-4,0,15,-17,-1,-1,-25,-8,127,-97,
	-25,-8,0,63,-57,-29,-4,-1,-1,-13,-4,-1,-64,31,-2,0,0,103,-1,-1,-57,-8,127,-97,
	-25,-8,0,63,-57,-29,-4,-1,-1,-13,-4,127,-64,31,-2,0,15,103,-1,-1,-57,-8,127,
	-97,-25,-8,0,63,-61,-61,-4,127,-1,-29,-4,127,-64,15,-8,0,0,55,-1,-1,-121,-8,
	127,-97,-25,-8,0,63,-61,-61,-4,127,-1,-29,-4,63,-64,15,-32,0,0,23,-1,-2,3,-16,
	63,15,-61,-16,0,31,-127,-127,-8,31,-1,-127,-8,31,-128,7,-128,0,0
};

int get_pixel(int x, int y) {
	return (image_bitmap[(x>>3) + y*image_row_length]>>(~x&0x7)) & 1;
}

cShape * make_ball( cpFloat x, cpFloat y ) {
	cBody * body = cBody::New( 1.0, INFINITY );

	body->Pos( cVectNew( x, y ) );

	cShapePoint * shape = cShapePoint::New( body, 0.95, cVectZero );

	shape->DrawRadius( 4 );
	shape->Elasticity( 0.0 );
	shape->Friction( 0.0 );

	return shape;
}

static int bodyCount = 0;

void Demo1Create() {
	DefaultDrawOptions();

	CreateJointAndBody();

	mWindow->Caption( "eepp - Physics - Logo Smash" );

	mSpace = Physics::cSpace::New();
	mSpace->Iterations( 1 );

	// The space will contain a very large number of similary sized objects.
	// This is the perfect candidate for using the spatial hash.
	// Generally you will never need to do this.
	mSpace->UseSpatialHash( 2.0, 10000 );

	bodyCount = 0;

	cBody * body;
	cShape * shape;

	eeFloat pX = mWindow->GetWidth()	/ 2 - ( image_width		* 4 ) / 2;
	eeFloat pY = mWindow->GetHeight()	/ 2 - ( image_height	* 4 ) / 2;

	for(int y=0; y<image_height; y++){
		for(int x=0; x<image_width; x++){
			if( !get_pixel(x, y) ) continue;

			shape = make_ball( pX + x * 4, pY + y * 4 );

			mSpace->AddBody( shape->Body() );
			mSpace->AddShape( shape );

			bodyCount++;
		}
	}

	body = mSpace->AddBody( cBody::New( INFINITY, INFINITY ) );
	body->Pos( cVectNew( 0, mWindow->GetHeight() / 2 + 16 ) );
	body->Vel( cVectNew( 400, 0 ) );

	shape = mSpace->AddShape( cShapeCircle::New( body, 8.0f, cVectZero ) );
	shape->Elasticity( 0.0 );
	shape->Friction( 0.0 );
	shape->Layers( NOT_GRABABLE_MASK );

	bodyCount++;
}

void Demo1Update(){
}

void Demo1Destroy() {
	DestroyDemo();
}

void Demo2Create() {
	DefaultDrawOptions();

	CreateJointAndBody();

	mWindow->Caption( "eepp - Physics - Pyramid Stack" );

	cShape::ResetShapeIdCounter();

	mSpace = Physics::cSpace::New();
	mSpace->Gravity( cVectNew( 0, 100 ) );
	mSpace->SleepTimeThreshold( 0.5f );

	cBody *body, *staticBody = mSpace->StaticBody();
	cShape * shape;

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, mWindow->GetHeight() ), cVectNew( mWindow->GetWidth(), mWindow->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( mWindow->GetWidth(), 0 ), cVectNew( mWindow->GetWidth(), mWindow->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, 0 ), cVectNew( 0, mWindow->GetHeight() ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( 0, 0 ), cVectNew( mWindow->GetWidth(), 0 ), 0.0f ) );
	shape->e( 1.0f );
	shape->u( 1.0f );
	shape->Layers( NOT_GRABABLE_MASK );

	eeFloat hw = mWindow->GetWidth() / 2;

	for(int i=0; i<14; i++){
		for(int j=0; j<=i; j++){
			body = mSpace->AddBody( cBody::New( 1.0f, Moment::ForBox( 1.0f, 30.0f, 30.0f ) ) );
			body->Pos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			shape = mSpace->AddShape( cShapePoly::New( body, 30.f, 30.f ) );
			shape->e( 0.0f );
			shape->u( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body = mSpace->AddBody( cBody::New( 10.0f, Moment::ForCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->Pos( cVectNew( hw, mWindow->GetHeight() - radius - 5 ) );

	shape = mSpace->AddShape( cShapeCircle::New( body, radius, cVectZero ) );
	shape->e( 0.0f );
	shape->u( 0.9f );
}

void Demo2Update() {
}

void Demo2Destroy() {
	DestroyDemo();
}

enum CollisionTypes {
	BALL_TYPE,
	BLOCKING_SENSOR_TYPE,
	CATCH_SENSOR_TYPE
};

struct Emitter {
	int queue;
	int blocked;
	cVect position;
};
Emitter emitterInstance;

cpBool blockerBegin( cArbiter *arb, cSpace *space, void *unused ) {
	cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void blockerSeparate( cArbiter *arb, cSpace * space, void *unused ) {
	cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->blocked--;
}

void postStepRemove( cSpace *space, void * tshape, void * unused ) {
	cShape * shape = reinterpret_cast<cShape*>( tshape );

	if ( NULL != mMouseJoint && ( mMouseJoint->A() == shape->Body() || mMouseJoint->B() == shape->Body() ) ) {
		space->RemoveConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}

	space->RemoveBody( shape->Body() );
	space->RemoveShape( shape );

	cShape::Free( shape, true );
}

cpBool catcherBarBegin(cArbiter *arb, cSpace *space, void *unused) {
	cShape * a, * b;
	arb->GetShapes( &a, &b );

	Emitter *emitter = (Emitter *) a->Data();

	emitter->queue++;

	space->AddPostStepCallback( cb::Make3( &postStepRemove ), b, NULL );

	return cpFalse;
}

void Demo3Create() {
	DefaultDrawOptions();

	CreateJointAndBody();

	mWindow->Caption( "eepp - Physics - Sensor" );

	cShape::ResetShapeIdCounter();

	mSpace = Physics::cSpace::New();
	mSpace->Iterations( 10 );
	mSpace->Gravity( cVectNew( 0, 100 ) );

	cBody * staticBody = mSpace->StaticBody();
	cShape * shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( mWindow->GetWidth() / 2 , 150);

	shape = mSpace->AddShape( cShapeCircle::New( staticBody, 15.0f, emitterInstance.position ) );
	shape->Sensor( 1 );
	shape->CollisionType( BLOCKING_SENSOR_TYPE );
	shape->Data( &emitterInstance );

	// Create our catch sensor to requeue the balls when they reach the bottom of the screen
	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew(-4000, 600), cVectNew(4000, 600), 15.0f ) );
	shape->Sensor( 1 );
	shape->CollisionType( CATCH_SENSOR_TYPE );
	shape->Data( &emitterInstance );

	cSpace::cCollisionHandler handler;
	handler.a			= BLOCKING_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( &blockerBegin );
	handler.separate	= cb::Make3( &blockerSeparate );
	mSpace->AddCollisionHandler( handler );

	handler.Reset(); // Reset all the values and the callbacks ( set the callbacks as !IsSet()

	handler.a			= CATCH_SENSOR_TYPE;
	handler.b			= BALL_TYPE;
	handler.begin		= cb::Make3( &catcherBarBegin );
	mSpace->AddCollisionHandler( handler );
}

void Demo3Update() {
	if( !emitterInstance.blocked && emitterInstance.queue ){
		emitterInstance.queue--;

		cBody * body = mSpace->AddBody( cBody::New( 1.0f, Moment::ForCircle(1.0f, 15.0f, 0.0f, cVectZero ) ) );
		body->Pos( emitterInstance.position );
		body->Vel( cVectNew( Math::Randf(-1,1), Math::Randf(-1,1) ) * (cpFloat)100 );

		cShape *shape = mSpace->AddShape( cShapeCircle::New( body, 15.0f, cVectZero ) );
		shape->CollisionType( BALL_TYPE );
	}
}

void Demo3Destroy() {
	DestroyDemo();
}

enum {
	COLLIDE_STICK_SENSOR = 1
};

#define STICK_SENSOR_THICKNESS 2.5f

void PostStepAddJoint(cSpace *space, void *key, void *data)
{
	cConstraint *joint = (cConstraint *)key;
	space->AddConstraint( joint );
}

cpBool StickyPreSolve( cArbiter *arb, cSpace *space, void *data )
{
	// We want to fudge the collisions a bit to allow shapes to overlap more.
	// This simulates their squishy sticky surface, and more importantly
	// keeps them from separating and destroying the joint.

	// Track the deepest collision point and use that to determine if a rigid collision should occur.
	cpFloat deepest = INFINITY;

	// Grab the contact set and iterate over them.
	cpContactPointSet contacts = arb->ContactPointSet();

	for(int i=0; i<contacts.count; i++){
		// Increase the distance (negative means overlaping) of the
		// collision to allow them to overlap more.
		// This value is used only for fixing the positions of overlapping shapes.
		cpFloat dist = contacts.points[i].dist + 2.0f*STICK_SENSOR_THICKNESS;
		contacts.points[i].dist = eemin<eeFloat>(0.0f, dist);
		deepest = eemin<eeFloat>(deepest, dist);
	}

	// Set the new contact point data.
	arb->ContactPointSet( &contacts );

	// If the shapes are overlapping enough, then create a
	// joint that sticks them together at the first contact point.

	if(!arb->UserData() && deepest <= 0.0f){
		cBody *bodyA, *bodyB;
		arb->GetBodies( &bodyA, &bodyB );

		// Create a joint at the contact point to hold the body in place.
		cPivotJoint * joint = cpNew( cPivotJoint, ( bodyA, bodyB, tovect( contacts.points[0].point ) ) );

		// Dont draw the constraint
		joint->DrawPointSize( 0 );

		// Give it a finite force for the stickyness.
		joint->MaxForce( 3e3 );

		// Schedule a post-step() callback to add the joint.
		space->AddPostStepCallback( cb::Make3( &PostStepAddJoint ), joint, NULL );

		// Store the joint on the arbiter so we can remove it later.
		arb->UserData(joint);
	}

	// Position correction and velocity are handled separately so changing
	// the overlap distance alone won't prevent the collision from occuring.
	// Explicitly the collision for this frame if the shapes don't overlap using the new distance.
	return (deepest <= 0.0f);

	// Lots more that you could improve upon here as well:
	// * Modify the joint over time to make it plastic.
	// * Modify the joint in the post-step to make it conditionally plastic (like clay).
	// * Track a joint for the deepest contact point instead of the first.
	// * Track a joint for each contact point. (more complicated since you only get one data pointer).
}

void PostStepRemoveJoint(cSpace *space, void *key, void *data)
{
	cConstraint *joint = (cConstraint *)key;
	space->RemoveConstraint( joint );
	cConstraint::Free( joint );
}

void StickySeparate(cArbiter *arb, cSpace *space, void *data)
{
	cConstraint *joint = (cConstraint *)arb->UserData();

	if(joint){
		// The joint won't be removed until the step is done.
		// Need to disable it so that it won't apply itself.
		// Setting the force to 0 will do just that
		joint->MaxForce( 0.0f );

		// Perform the removal in a post-step() callback.
		space->AddPostStepCallback( cb::Make3( &PostStepRemoveJoint ), joint, NULL );

		// NULL out the reference to the joint.
		// Not required, but it's a good practice.
		arb->UserData( NULL );
	}
}

void Demo4Create() {
	cPhysicsManager::cDrawSpaceOptions * DSO = cPhysicsManager::instance()->GetDrawOptions();
	DSO->DrawBBs			= false;
	DSO->DrawShapes			= true;
	DSO->DrawShapesBorders	= false;
	DSO->CollisionPointSize	= 0;
	DSO->BodyPointSize		= 0;
	DSO->LineThickness		= 0;

	CreateJointAndBody();

	mWindow->Caption( "eepp - Physics - Sticky collisions using the cArbiter data pointer." );

	mSpace = cSpace::New();
	mSpace->Iterations( 10 );
	mSpace->Gravity( cVectNew( 0, 1000 ) );
	mSpace->CollisionSlop( 2.0 );

	cBody * staticBody = mSpace->StaticBody();
	cShape * shape;

	cpFloat x = 500;
	cpFloat y = 400;

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( x + -340, y -260 ), cVectNew( x -340, y + 260 ), 20.0f ) );
	shape->Elasticity( 1.0f );
	shape->Friction( 1.0f );
	shape->CollisionType( COLLIDE_STICK_SENSOR );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( x + 340, y -260 ), cVectNew( x + 340, y + 260 ), 20.0f ) );
	shape->Elasticity( 1.0f );
	shape->Friction( 1.0f );
	shape->CollisionType( COLLIDE_STICK_SENSOR );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( x -340, y -260 ), cVectNew( x + 340, y -260 ), 20.0f ) );
	shape->Elasticity( 1.0f );
	shape->Friction( 1.0f );
	shape->CollisionType( COLLIDE_STICK_SENSOR );
	shape->Layers( NOT_GRABABLE_MASK );

	shape = mSpace->AddShape( cShapeSegment::New( staticBody, cVectNew( x -340, y + 260 ), cVectNew( x + 340, y + 260 ), 20.0f ) );
	shape->Elasticity( 1.0f );
	shape->Friction( 1.0f );
	shape->CollisionType( COLLIDE_STICK_SENSOR );
	shape->Layers( NOT_GRABABLE_MASK );

	for(int i=0; i<200; i++){
		cpFloat mass = 0.15f;
		cpFloat radius = 10.0f;

		cBody * body = mSpace->AddBody( cBody::New( mass, Moment::ForCircle( mass, 0.0f, radius, cVectZero ) ) );
		body->Pos( cVectNew( x + easing::LinearInterpolation( Math::Randf(), -150.0f, 150.0f, 1 ),
							 y + easing::LinearInterpolation( Math::Randf(), -150.0f, 150.0f, 1 )
					) );

		cShape * shape = mSpace->AddShape( cShapeCircle::New( body, radius + STICK_SENSOR_THICKNESS, cVectZero ) );
		shape->Friction( 0.9f );
		shape->CollisionType( COLLIDE_STICK_SENSOR );
	}

	cSpace::cCollisionHandler c;
	c.a = COLLIDE_STICK_SENSOR;
	c.b = COLLIDE_STICK_SENSOR;
	c.preSolve = cb::Make3( &StickyPreSolve );
	c.separate = cb::Make3( &StickySeparate );

	mSpace->AddCollisionHandler( c );
}

void Demo4Update() {
}

void Demo4Destroy() {
	DestroyDemo();
}

void ChangeDemo( eeInt num ) {
	if ( num >= 0 && num < (eeInt)mDemo.size() && num != mCurDemo ) {
		if ( (eeInt)eeINDEX_NOT_FOUND != mCurDemo )
			mDemo[ mCurDemo ].destroy();

		mCurDemo = num;

		mDemo[ mCurDemo ].init();
	}
}

void PhysicsCreate() {
	// Initialize the physics engine
	cPhysicsManager::CreateSingleton();

	mDemo.clear();

	// Add the demos
	physicDemo demo;

	demo.init		= cb::Make0( &Demo1Create );
	demo.update		= cb::Make0( &Demo1Update );
	demo.destroy	= cb::Make0( &Demo1Destroy );
	mDemo.push_back( demo );

	demo.init		= cb::Make0( &Demo2Create );
	demo.update		= cb::Make0( &Demo2Update );
	demo.destroy	= cb::Make0( &Demo2Destroy );
	mDemo.push_back( demo );

	demo.init		= cb::Make0( &Demo3Create );
	demo.update		= cb::Make0( &Demo3Update );
	demo.destroy	= cb::Make0( &Demo3Destroy );
	mDemo.push_back( demo );

	demo.init		= cb::Make0( &Demo4Create );
	demo.update		= cb::Make0( &Demo4Update );
	demo.destroy	= cb::Make0( &Demo4Destroy );
	mDemo.push_back( demo );

	ChangeDemo( 0 );
}

void PhysicsUpdate() {
	// Creates a joint to drag any grabable object on the scene
	mMousePoint = cVectNew( KM->GetMousePosf().x, KM->GetMousePosf().y );
	cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last ), tocpv( mMousePoint ), 0.25 ) );
	mMouseBody->Pos( newPoint );
	mMouseBody->Vel( ( newPoint - mMousePoint_last ) * (cpFloat)mWindow->FPS() );
	mMousePoint_last = newPoint;

	if ( KM->MouseLeftPressed() ) {
		if ( NULL == mMouseJoint ) {
			cVect point = cVectNew( KM->GetMousePosf().x, KM->GetMousePosf().y );

			cShape * shape = mSpace->PointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if( NULL != shape ){
				mMouseJoint = eeNew( cPivotJoint, ( mMouseBody, shape->Body(), cVectZero, shape->Body()->World2Local( point ) ) );

				mMouseJoint->MaxForce( 50000.0f );
				mSpace->AddConstraint( mMouseJoint );
			}
		}
	} else if ( NULL != mMouseJoint ) {
		mSpace->RemoveConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}

	mDemo[ mCurDemo ].update();
	mSpace->Update();
	mSpace->Draw();
}

void PhysicsDestroy() {
	mDemo[ mCurDemo ].destroy();
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	mWindow = cEngine::instance()->CreateWindow( WindowSettings( 1024, 768, "eepp - Physics" ), ContextSettings( true ) );

	if ( mWindow->Created() ) {
		KM = mWindow->GetInput();

		mWindow->BackColor( eeColor( 255, 255, 255 ) );

		PhysicsCreate();

		while ( mWindow->Running() ) {
			KM->Update();

			if ( KM->IsKeyDown( KEY_ESCAPE ) ) {
				mWindow->Close();
			}

			PhysicsUpdate();

			if ( KM->IsKeyUp( KEY_LEFT ) ) {
				ChangeDemo( mCurDemo - 1 );
			} else if ( KM->IsKeyUp( KEY_RIGHT ) ) {
				ChangeDemo( mCurDemo + 1 );
			}

			mWindow->Display();
		}

		PhysicsDestroy();
	}

	cEngine::DestroySingleton();

	EE::MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
