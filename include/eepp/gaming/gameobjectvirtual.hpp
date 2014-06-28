#ifndef EE_GAMINGCGAMEOBJECTVIRTUAL_HPP
#define EE_GAMINGCGAMEOBJECTVIRTUAL_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/gameobject.hpp>
#include <eepp/graphics/subtexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class MapLayer;

class EE_API GameObjectVirtual : public GameObject {
	public:
		GameObjectVirtual( Uint32 DataId, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const Vector2f& Pos = Vector2f() );

		GameObjectVirtual( SubTexture * SubTexture, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const Vector2f& Pos = Vector2f() );

		virtual ~GameObjectVirtual();

		virtual void Draw();

		virtual Vector2f Pos() const;

		virtual Sizei Size();

		virtual void Pos( Vector2f pos );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual Uint32 RealType() const;

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );

		void SetLayer( MapLayer * Layer );
	protected:
		Uint32		mType;
		Uint32		mDataId;
		Vector2f	mPos;
		MapLayer *	mLayer;
		SubTexture *	mSubTexture;
};

}}

#endif
