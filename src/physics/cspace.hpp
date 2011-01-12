#ifndef EE_PHYSICS_CSPACE_HPP
#define EE_PHYSICS_CSPACE_HPP

#include "base.hpp"
#include "cbody.hpp"
#include "cshape.hpp"
#include "constraints/cconstraint.hpp"

namespace EE { namespace Physics {

class cSpace {
	public:
		cSpace();

		~cSpace();

		void Update( const cpFloat& dt );

		void Update();

		cBody * StaticBody() const;

		const int& Iterations() const;

		void Iterations( const int& iterations );

		const cpVect& Gravity() const;

		void Gravity( const cpVect& gravity );

		const cpFloat& Damping() const;

		void Damping( const cpFloat& damping );

		const cpFloat& IdleSpeedThreshold() const;

		void IdleSpeedThreshold( const cpFloat& idleSpeedThreshold );

		const cpFloat& SleepTimeThreshold() const;

		void SleepTimeThreshold( const cpFloat& sleepTimeThreshold );

		cShape * AddShape( cShape *shape );

		cShape * AddStaticShape( cShape *shape );

		cBody * AddBody( cBody * body );

		cConstraint * AddConstraint( cConstraint * constraint );

		void RemoveShape( cShape * shape);

		void RemoveStaticShape( cShape * shape );

		void RemoveBody( cBody * body );

		void RemoveConstraint( cConstraint * constraint );

		void ResizeStaticHash( cpFloat dim, int count );

		void ResizeActiveHash( cpFloat dim, int count );

		void RehashStatic();

		void RehashShape( cShape * shape );

		cpSpace * Space() const;

		void ActivateShapesTouchingShape( cShape * shape );

		virtual void Draw();
	protected:
		cpSpace *					mSpace;
		cBody *						mStaticBody;
		std::list<cBody*>			mBodys;
		std::list<cShape*>			mShapes;
		std::list<cConstraint*>		mConstraints;
};

}}

#endif
