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

		class cCollisionHandler {
			public:
				cCollisionHandler() :
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

		class cPostStepCallback {
			public:
				cPostStepCallback() :
					Data( NULL )
				{}

				PostStepCallback	Callback;
				void *				Data;
		};

		class cBBQuery {
			public:
				cBBQuery() :
					Data( NULL )
				{}

				Physics::Space *	Space;
				BBQueryFunc			Func;
				void *				Data;
		};

		class cSegmentQuery {
			public:
				cSegmentQuery()
				{}

				Physics::Space *	Space;
				SegmentQueryFunc	Func;
				void *				Data;
		};

		class cPointQuery {
			public:
				cPointQuery() :
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

		void Step( const cpFloat& dt );

		void Update();

		Body * StatiBody() const;

		const int& Iterations() const;

		void Iterations( const int& iterations );

		cVect Gravity() const;

		void Gravity( const cVect& gravity );

		const cpFloat& Damping() const;

		void Damping( const cpFloat& damping );

		const cpFloat& IdleSpeedThreshold() const;

		void IdleSpeedThreshold( const cpFloat& idleSpeedThreshold );

		const cpFloat& SleepTimeThreshold() const;

		void SleepTimeThreshold( const cpFloat& sleepTimeThreshold );

		void CollisionSlop( cpFloat slop );

		cpFloat CollisionSlop() const;

		void CollisionBias( cpFloat bias );

		cpFloat CollisionBias() const;

		cpTimestamp CollisionPersistence();

		void CollisionPersistence( cpTimestamp value );

		bool EnableContactGraph();

		void EnableContactGraph( bool value );

		bool Contains( Shape * shape );

		bool Contains( Body * body );

		bool Contains( Constraint * constraint );

		Shape * AddShape( Shape * shape );

		Shape * AddStatiShape( Shape *shape );

		Body * AddBody( Body * body );

		Constraint * AddConstraint( Constraint * constraint );

		void RemoveShape( Shape * shape );

		void RemoveStatiShape( Shape * shape );

		void RemoveBody( Body * body );

		void RemoveConstraint( Constraint * constraint );

		cpSpace * GetSpace() const;

		void ActivateShapesTouchingShape( Shape * shape );

		virtual void Draw();

		Shape * PointQueryFirst( cVect point, cpLayers layers, cpGroup group );

		Shape * SegmentQueryFirst( cVect start, cVect end, cpLayers layers, cpGroup group, cpSegmentQueryInfo * out );

		void AddCollisionHandler( const cCollisionHandler& handler );

		void RemoveCollisionHandler( cpCollisionType a, cpCollisionType b );

		void SetDefaultCollisionHandler( const cCollisionHandler& handler );

		void AddPostStepCallback( PostStepCallback postStep, void * obj, void * data );

		virtual cpBool OnCollisionBegin( Arbiter * arb, void * data );

		virtual cpBool OnCollisionPreSolve( Arbiter * arb, void * data );

		virtual void OnCollisionPostSolve( Arbiter * arb, void * data );

		virtual void OnCollisionSeparate( Arbiter * arb, void * data );

		virtual void OnPostStepCallback( void * obj, void * data );

		virtual void OnBBQuery( Shape * shape, cBBQuery * query );

		virtual void OnSegmentQuery( Shape * shape, cpFloat t, cVect n , cSegmentQuery * query );

		virtual void OnPointQuery( Shape * shape, cPointQuery * query );

		void BBQuery( cBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void * data );

		void SegmentQuery( cVect start, cVect end, cpLayers layers, cpGroup group, SegmentQueryFunc func, void * data );

		void PointQuery( cVect point, cpLayers layers, cpGroup group, PointQueryFunc func, void * data );

		void Data( void * data );

		void * Data() const;

		void ReindexShape( Shape * shape );

		void ReindexShapesForBody( Body *body );

		void ReindexStatic();

		void UseSpatialHash( cpFloat dim, int count );

		void EachShape( ShapeIteratorFunc Func, void * data );

		virtual void OnEachShape( Shape * Shape, ShapeIterator * it );

		void EachBody( BodyIteratorFunc Func, void * data );

		virtual void OnEachBody( Body * Body, BodyIterator * it );

		void ConvertBodyToDynamic( Body * body, cpFloat mass, cpFloat moment );

		void ConvertBodyToStatic( Body * body );
	protected:
		cpSpace *									mSpace;
		Body *										mStatiBody;
		void *										mData;
		std::list<Body*>							mBodys;
		std::list<Shape*>							mShapes;
		std::list<Constraint*>						mConstraints;
		std::map< cpHashValue, cCollisionHandler >	mCollisions;
		cCollisionHandler							mCollisionsDefault;
		std::list< cPostStepCallback* >				mPostStepCallbacks;
};

CP_NAMESPACE_END

#endif
