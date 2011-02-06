#ifndef EE_PHYSICS_CSHAPEPOLYSPRITE_HPP
#define EE_PHYSICS_CSHAPEPOLYSPRITE_HPP

#include "cshapepoly.hpp"

#ifdef PHYSICS_RENDERER_ENABLED

#include "../graphics/csprite.hpp"
using namespace EE::Graphics;

CP_NAMESPACE_BEGIN

class CP_API cShapePolySprite : public cShapePoly {
	public:
		static cShapePolySprite * New( cBody * body, int numVerts, cVect *verts, cVect offset, cSprite * Sprite, bool AutoDeleteSprite = false );

		static cShapePolySprite * New( cBody * body, cpFloat width, cpFloat height, cSprite * Sprite, bool AutoDeleteSprite = false );

		cShapePolySprite( cBody * body, int numVerts, cVect *verts, cVect offset, cSprite * Sprite, bool AutoDeleteSprite = false );

		cShapePolySprite( cBody * body, cpFloat width, cpFloat height, cSprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~cShapePolySprite();

		virtual void Draw( cSpace * space );

		cSprite * GetSprite() const;
	protected:
		cSprite *	mSprite;
		bool		mSpriteAutoDelete;

		void OffsetSet( cVect center );
};

CP_NAMESPACE_END

#endif

#endif
