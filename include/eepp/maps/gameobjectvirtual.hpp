#ifndef EE_MAPS_CGAMEOBJECTVIRTUAL_HPP
#define EE_MAPS_CGAMEOBJECTVIRTUAL_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobject.hpp>
#include <eepp/graphics/textureregion.hpp>
using namespace EE::Graphics;

namespace EE { namespace Maps {

class MapLayer;

class EE_API GameObjectVirtual : public GameObject {
	public:
		GameObjectVirtual( Uint32 getDataId, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const Vector2f& Pos = Vector2f() );

		GameObjectVirtual( TextureRegion * TextureRegion, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, Uint32 RealType = GAMEOBJECT_TYPE_VIRTUAL, const Vector2f& Pos = Vector2f() );

		virtual ~GameObjectVirtual();

		virtual void draw();

		virtual Vector2f getPosition() const;

		virtual Sizei getSize();

		virtual void setPosition( Vector2f pos );

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type );

		virtual Uint32 getRealType() const;

		virtual Uint32 getDataId();

		virtual void setDataId( Uint32 Id );

		void setLayer( MapLayer * Layer );
	protected:
		Uint32		mType;
		Uint32		mDataId;
		Vector2f	mPos;
		MapLayer *	mLayer;
		TextureRegion *	mTextureRegion;
};

}}

#endif
