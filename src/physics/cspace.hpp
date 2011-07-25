#ifndef EE_PHYSICS_CSPACE_HPP
#define EE_PHYSICS_CSPACE_HPP

#include "base.hpp"
#include "cbody.hpp"
#include "cshape.hpp"
#include "carbiter.hpp"
#include "constraints/cconstraint.hpp"

CP_NAMESPACE_BEGIN

class CP_API cSpace {
	public:
		typedef cb::Callback3<int	, cArbiter *, cSpace *	, void * >				CollisionBeginFunc;
		typedef cb::Callback3<int	, cArbiter *, cSpace *	, void * >				CollisionPreSolveFunc;
		typedef cb::Callback3<void	, cArbiter *, cSpace *	, void * >				CollisionPostSolveFunc;
		typedef cb::Callback3<void	, cArbiter *, cSpace *	, void * >				CollisionSeparateFunc;
		typedef cb::Callback3<void	, cSpace *	, void *	, void * >				PostStepCallback;
		typedef cb::Callback2<void	, cShape *	, void * >							BBQueryFunc;
		typedef cb::Callback4<void	, cShape *	, cpFloat	, cVect	, void * >		SegmentQueryFunc;
		typedef cb::Callback2<void	, cShape *	, void * >							PointQueryFunc;
		typedef cb::Callback3<void	, cSpace *	, cBody *	, void * >				BodyIteratorFunc;
		typedef cb::Callback3<void	, cSpace *	, cShape *	, void * >				ShapeIteratorFunc;

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

				cSpace *	Space;
				BBQueryFunc Func;
				void *		Data;
		};

		class cSegmentQuery {
			public:
				cSegmentQuery()
				{}

				cSpace *			Space;
				SegmentQueryFunc	Func;
				void *				Data;
		};

		class cPointQuery {
			public:
				cPointQuery() :
					Data( NULL )
				{}

				cSpace *		Space;
				PointQueryFunc	Func;
				void *			Data;
		};

		class cBodyIterator {
			public:
				cBodyIterator( cSpace * space, void * data, BodyIteratorFunc func ) :
					Space( space ),
					Data( data ),
					Func( func )
				{}

				cSpace *					Space;
				void *						Data;
				BodyIteratorFunc			Func;
		};

		class cShapeIterator {
			public:
				cShapeIterator( cSpace * space, void * data, ShapeIteratorFunc func ) :
					Space( space ),
					Data( data ),
					Func( func )
				{}

				cSpace *					Space;
				void *						Data;
				ShapeIteratorFunc			Func;
		};

		static cSpace * New();

		static void Free( cSpace * space );

		cSpace();

		virtual ~cSpace();

		void Step( const cpFloat& dt );

		void Update();

		cBody * StaticBody() const;

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

		bool Contains( cShape * shape );

		bool Contains( cBody * body );

		bool Contains( cConstraint * constraint );

		cShape * AddShape( cShape * shape );

		cShape * AddStaticShape( cShape *shape );

		cBody * AddBody( cBody * body );

		cConstraint * AddConstraint( cConstraint * constraint );

		void RemoveShape( cShape * shape );

		void RemoveStaticShape( cShape * shape );

		void RemoveBody( cBody * body );

		void RemoveConstraint( cConstraint * constraint );

		cpSpace * Space() const;

		void ActivateShapesTouchingShape( cShape * shape );

		virtual void Draw();

		cShape * PointQueryFirst( cVect point, cpLayers layers, cpGroup group );

		cShape * SegmentQueryFirst( cVect start, cVect end, cpLayers layers, cpGroup group, cpSegmentQueryInfo * out );

		void AddCollisionHandler( const cCollisionHandler& handler );

		void RemoveCollisionHandler( cpCollisionType a, cpCollisionType b );

		void SetDefaultCollisionHandler( const cCollisionHandler& handler );

		void AddPostStepCallback( PostStepCallback postStep, void * obj, void * data );

		virtual cpBool OnCollisionBegin( cArbiter * arb, void * data );

		virtual cpBool OnCollisionPreSolve( cArbiter * arb, void * data );

		virtual void OnCollisionPostSolve( cArbiter * arb, void * data );

		virtual void OnCollisionSeparate( cArbiter * arb, void * data );

		virtual void OnPostStepCallback( void * obj, void * data );

		virtual void OnBBQuery( cShape * shape, cBBQuery * query );

		virtual void OnSegmentQuery( cShape * shape, cpFloat t, cVect n , cSegmentQuery * query );

		virtual void OnPointQuery( cShape * shape, cPointQuery * query );

		void BBQuery( cBB bb, cpLayers layers, cpGroup group, BBQueryFunc func, void * data );

		void SegmentQuery( cVect start, cVect end, cpLayers layers, cpGroup group, SegmentQueryFunc func, void * data );

		void PointQuery( cVect point, cpLayers layers, cpGroup group, PointQueryFunc func, void * data );

		void Data( void * data );

		void * Data() const;

		void ReindexShape( cShape * shape );

		void ReindexShapesForBody( cBody *body );

		void ReindexStatic();

		void UseSpatialHash( cpFloat dim, int count );

		void EachShape( ShapeIteratorFunc Func, void * data );

		virtual void OnEachShape( cShape * Shape, cShapeIterator * it );

		void EachBody( BodyIteratorFunc Func, void * data );

		virtual void OnEachBody( cBody * Body, cBodyIterator * it );
	protected:
		cpSpace *									mSpace;
		cBody *										mStaticBody;
		void *										mData;
		std::list<cBody*>							mBodys;
		std::list<cShape*>							mShapes;
		std::list<cConstraint*>						mConstraints;
		std::map< cpHashValue, cCollisionHandler >	mCollisions;
		cCollisionHandler							mCollisionsDefault;
		std::list< cPostStepCallback* >				mPostStepCallbacks;
};

CP_NAMESPACE_END

#endif
