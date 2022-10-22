#ifndef EE_MAPS_CGAMEOBJECTSPRITE_HPP
#define EE_MAPS_CGAMEOBJECTSPRITE_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobject.hpp>

namespace EE { namespace Graphics {
class Sprite;
}} // namespace EE::Graphics

namespace EE { namespace Maps {

class EE_API GameObjectSprite : public GameObject {
  public:
	GameObjectSprite( const Uint32& Flags, MapLayer* Layer, Graphics::Sprite* Sprite = NULL );

	virtual ~GameObjectSprite();

	virtual void draw();

	virtual void update( const Time& dt );

	Vector2f getPosition() const;

	virtual void setPosition( Vector2f pos );

	virtual Vector2i getTilePosition() const;

	virtual void setTilePosition( Vector2i pos );

	virtual Sizei getSize();

	Graphics::Sprite* getSprite() const;

	void setSprite( Graphics::Sprite* sprite );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type );

	virtual void setFlag( const Uint32& Flag );

	virtual Uint32 getDataId();

	virtual void setDataId( Uint32 Id );

  protected:
	Graphics::Sprite* mSprite;
	Vector2i mTilePos;
};

}} // namespace EE::Maps

#endif
