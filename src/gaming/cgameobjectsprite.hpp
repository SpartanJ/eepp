#ifndef EE_GAMINGCGAMEOBJECTSPRITE_HPP
#define EE_GAMINGCGAMEOBJECTSPRITE_HPP

#include "base.hpp"
#include "cgameobject.hpp"

#include "../graphics/csprite.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class cGameObjectSprite : public cGameObject {
	public:
		cGameObjectSprite( const Uint32& Flags = GObjFlags::GAMEOBJECT_ANIMATED, cSprite * Sprite = NULL );

		virtual ~cGameObjectSprite();

		virtual void Draw();

		virtual void Update();

		eeVector2f Pos() const;

		virtual void Pos( eeVector2f pos );

		cSprite * Sprite() const;

		void Sprite( cSprite * sprite );

		virtual Uint32 Type() const;
	private:
		cSprite *	mSprite;
};

}}

#endif
