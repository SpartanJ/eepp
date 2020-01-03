#ifndef CSHAPECIRCLESPRITE_HPP
#define CSHAPECIRCLESPRITE_HPP

#include <eepp/physics/shapecircle.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}}
using namespace EE::Graphics;

namespace EE { namespace Physics {

class EE_API ShapeCircleSprite : public ShapeCircle {
	public:
		static ShapeCircleSprite * New( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		ShapeCircleSprite( Physics::Body * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~ShapeCircleSprite();

		virtual void draw( Space * space );

		virtual void setRadius( const cpFloat& radius );

		virtual void setOffset( const cVect &offset );

		Sprite * getSprite() const;
	protected:
		Sprite *	mSprite;
		bool		mSpriteAutoDelete;

		void offsetSet();
};

}}

#endif

#endif
