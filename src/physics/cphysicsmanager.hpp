#ifndef EE_PHYSICS_PHYSICSMANAGER_HPP
#define EE_PHYSICS_PHYSICSMANAGER_HPP

#include "base.hpp"

CP_NAMESPACE_BEGIN

class cBody;
class cShape;
class cConstraint;
class cSpace;

class CP_API cPhysicsManager : public tSingleton<cPhysicsManager> {
	public:
		SINGLETON_DECLARE_HEADERS(cPhysicsManager)

		class cDrawSpaceOptions {
			public:
				cDrawSpaceOptions() :
					DrawBBs( false ),
					DrawShapes( true ),
					CollisionPointSize( 4.0f ),
					BodyPointSize( 0.0f ),
					LineThickness( 1.5f )
				{}

				bool	DrawBBs;
				bool	DrawShapes;
				cpFloat CollisionPointSize;
				cpFloat BodyPointSize;
				cpFloat LineThickness;
		};

		cPhysicsManager();

		~cPhysicsManager();

		/** The Memory Manager will keep track of all the allocations from cSpace, cBody, cShape and cConstraint and will release any non-released pointer.
		***	This is a lazy deallocation for the lazy programmers. It is disabled by default.
		*** To work properly set as active before allocating anything, activate it just before the singleton creation.
		*/
		void MemoryManager( bool MemoryManager );

		const bool& MemoryManager() const;

		cPhysicsManager::cDrawSpaceOptions * GetDrawOptions();
	protected:
		cDrawSpaceOptions	mOptions;

		friend class cBody;
		friend class cShape;
		friend class cConstraint;
		friend class cSpace;

		bool						mMemoryManager;
		std::list<cBody *>			mBodysFree;
		std::list<cShape *>			mShapesFree;
		std::list<cConstraint *>	mConstraintFree;
		std::list<cSpace*>			mSpaces;

		void AddBodyFree( cBody * body );

		void RemoveBodyFree( cBody * body );

		void AddShapeFree( cShape * shape );

		void RemoveShapeFree( cShape * shape );

		void AddConstraintFree( cConstraint * constraint );

		void RemoveConstraintFree( cConstraint * constraint );

		void AddSpace( cSpace * space );

		void RemoveSpace( cSpace * space );
};

CP_NAMESPACE_END

#endif
