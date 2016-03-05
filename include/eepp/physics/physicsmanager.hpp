#ifndef EE_PHYSICS_PHYSICSMANAGER_HPP
#define EE_PHYSICS_PHYSICSMANAGER_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class Body;
class Shape;
class Constraint;
class Space;

class CP_API PhysicsManager {
	SINGLETON_DECLARE_HEADERS(PhysicsManager)

	public:
		class DrawSpaceOptions {
			public:
				DrawSpaceOptions() :
					DrawBBs( false ),
					DrawShapes( true ),
	#ifdef EE_GLES
					DrawShapesBorders( false ),
	#else
					DrawShapesBorders( true ),
	#endif
					CollisionPointSize( 0.0f ),
					BodyPointSize( 0.0f ),
					LineThickness( 0.0f )
				{}

				bool	DrawBBs;
				bool	DrawShapes;
				bool	DrawShapesBorders;
				cpFloat CollisionPointSize;
				cpFloat BodyPointSize;
				cpFloat LineThickness;
		};

		~PhysicsManager();

		/** The Memory Manager will keep track of all the allocations from Space, Body, Shape and Constraint and will release any non-released pointer.
		***	This is a lazy deallocation for the lazy programmers. It is disabled by default.
		*** To work properly set as active before allocating anything, activate it just after the singleton instantiation.
		*/
		void MemoryManager( bool MemoryManager );

		const bool& MemoryManager() const;

		PhysicsManager::DrawSpaceOptions * GetDrawOptions();
	protected:
		DrawSpaceOptions	mOptions;

		friend class Body;
		friend class Shape;
		friend class Constraint;
		friend class Space;

		bool						mMemoryManager;
		std::list<Body *>			mBodysFree;
		std::list<Shape *>			mShapesFree;
		std::list<Constraint *>	mConstraintFree;
		std::list<Space*>			mSpaces;

		PhysicsManager();

		void AddBodyFree( Body * body );

		void RemoveBodyFree( Body * body );

		void AddShapeFree( Shape * shape );

		void RemoveShapeFree( Shape * shape );

		void AddConstraintFree( Constraint * constraint );

		void RemoveConstraintFree( Constraint * constraint );

		void AddSpace( Space * space );

		void RemoveSpace( Space * space );
};

CP_NAMESPACE_END

#endif
