#ifndef EE_GAMINGCGAMEOBJECTVIRTUAL_HPP
#define EE_GAMINGCGAMEOBJECTVIRTUAL_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/cgameobject.hpp>
#include <eepp/graphics/subtexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class cLayer;

class EE_API cGameObjectVirtual : public cGameObject {
	public:
		cGameObjectVirtual( Uint32 DataId, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const Vector2f& Pos = Vector2f() );

		cGameObjectVirtual( SubTexture * SubTexture, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const Vector2f& Pos = Vector2f() );

		virtual ~cGameObjectVirtual();

		virtual void Draw();

		virtual Vector2f Pos() const;

		virtual Sizei Size();

		virtual void Pos( Vector2f pos );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual Uint32 RealType() const;

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );

		void SetLayer( cLayer * Layer );
	protected:
		Uint32		mType;
		Uint32		mDataId;
		Vector2f	mPos;
		cLayer *	mLayer;
		SubTexture *	mSubTexture;
};

}}

#endif
