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

		virtual void draw( Space * space );

		virtual void radius( const cpFloat& radius );

		virtual void offset( const cVect &offset );

		Sprite * getSprite() const;
	protected:
		Sprite *	mSprite;
		bool		mSpriteAutoDelete;

		void offsetSet();
};

CP_NAMESPACE_END

#endif

#endif
