#ifndef EE_PHYSICS_CSPACE_HPP
#define EE_PHYSICS_CSPACE_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/body.hpp>
#include <eepp/physics/shape.hpp>
#include <eepp/physics/arbiter.hpp>
#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API Space {
	public:
		typedef cb::Callback3<int	, Arbiter *, Space *	, void * >				CollisionBeginFunc;
		typedef cb::Callback3<int	, Arbiter *, Space *	, void * >				CollisionPreSolveFunc;
		typedef cb::Callback3<void	, Arbiter *, Space *	, void * >				CollisionPostSolveFunc;
		typedef cb::Callback3<void	, Arbiter *, Space *	, void * >				CollisionSeparateFunc;
		typedef cb::Callback3<void	, Space *	, void *	, void * >				PostStepCallback;
		typedef cb::Callback2<void	, Shape *	, void * >							BBQueryFunc;
		typedef cb::Callback4<void	, Shape *	, cpFloat	, cVect	, void * >		SegmentQueryFunc;
		typedef cb::Callback2<void	, Shape *	, void * >							PointQueryFunc;
		typedef cb::Callback3<void	, Space *	, Body *	, void * >				BodyIteratorFunc;
		typedef cb::Callback3<void	, Space *	, Shape *	, void * >				ShapeIteratorFunc;

		class CollisionHandler {
			public:
				CollisionHandler() :
					a( 0 ),
					b( 0  ),
					data( NULL )
				{
				}

				inline void Reset() {
					a			= 0;
					b			= 0;
					data		= NULL;
					begin		= CollisionBeginFunc();
					preSolve	= CollisionPreSolveFunc();
					postSolve	= CollisionPostSolveFunc();
					separate	= CollisionSeparateFunc();
				}

				cpCollisionType a;
				cpCollisionType b;
				CollisionBeginFunc begin;
				CollisionPreSolveFunc preSolve;
				CollisionPostSolveFunc postSolve;
				CollisionSeparateFunc separate;
				void * data;
		};

		class PostStepCallbackCont {
			public:
				PostStepCallbackCont() :
					Data( NULL )
				{}

				PostStepCallback	Callback;
				void *				Data;
		};

		class BBQuery {
			public:
				BBQuery() :
					Data( NULL )
				{}

				Physics::Space *	Space;
				BBQueryFunc			Func;
				void *				Data;
		};

		class SegmentQuery {
			public:
				SegmentQuery()
				{}

				Physics::Space *	Space;
				SegmentQueryFunc	Func;
				void *				Data;
		};

		class PointQuery {
			public:
				PointQuery() :
					Data( NULL )
				{}

				Physics::Space *	Space;
				PointQueryFunc		Func;
				void *				Data;
		};

		class BodyIterator {
			public:
				BodyIterator( Physics::Space * space, void * data, BodyIteratorFunc func ) :
					Space( space ),
					Data( data ),
					Func( func )
				{}

				Physics::Space *		Space;
				void *					Data;
				BodyIteratorFunc		Func;
		};

		class ShapeIterator {
			public:
				ShapeIterator( Physics::Space * space, void * data, ShapeIteratorFunc func ) :
					Space( space ),
					Data( data ),
					Func( func )
				{}

				Physics::Space *			Space;
				void *						Data;
				ShapeIteratorFunc			Func;
		};

		static Space * New();

		static void Free( Space * space );

		Space();

		virtual ~Space();

		void step( const cpFloat& dt );

		void update();

		Body * getStaticBody() const;

		const int& getIterations() const;

		void setIterations( const int& iterations );

		cVect getGravity() const;

		void setGravity( const cVect& gravity );

		const cpFloat& getDamping() const;

		void setDamping( const cpFloat& damping );

		const cpFloat& getIdleSpeedThreshold() const;

		void setIdleSpeedThreshold( const cpFloat& idleSpeedThreshold );

		const cpFloat& getSleepTimeThreshold() const;

		void setSleepTimeThreshold( const cpFloat& sleepTimeThreshold );

		void setCollisionSlop( cpFloat slop );

		cpFloat getCollisionSlop() const;

		void setCollisionBias( cpFloat bias );

		cpFloat getCollisionBias() const;

		cpTimestamp getCollisionPersistence();

		void setCollisionPersistence( cpTimestamp value );

		bool getEnableContactGraph();

		void setEnableContactGraph( bool value );

		bool contains( Shape * shape );

		bool contains( Body * body );

		bool contains( Constraint * constraint );

		Shape * addShape( Shape * shape );

		Shape * addStaticShape( Shape *shape );

		Body * addBody( Body * body );

		Constraint * addConstraint( Constraint * constraint );

		void removeShape( Shape * shape );

		void removeStatiShape( Shape * shape );

		void removeBody( Body * body );

		void removeConstraint( Constraint * constraint );

		cpSpace * getSpace() const;

		void activateShapesTouchingShape( Shape * shape );

		virtual void draw();

		Shape * pointQueryFirst( cVect point, cpLayers layers, cpGroup group );

		Shape * segmentQueryFirst( cVect start, cVect end, cpLayers layers, cpGroup group, cpSegmentQueryInfo * out );

		void addCollisionHandler( const CollisionHandler& handler );

		void removeCollisionHandler( cpCollisionType a, cpCollisionType b );

		void setDefaultCollisionHandler( const CollisionHandler& handler );

		void addPostStepCallback( PostStepCallback postStep, void * obj, void * data );

		virtual cpBool onCollisionBegin( Arbiter * arb, void * data );

		virtual cpBool onCollisionPreSolve( Arbiter * arb, void * data );

		virtual void onCollisionPostSolve( Arbiter * arb, void * data );

		virtual void onCollisionSeparate( Arbiter * arb, void * data );

		virtual void onPostStepCallback( void * obj, void * data );

		virtual void onBBQuery( Shape * shape, BBQuery * query );

		virtual void onSegmentQuery( Shape * shape, cpFloat t, cVect n , SegmentQuery * query );

		virtual void onPointQuery( Shape * shape, PointQuery * query );

		void bbQuery( cBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void * data );

		void segmentQuery( cVect start, cVect end, cpLayers layers, cpGroup group, SegmentQueryFunc func, void * data );

		void pointQuery( cVect point, cpLayers layers, cpGroup group, PointQueryFunc func, void * data );

		void setData( void * data );

		void * getData() const;

		void reindexShape( Shape * shape );

		void reindexShapesForBody( Body *body );

		void reindexStatic();

		void useSpatialHash( cpFloat dim, int count );

		void eachShape( ShapeIteratorFunc Func, void * data );

		virtual void onEachShape( Shape * Shape, ShapeIterator * it );

		void eachBody( BodyIteratorFunc Func, void * data );

		virtual void onEachBody( Body * Body, BodyIterator * it );

		void convertBodyToDynamic( Body * body, cpFloat mass, cpFloat moment );

		void convertBodyToStatic( Body * body );
	protected:
		cpSpace *									mSpace;
		Body *										mStatiBody;
		void *										mData;
		std::list<Body*>							mBodys;
		std::list<Shape*>							mShapes;
		std::list<Constraint*>						mConstraints;
		std::map< cpHashValue, CollisionHandler >	mCollisions;
		CollisionHandler							mCollisionsDefault;
		std::list< PostStepCallbackCont* >			mPostStepCallbacks;
};

CP_NAMESPACE_END

#endif
