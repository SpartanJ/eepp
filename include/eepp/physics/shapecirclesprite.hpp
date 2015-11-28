#ifndef CSHAPECIRCLESPRITE_HPP
#define CSHAPECIRCLESPRITE_HPP

#include <eepp/physics/shapecircle.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}}
using namespace EE::Graphics;

CP_NAMESPACE_BEGIN

class CP_API ShapeCircleSprite : public ShapeCircle {
	public:
		static ShapeCircleSprite * New( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		ShapeCircleSprite( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~ShapeCircleSprite();

		virtual void Draw( Space * space );

		virtual void Radius( const cpFloat& radius );

		virtual void Offset( const cVect &offset );

		Sprite * GetSprite() const;
	protected:
		Sprite *	mSprite;
		bool		mSpriteAutoDelete;

		void OffsetSet();
};

CP_NAMESPACE_END

#endif

#endif
