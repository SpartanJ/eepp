#ifndef EE_PHYSICS_CSHAPEPOLYSPRITE_HPP
#define EE_PHYSICS_CSHAPEPOLYSPRITE_HPP

#include <eepp/physics/shapepoly.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}}
using namespace EE::Graphics;

CP_NAMESPACE_BEGIN

class CP_API ShapePolySprite : public ShapePoly {
	public:
		static ShapePolySprite * New( Physics::Body * body, int numVerts, cVect *verts, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		static ShapePolySprite * New( Physics::Body * body, cpFloat width, cpFloat height, Sprite * Sprite, bool AutoDeleteSprite = false );

		ShapePolySprite( Physics::Body * body, int numVerts, cVect *verts, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		ShapePolySprite( Physics::Body * body, cpFloat width, cpFloat height, Sprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~ShapePolySprite();

		virtual void Draw( Space * space );

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
