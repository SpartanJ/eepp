#ifndef EE_PHYSICS_PHYSICSMANAGER_HPP
#define EE_PHYSICS__PHYSICSMANAGER_HPP

#include "base.hpp"

namespace EE { namespace Physics {

class EE_API cPhysicsManager : public tSingleton<cPhysicsManager> {
	friend class tSingleton<cPhysicsManager>;
	public:
		class cDrawSpaceOptions {
			public:
				cDrawSpaceOptions() :
					DrawHash( false ),
					DrawBBs( false ),
					DrawShapes( true ),
					CollisionPointSize( 4.0f ),
					BodyPointSize( 0.0f ),
					LineThickness( 1.5f )
				{}

				int		DrawHash;
				int		DrawBBs;
				int		DrawShapes;
				cpFloat CollisionPointSize;
				cpFloat BodyPointSize;
				cpFloat LineThickness;
		};

		cPhysicsManager();

		~cPhysicsManager();

		const cpFloat& BiasCoef() const;

		void BiasCoef( const cpFloat& biasCoef );

		const cpFloat& ConstraintBiasCoef() const;

		void ConstraintBiasCoef( const cpFloat& constraintBiasCoef );

		const cpTimestamp& ContactPersistence() const;

		void ContactPersistence( const cpTimestamp& timestamp );

		const cpFloat& CollisionSlop() const;

		void CollisionSlop( const cpFloat& slop );

		cPhysicsManager::cDrawSpaceOptions * GetDrawOptions();
	protected:
		cDrawSpaceOptions	mOptions;
};

}}

#endif
