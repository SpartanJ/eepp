#ifndef EE_GAMINGCGAMEOBJECTSHAPE_HPP
#define EE_GAMINGCGAMEOBJECTSHAPE_HPP

#include "base.hpp"
#include "cgameobject.hpp"

#include "../graphics/cshape.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class EE_API cGameObjectShape : public cGameObject {
	public:
		cGameObjectShape( const Uint32& Flags, cLayer * Layer, cShape * Shape = NULL, const eeVector2f& Pos = eeVector2f() );

		virtual ~cGameObjectShape();

		virtual void Draw();

		virtual void Update();

		virtual eeVector2f Pos() const;

		virtual eeSize Size();

		virtual void Pos( eeVector2f pos );

		cShape * Shape() const;

		void Shape( cShape * shape );

		virtual Uint32 Type() const;

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );
	protected:
		cShape *	mShape;
		eeVector2f	mPos;
};

}}

#endif
