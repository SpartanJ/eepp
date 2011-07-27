#ifndef EE_GAMINGCGAMEOBJECTVIRTUAL_HPP
#define EE_GAMINGCGAMEOBJECTVIRTUAL_HPP

#include "base.hpp"
#include "cgameobject.hpp"
#include "../graphics/cshape.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class cLayer;

class EE_API cGameObjectVirtual : public cGameObject {
	public:
		cGameObjectVirtual( Uint32 DataId, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const eeVector2f& Pos = eeVector2f() );

		cGameObjectVirtual( cShape * Shape, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const eeVector2f& Pos = eeVector2f() );

		virtual ~cGameObjectVirtual();

		virtual void Draw();

		virtual eeVector2f Pos() const;

		virtual eeSize Size();

		virtual void Pos( eeVector2f pos );

		virtual Uint32 Type() const;

		virtual Uint32 RealType() const;

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );

		void SetLayer( cLayer * Layer );
	protected:
		Uint32		mType;
		Uint32		mDataId;
		eeVector2f	mPos;
		cLayer *	mLayer;
		cShape *	mShape;
};

}}

#endif
