#ifndef EE_GAMINGCGAMEOBJECTSUBTEXTURE_HPP
#define EE_GAMINGCGAMEOBJECTSUBTEXTURE_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/gameobject.hpp>

#include <eepp/graphics/subtexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class EE_API GameObjectSubTexture : public GameObject {
	public:
		GameObjectSubTexture( const Uint32& Flags, MapLayer * Layer, Graphics::SubTexture * SubTexture = NULL, const Vector2f& Pos = Vector2f() );

		virtual ~GameObjectSubTexture();

		virtual void draw();

		virtual Vector2f getPosition() const;

		virtual void setPosition( Vector2f pos );

		virtual Vector2i getTilePosition() const;

		virtual void setTilePosition( Vector2i pos );

		virtual Sizei getSize();

		Graphics::SubTexture * getSubTexture() const;

		void setSubTexture( Graphics::SubTexture * subTexture );

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type );

		virtual Uint32 getDataId();

		virtual void setDataId( Uint32 Id );
	protected:
		Graphics::SubTexture *	mSubTexture;
		Vector2f				mPos;
		Vector2i				mTilePos;
};

}}

#endif
