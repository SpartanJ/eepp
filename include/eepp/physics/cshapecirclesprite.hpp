#ifndef CSHAPECIRCLESPRITE_HPP
#define CSHAPECIRCLESPRITE_HPP

#include <eepp/physics/cshapecircle.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}}
using namespace EE::Graphics;

CP_NAMESPACE_BEGIN

class CP_API cShapeCircleSprite : public cShapeCircle {
	public:
		static cShapeCircleSprite * New( cBody * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		cShapeCircleSprite( cBody * body, cpFloat radius, cVect offset, Sprite * Sprite, bool AutoDeleteSprite = false );

		virtual ~cShapeCircleSprite();

		virtual void Draw( cSpace * space );

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
