#include <eepp/ee.hpp>
#include <eepp/physics/physics.hpp>
using namespace EE::Physics;

/**
The physics module is a OOP wrapper for Chipmunk Physics.
To understand the conceptos of space, body, shapes, etc you can read the
Chipmunk documentation:
http://chipmunk-physics.net/release/ChipmunkLatest-Docs/
*/
typedef std::function<void()> SceneCb;

struct physicDemo {
	SceneCb init;
	SceneCb update;
	SceneCb destroy;
};

std::vector<physicDemo> mDemo;
int mCurDemo = eeINDEX_NOT_FOUND;
Space* mSpace;
Body* mMouseBody;
cVect mMousePoint;
cVect mMousePoint_last;
Constraint* mMouseJoint;

#define GRABABLE_MASK_BIT ( 1 << 31 )
#define NOT_GRABABLE_MASK ( ~GRABABLE_MASK_BIT )

void createJointAndBody() {
	mMouseJoint = NULL;
	mMouseBody = Body::New( INFINITY, INFINITY );
}

EE::Window::Window* mWindow;
Input* KM;

void defaultDrawOptions() {
	PhysicsManager::DrawSpaceOptions* DSO = PhysicsManager::instance()->getDrawOptions();
	DSO->DrawBBs = false;
	DSO->DrawShapes = true;
	DSO->DrawShapesBorders = true;
	DSO->CollisionPointSize = 4;
	DSO->BodyPointSize = 0;
	DSO->LineThickness = 1;
}

void destroyDemo() {
	eeSAFE_DELETE( mMouseBody );
	eeSAFE_DELETE( mSpace );
}

static const int image_width = 188;
static const int image_height = 35;
static const int image_row_length = 24;

static const char image_bitmap[] = {
	15,	  -16,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   7,	 -64, 15,  63,	-32, -2,  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	31,	  -64,	15,	  127, -125, -1,
	-128, 0,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  127,	-64,  15,  127,	 15,  -1,  -64, 0,	 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   0,	 0,	  0,   1,	-1,	 -64, 15,	-2,	  31,	-1,	  -64, 0,	 0,
	0,	  0,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	0,	  0,	0,	  1,   -1,	 -64,
	0,	  -4,	63,	  -1,  -32,	 0,	  0,   0,	0,	 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   1,	 -1,  -64, 15,	-8,	 127, -1,	-32,  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	1,	  -1,	-64,  0,   -8,	 -15,
	-1,	  -32,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	1,	  -31,	-1,	  -64, 15,	 -8,  -32, -1,	-32, 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   0,	 0,	  0,   7,	-15, -1,  -64,	9,	  -15,	-32,  -1,  -32,	 0,
	0,	  0,	0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,	0,	  0,	0,	  31,  -15,	 -1,
	-64,  0,	-15,  -32, -1,	 -32, 0,   0,	0,	 0,	  0,	0,	  0,	0,	  0,   0,	 0,
	0,	  0,	0,	  0,   63,	 -7,  -1,  -64, 9,	 -29, -32,	127,  -61,	-16,  63,  15,	 -61,
	-1,	  -8,	31,	  -16, 15,	 -8,  126, 7,	-31, -8,  31,	-65,  -7,	-1,	  -64, 9,	 -29,
	-32,  0,	7,	  -8,  127,	 -97, -25, -1,	-2,	 63,  -8,	31,	  -4,	-1,	  15,  -13,	 -4,
	63,	  -1,	-3,	  -1,  -64,	 9,	  -29, -32, 0,	 7,	  -8,	127,  -97,	-25,  -1,  -2,	 63,
	-8,	  31,	-4,	  -1,  15,	 -13, -2,  63,	-1,	 -3,  -1,	-64,  9,	-29,  -32, 0,	 7,
	-8,	  127,	-97,  -25, -1,	 -1,  63,  -4,	63,	 -4,  -1,	15,	  -13,	-2,	  63,  -33,	 -1,
	-1,	  -32,	9,	  -25, -32,	 0,	  7,   -8,	127, -97, -25,	-1,	  -1,	63,	  -4,  63,	 -4,
	-1,	  15,	-13,  -1,  63,	 -33, -1,  -1,	-16, 9,	  -25,	-32,  0,	7,	  -8,  127,	 -97,
	-25,  -1,	-1,	  63,  -4,	 63,  -4,  -1,	15,	 -13, -1,	63,	  -49,	-1,	  -1,  -8,	 9,
	-57,  -32,	0,	  7,   -8,	 127, -97, -25, -8,	 -1,  63,	-2,	  127,	-4,	  -1,  15,	 -13,
	-1,	  -65,	-49,  -1,  -1,	 -4,  9,   -57, -32, 0,	  7,	-8,	  127,	-97,  -25, -8,	 -1,
	63,	  -2,	127,  -4,  -1,	 15,  -13, -1,	-65, -57, -1,	-1,	  -2,	9,	  -57, -32,	 0,
	7,	  -8,	127,  -97, -25,	 -8,  -1,  63,	-2,	 127, -4,	-1,	  15,	-13,  -1,  -1,	 -57,
	-1,	  -1,	-1,	  9,   -57,	 -32, 0,   7,	-1,	 -1,  -97,	-25,  -8,	-1,	  63,  -1,	 -1,
	-4,	  -1,	15,	  -13, -1,	 -1,  -61, -1,	-1,	 -1,  -119, -57,  -32,	0,	  7,   -1,	 -1,
	-97,  -25,	-8,	  -1,  63,	 -1,  -1,  -4,	-1,	 15,  -13,	-1,	  -1,	-61,  -1,  -1,	 -1,
	-55,  -49,	-32,  0,   7,	 -1,  -1,  -97, -25, -8,  -1,	63,	  -1,	-1,	  -4,  -1,	 15,
	-13,  -1,	-1,	  -63, -1,	 -1,  -1,  -23, -49, -32, 127,	-57,  -1,	-1,	  -97, -25,	 -1,
	-1,	  63,	-1,	  -1,  -4,	 -1,  15,  -13, -1,	 -1,  -63,	-1,	  -1,	-1,	  -16, -49,	 -32,
	-1,	  -25,	-1,	  -1,  -97,	 -25, -1,  -1,	63,	 -33, -5,	-4,	  -1,	15,	  -13, -1,	 -1,
	-64,  -1,	-9,	  -1,  -7,	 -49, -32, -1,	-25, -8,  127,	-97,  -25,	-1,	  -1,  63,	 -33,
	-5,	  -4,	-1,	  15,  -13,	 -1,  -1,  -64, -1,	 -13, -1,	-32,  -49,	-32,  -1,  -25,	 -8,
	127,  -97,	-25,  -1,  -2,	 63,  -49, -13, -4,	 -1,  15,	-13,  -1,	-1,	  -64, 127,	 -7,
	-1,	  -119, -17,  -15, -1,	 -25, -8,  127, -97, -25, -1,	-2,	  63,	-49,  -13, -4,	 -1,
	15,	  -13,	-3,	  -1,  -64,	 127, -8,  -2,	15,	 -17, -1,	-1,	  -25,	-8,	  127, -97,	 -25,
	-1,	  -8,	63,	  -49, -13,	 -4,  -1,  15,	-13, -3,  -1,	-64,  63,	-4,	  120, 0,	 -17,
	-1,	  -1,	-25,  -8,  127,	 -97, -25, -8,	0,	 63,  -57,	-29,  -4,	-1,	  15,  -13,	 -4,
	-1,	  -64,	63,	  -4,  0,	 15,  -17, -1,	-1,	 -25, -8,	127,  -97,	-25,  -8,  0,	 63,
	-57,  -29,	-4,	  -1,  -1,	 -13, -4,  -1,	-64, 31,  -2,	0,	  0,	103,  -1,  -1,	 -57,
	-8,	  127,	-97,  -25, -8,	 0,	  63,  -57, -29, -4,  -1,	-1,	  -13,	-4,	  127, -64,	 31,
	-2,	  0,	15,	  103, -1,	 -1,  -57, -8,	127, -97, -25,	-8,	  0,	63,	  -61, -61,	 -4,
	127,  -1,	-29,  -4,  127,	 -64, 15,  -8,	0,	 0,	  55,	-1,	  -1,	-121, -8,  127,	 -97,
	-25,  -8,	0,	  63,  -61,	 -61, -4,  127, -1,	 -29, -4,	63,	  -64,	15,	  -32, 0,	 0,
	23,	  -1,	-2,	  3,   -16,	 63,  15,  -61, -16, 0,	  31,	-127, -127, -8,	  31,  -1,	 -127,
	-8,	  31,	-128, 7,   -128, 0,	  0 };

int get_pixel( int x, int y ) {
	return ( image_bitmap[( x >> 3 ) + y * image_row_length] >> ( ~x & 0x7 ) ) & 1;
}

Shape* make_ball( cpFloat x, cpFloat y ) {
	Body* body = Body::New( 1.0, INFINITY );

	body->setPos( cVectNew( x, y ) );

	ShapePoint* shape = ShapePoint::New( body, 0.95, cVectZero );

	shape->setDrawRadius( 4 );
	shape->setElasticity( 0.0 );
	shape->setFriction( 0.0 );

	return shape;
}

static int bodyCount = 0;

void demo1Create() {
	defaultDrawOptions();

	createJointAndBody();

	mWindow->setTitle( "eepp - Physics - Logo Smash" );

	mSpace = Physics::Space::New();
	mSpace->setIterations( 1 );

	// The space will contain a very large number of similary sized objects.
	// This is the perfect candidate for using the spatial hash.
	// Generally you will never need to do this.
	mSpace->useSpatialHash( 2.0, 10000 );

	bodyCount = 0;

	Body* body;
	Shape* shape;

	Float pX = mWindow->getWidth() / 2 - ( image_width * 4 ) / 2;
	Float pY = mWindow->getHeight() / 2 - ( image_height * 4 ) / 2;

	for ( int y = 0; y < image_height; y++ ) {
		for ( int x = 0; x < image_width; x++ ) {
			if ( !get_pixel( x, y ) )
				continue;

			shape = make_ball( pX + x * 4, pY + y * 4 );

			mSpace->addBody( shape->getBody() );
			mSpace->addShape( shape );

			bodyCount++;
		}
	}

	body = mSpace->addBody( Body::New( INFINITY, INFINITY ) );
	body->setPos( cVectNew( 0, mWindow->getHeight() / 2 + 16 ) );
	body->setVel( cVectNew( 400, 0 ) );

	shape = mSpace->addShape( ShapeCircle::New( body, 8.0f, cVectZero ) );
	shape->setElasticity( 0.0 );
	shape->setFriction( 0.0 );
	shape->setLayers( NOT_GRABABLE_MASK );

	bodyCount++;
}

void demo1Update() {}

void demo1Destroy() {
	destroyDemo();
}

void demo2Create() {
	defaultDrawOptions();

	createJointAndBody();

	mWindow->setTitle( "eepp - Physics - Pyramid Stack" );

	Shape::resetShapeIdCounter();

	mSpace = Physics::Space::New();
	mSpace->setGravity( cVectNew( 0, 100 ) );
	mSpace->setSleepTimeThreshold( 0.5f );

	Body *body, *statiBody = mSpace->getStaticBody();
	Shape* shape;

	shape = mSpace->addShape(
		ShapeSegment::New( statiBody, cVectNew( 0, mWindow->getHeight() ),
						   cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape(
		ShapeSegment::New( statiBody, cVectNew( mWindow->getWidth(), 0 ),
						   cVectNew( mWindow->getWidth(), mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ),
												 cVectNew( 0, mWindow->getHeight() ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( 0, 0 ),
												 cVectNew( mWindow->getWidth(), 0 ), 0.0f ) );
	shape->setE( 1.0f );
	shape->setU( 1.0f );
	shape->setLayers( NOT_GRABABLE_MASK );

	Float hw = mWindow->getWidth() / 2;

	for ( int i = 0; i < 14; i++ ) {
		for ( int j = 0; j <= i; j++ ) {
			body = mSpace->addBody( Body::New( 1.0f, Moment::forBox( 1.0f, 30.0f, 30.0f ) ) );
			body->setPos( cVectNew( hw + j * 32 - i * 16, 100 + i * 32 ) );

			shape = mSpace->addShape( ShapePoly::New( body, 30.f, 30.f ) );
			shape->setE( 0.0f );
			shape->setU( 0.8f );
		}
	}

	cpFloat radius = 15.0f;

	body =
		mSpace->addBody( Body::New( 10.0f, Moment::forCircle( 10.0f, 0.0f, radius, cVectZero ) ) );
	body->setPos( cVectNew( hw, mWindow->getHeight() - radius - 5 ) );

	shape = mSpace->addShape( ShapeCircle::New( body, radius, cVectZero ) );
	shape->setE( 0.0f );
	shape->setU( 0.9f );
}

void demo2Update() {}

void demo2Destroy() {
	destroyDemo();
}

enum CollisionTypes { BALL_TYPE, BLOCKING_SENSOR_TYPE, CATCH_SENSOR_TYPE };

struct Emitter {
	int queue;
	int blocked;
	cVect position;
};
Emitter emitterInstance;

cpBool blockerBegin( Arbiter* arb, Space*, void* ) {
	Shape *a, *b;
	arb->getShapes( &a, &b );

	Emitter* emitter = (Emitter*)a->getData();

	emitter->blocked++;

	return cpFalse; // Return values from sensors callbacks are ignored,
}

void blockerSeparate( Arbiter* arb, Space*, void* ) {
	Shape *a, *b;
	arb->getShapes( &a, &b );

	Emitter* emitter = (Emitter*)a->getData();

	emitter->blocked--;
}

void postStepRemove( Space* space, void* tshape, void* ) {
	Shape* shape = reinterpret_cast<Shape*>( tshape );

	if ( NULL != mMouseJoint &&
		 ( mMouseJoint->getA() == shape->getBody() || mMouseJoint->getB() == shape->getBody() ) ) {
		space->removeConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}

	space->removeBody( shape->getBody() );
	space->removeShape( shape );

	Shape::Free( shape, true );
}

cpBool catcherBarBegin( Arbiter* arb, Space* space, void* ) {
	Shape *a, *b;
	arb->getShapes( &a, &b );

	Emitter* emitter = (Emitter*)a->getData();

	emitter->queue++;

	space->addPostStepCallback( &postStepRemove, b, NULL );

	return cpFalse;
}

void demo3Create() {
	defaultDrawOptions();

	createJointAndBody();

	mWindow->setTitle( "eepp - Physics - Sensor" );

	Shape::resetShapeIdCounter();

	mSpace = Physics::Space::New();
	mSpace->setIterations( 10 );
	mSpace->setGravity( cVectNew( 0, 100 ) );

	Body* statiBody = mSpace->getStaticBody();
	Shape* shape;

	emitterInstance.queue = 5;
	emitterInstance.blocked = 0;
	emitterInstance.position = cVectNew( mWindow->getWidth() / 2, 150 );

	shape = mSpace->addShape( ShapeCircle::New( statiBody, 15.0f, emitterInstance.position ) );
	shape->setSensor( 1 );
	shape->setCollisionType( BLOCKING_SENSOR_TYPE );
	shape->setData( &emitterInstance );

	// Create our catch sensor to requeue the balls when they reach the bottom of the screen
	shape = mSpace->addShape(
		ShapeSegment::New( statiBody, cVectNew( -4000, 600 ), cVectNew( 4000, 600 ), 15.0f ) );
	shape->setSensor( 1 );
	shape->setCollisionType( CATCH_SENSOR_TYPE );
	shape->setData( &emitterInstance );

	Space::CollisionHandler handler;
	handler.a = BLOCKING_SENSOR_TYPE;
	handler.b = BALL_TYPE;
	handler.begin = &blockerBegin;
	handler.separate = &blockerSeparate;
	mSpace->addCollisionHandler( handler );

	handler.reset(); // Reset all the values and the callbacks ( set the callbacks as !IsSet()

	handler.a = CATCH_SENSOR_TYPE;
	handler.b = BALL_TYPE;
	handler.begin = &catcherBarBegin;
	mSpace->addCollisionHandler( handler );
}

void demo3Update() {
	if ( !emitterInstance.blocked && emitterInstance.queue ) {
		emitterInstance.queue--;

		Body* body =
			mSpace->addBody( Body::New( 1.0f, Moment::forCircle( 1.0f, 15.0f, 0.0f, cVectZero ) ) );
		body->setPos( emitterInstance.position );
		body->setVel( cVectNew( Math::randf( -1, 1 ), Math::randf( -1, 1 ) ) * (cpFloat)100 );

		Shape* shape = mSpace->addShape( ShapeCircle::New( body, 15.0f, cVectZero ) );
		shape->setCollisionType( BALL_TYPE );
	}
}

void demo3Destroy() {
	destroyDemo();
}

enum { COLLIDE_STICK_SENSOR = 1 };

#define STICK_SENSOR_THICKNESS 2.5f

void postStepAddJoint( Space* space, void* key, void* ) {
	Constraint* joint = (Constraint*)key;
	space->addConstraint( joint );
}

cpBool stickyPreSolve( Arbiter* arb, Space* space, void* ) {
	// We want to fudge the collisions a bit to allow shapes to overlap more.
	// This simulates their squishy sticky surface, and more importantly
	// keeps them from separating and destroying the joint.

	// Track the deepest collision point and use that to determine if a rigid collision should
	// occur.
	cpFloat deepest = INFINITY;

	// Grab the contact set and iterate over them.
	cpContactPointSet contacts = arb->getContactPointSet();

	for ( int i = 0; i < contacts.count; i++ ) {
		// Increase the distance (negative means overlaping) of the
		// collision to allow them to overlap more.
		// This value is used only for fixing the positions of overlapping shapes.
		cpFloat dist = contacts.points[i].dist + 2.0f * STICK_SENSOR_THICKNESS;
		contacts.points[i].dist = eemin<Float>( 0.0f, dist );
		deepest = eemin<Float>( deepest, dist );
	}

	// Set the new contact point data.
	arb->setContactPointSet( &contacts );

	// If the shapes are overlapping enough, then create a
	// joint that sticks them together at the first contact point.

	if ( !arb->getUserData() && deepest <= 0.0f ) {
		Body *bodyA, *bodyB;
		arb->getBodies( &bodyA, &bodyB );

		// Create a joint at the contact point to hold the body in place.
		PivotJoint* joint =
			eeNew( PivotJoint, ( bodyA, bodyB, tovect( contacts.points[0].point ) ) );

		// Dont draw the constraint
		joint->setDrawPointSize( 0 );

		// Give it a finite force for the stickyness.
		joint->setMaxForce( 3e3 );

		// Schedule a post-step() callback to add the joint.
		space->addPostStepCallback( &postStepAddJoint, joint, NULL );

		// Store the joint on the arbiter so we can remove it later.
		arb->setUserData( joint );
	}

	// Position correction and velocity are handled separately so changing
	// the overlap distance alone won't prevent the collision from occuring.
	// Explicitly the collision for this frame if the shapes don't overlap using the new distance.
	return ( deepest <= 0.0f );

	// Lots more that you could improve upon here as well:
	// * Modify the joint over time to make it plastic.
	// * Modify the joint in the post-step to make it conditionally plastic (like clay).
	// * Track a joint for the deepest contact point instead of the first.
	// * Track a joint for each contact point. (more complicated since you only get one data
	// pointer).
}

void postStepRemoveJoint( Space* space, void* key, void* ) {
	Constraint* joint = (Constraint*)key;
	space->removeConstraint( joint );
	Constraint::Free( joint );
}

void stickySeparate( Arbiter* arb, Space* space, void* ) {
	Constraint* joint = (Constraint*)arb->getUserData();

	if ( joint ) {
		// The joint won't be removed until the step is done.
		// Need to disable it so that it won't apply itself.
		// Setting the force to 0 will do just that
		joint->setMaxForce( 0.0f );

		// Perform the removal in a post-step() callback.
		space->addPostStepCallback( &postStepRemoveJoint, joint, NULL );

		// NULL out the reference to the joint.
		// Not required, but it's a good practice.
		arb->setUserData( NULL );
	}
}

void demo4Create() {
	PhysicsManager::DrawSpaceOptions* DSO = PhysicsManager::instance()->getDrawOptions();
	DSO->DrawBBs = false;
	DSO->DrawShapes = true;
	DSO->DrawShapesBorders = false;
	DSO->CollisionPointSize = 0;
	DSO->BodyPointSize = 0;
	DSO->LineThickness = 0;

	createJointAndBody();

	mWindow->setTitle( "eepp - Physics - Sticky collisions using the Arbiter data pointer." );

	mSpace = Space::New();
	mSpace->setIterations( 10 );
	mSpace->setGravity( cVectNew( 0, 1000 ) );
	mSpace->setCollisionSlop( 2.0 );

	Body* statiBody = mSpace->getStaticBody();
	Shape* shape;

	cpFloat x = 500;
	cpFloat y = 400;

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( x + -340, y - 260 ),
												 cVectNew( x - 340, y + 260 ), 20.0f ) );
	shape->setElasticity( 1.0f );
	shape->setFriction( 1.0f );
	shape->setCollisionType( COLLIDE_STICK_SENSOR );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( x + 340, y - 260 ),
												 cVectNew( x + 340, y + 260 ), 20.0f ) );
	shape->setElasticity( 1.0f );
	shape->setFriction( 1.0f );
	shape->setCollisionType( COLLIDE_STICK_SENSOR );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( x - 340, y - 260 ),
												 cVectNew( x + 340, y - 260 ), 20.0f ) );
	shape->setElasticity( 1.0f );
	shape->setFriction( 1.0f );
	shape->setCollisionType( COLLIDE_STICK_SENSOR );
	shape->setLayers( NOT_GRABABLE_MASK );

	shape = mSpace->addShape( ShapeSegment::New( statiBody, cVectNew( x - 340, y + 260 ),
												 cVectNew( x + 340, y + 260 ), 20.0f ) );
	shape->setElasticity( 1.0f );
	shape->setFriction( 1.0f );
	shape->setCollisionType( COLLIDE_STICK_SENSOR );
	shape->setLayers( NOT_GRABABLE_MASK );

	for ( int i = 0; i < 200; i++ ) {
		cpFloat mass = 0.15f;
		cpFloat radius = 10.0f;

		Body* body = mSpace->addBody(
			Body::New( mass, Moment::forCircle( mass, 0.0f, radius, cVectZero ) ) );
		body->setPos(
			cVectNew( x + easing::linearInterpolation( Math::randf(), -150.0f, 150.0f, 1 ),
					  y + easing::linearInterpolation( Math::randf(), -150.0f, 150.0f, 1 ) ) );

		Shape* shape = mSpace->addShape(
			ShapeCircle::New( body, radius + STICK_SENSOR_THICKNESS, cVectZero ) );
		shape->setFriction( 0.9f );
		shape->setCollisionType( COLLIDE_STICK_SENSOR );
	}

	Space::CollisionHandler c;
	c.a = COLLIDE_STICK_SENSOR;
	c.b = COLLIDE_STICK_SENSOR;
	c.preSolve = &stickyPreSolve;
	c.separate = &stickySeparate;

	mSpace->addCollisionHandler( c );
}

void demo4Update() {}

void demo4Destroy() {
	destroyDemo();
}

void ChangeDemo( int num ) {
	if ( num >= 0 && num < (int)mDemo.size() && num != mCurDemo ) {
		if ( (int)eeINDEX_NOT_FOUND != mCurDemo )
			mDemo[mCurDemo].destroy();

		mCurDemo = num;

		mDemo[mCurDemo].init();
	}
}

void physicsCreate() {
	// Initialize the physics engine
	PhysicsManager::createSingleton();

	mDemo.clear();

	// Add the demos
	physicDemo demo;

	demo.init = &demo1Create;
	demo.update = &demo1Update;
	demo.destroy = &demo1Destroy;
	mDemo.push_back( demo );

	demo.init = &demo2Create;
	demo.update = &demo2Update;
	demo.destroy = &demo2Destroy;
	mDemo.push_back( demo );

	demo.init = &demo3Create;
	demo.update = &demo3Update;
	demo.destroy = &demo3Destroy;
	mDemo.push_back( demo );

	demo.init = &demo4Create;
	demo.update = &demo4Update;
	demo.destroy = &demo4Destroy;
	mDemo.push_back( demo );

	ChangeDemo( 0 );
}

void physicsUpdate() {
	// Creates a joint to drag any grabable object on the scene
	const Vector2f mousePos( KM->getMousePos().asFloat() );
	mMousePoint = cVectNew( mousePos.x, mousePos.y );
	cVect newPoint = tovect( cpvlerp( tocpv( mMousePoint_last ), tocpv( mMousePoint ), 0.25 ) );
	mMouseBody->setPos( newPoint );
	mMouseBody->setVel( ( newPoint - mMousePoint_last ) * (cpFloat)mWindow->getFPS() );
	mMousePoint_last = newPoint;

	if ( KM->isMouseLeftPressed() ) {
		if ( NULL == mMouseJoint ) {
			cVect point = cVectNew( mousePos.x, mousePos.y );

			Shape* shape = mSpace->pointQueryFirst( point, GRABABLE_MASK_BIT, CP_NO_GROUP );

			if ( NULL != shape ) {
				mMouseJoint = eeNew( PivotJoint, ( mMouseBody, shape->getBody(), cVectZero,
												   shape->getBody()->world2Local( point ) ) );

				mMouseJoint->setMaxForce( 50000.0f );
				mSpace->addConstraint( mMouseJoint );
			}
		}
	} else if ( NULL != mMouseJoint ) {
		mSpace->removeConstraint( mMouseJoint );
		eeSAFE_DELETE( mMouseJoint );
	}

	mDemo[mCurDemo].update();
	mSpace->update();
	mSpace->draw();
}

void physicsDestroy() {
	mDemo[mCurDemo].destroy();
}

void mainLoop() {
	mWindow->clear();

	KM->update();

	if ( KM->isKeyDown( KEY_ESCAPE ) ) {
		mWindow->close();
	}

	physicsUpdate();

	if ( KM->isKeyUp( KEY_LEFT ) || KM->isKeyUp( KEY_A ) ) {
		ChangeDemo( mCurDemo - 1 );
	} else if ( KM->isKeyUp( KEY_RIGHT ) || KM->isKeyUp( KEY_D ) ) {
		ChangeDemo( mCurDemo + 1 );
	}

	mWindow->display();
}

EE_MAIN_FUNC int main( int, char*[] ) {
	mWindow = Engine::instance()->createWindow( WindowSettings( 1024, 768, "eepp - Physics" ),
												ContextSettings( true, EE::Graphics::GLv_ES2 ) );

	if ( mWindow->isOpen() ) {
		KM = mWindow->getInput();

		mWindow->setClearColor( RGB( 255, 255, 255 ) );

		physicsCreate();

		mWindow->runMainLoop( &mainLoop );

		physicsDestroy();
	}

	PhysicsManager::destroySingleton();

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
