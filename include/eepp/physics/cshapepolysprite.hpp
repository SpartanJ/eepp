#ifndef EE_PHYSICS_CSHAPEPOLYSPRITE_HPP
#define EE_PHYSICS_CSHAPEPOLYSPRITE_HPP

#include <eepp/physics/cshapepoly.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}}
using namespace EE::Graphics;

CP_NAMESPACE_BEGIN

class CP_API cShapePolySprite : public cShapePoly {
	public:
		static cShapePolySprite * New( cBody * body, int numVerts, cVect *verts, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		static cShapePolySprite * New( cBody * body, cpFloat width, cpFloat height, Sprite * Sprite, bool AutoDeleteSprite = false );

		cShapePolySprite( cBody * body, int numVerts, cVect *verts, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		cShapePolySprite( cBody * body, cpFloat width, cpFloat height, Sprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~cShapePolySprite();

		virtual void Draw( cSpace * space );

		Sprite * GetSprite() const;
	protected:
		Sprite *	mSprite;
		bool		mSpriteAutoDelete;
		Vector2i	mOffset;

		void OffsetSet( cVect center );
};

CP_NAMESPACE_END

#endif

#endif
