#ifndef EE_GAMINGCGAMEOBJECTSHAPE_HPP
#define EE_GAMINGCGAMEOBJECTSHAPE_HPP

#include "base.hpp"
#include "cgameobject.hpp"

#include "../graphics/cshape.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class cGameObjectShape : public cGameObject {
	public:
		cGameObjectShape( const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, cShape * Shape = NULL, const eeVector2f& Pos = eeVector2f() );

		virtual ~cGameObjectShape();

		virtual void Draw();

		virtual void Update();

		virtual eeVector2f Pos() const;

		virtual void Pos( eeVector2f pos );

		cShape * Shape() const;

		void Shape( cShape * shape );

		virtual Uint32 Type() const;
	protected:
		cShape *	mShape;
		eeVector2f	mPos;
};

}}

#endif
