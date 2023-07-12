#ifndef EE_MAPS_CGAMEOBJECTTEXTUREREGION_HPP
#define EE_MAPS_CGAMEOBJECTTEXTUREREGION_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobject.hpp>

#include <eepp/graphics/textureregion.hpp>
using namespace EE::Graphics;

namespace EE { namespace Maps {

class EE_MAPS_API GameObjectTextureRegion : public GameObject {
  public:
	GameObjectTextureRegion( const Uint32& Flags, MapLayer* Layer,
							 Graphics::TextureRegion* TextureRegion = NULL,
							 const Vector2f& Pos = Vector2f() );

	virtual ~GameObjectTextureRegion();

	virtual void draw();

	virtual Vector2f getPosition() const;

	virtual void setPosition( Vector2f pos );

	virtual Vector2i getTilePosition() const;

	virtual void setTilePosition( Vector2i pos );

	virtual Sizei getSize();

	Graphics::TextureRegion* getTextureRegion() const;

	void setTextureRegion( Graphics::TextureRegion* TextureRegion );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type );

	virtual Uint32 getDataId();

	virtual void setDataId( Uint32 Id );

  protected:
	Graphics::TextureRegion* mTextureRegion;
	Vector2f mPos;
	Vector2i mTilePos;
};

}} // namespace EE::Maps

#endif
