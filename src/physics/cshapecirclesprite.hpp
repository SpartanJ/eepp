#ifndef CSHAPECIRCLESPRITE_HPP
#define CSHAPECIRCLESPRITE_HPP

#include "cshapecircle.hpp"

#ifdef PHYSICS_RENDERER_ENABLED

#include "../graphics/csprite.hpp"
using namespace EE::Graphics;

CP_NAMESPACE_BEGIN

class CP_API cShapeCircleSprite : public cShapeCircle {
	public:
		static cShapeCircleSprite * New( cBody * body, cpFloat radius, cVect offset, cSprite * Sprite, bool AutoDeleteSprite = false );

		cShapeCircleSprite( cBody * body, cpFloat radius, cVect offset, cSprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~cShapeCircleSprite();

		virtual void Draw( cSpace * space );

		virtual void Radius( const cpFloat& radius );

		virtual void Offset( const cVect &offset );

		cSprite * GetSprite() const;
	protected:
		cSprite *	mSprite;
		bool		mSpriteAutoDelete;

		void OffsetSet();
};

CP_NAMESPACE_END

#endif

#endif
