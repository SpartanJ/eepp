#ifndef EE_MAPS_CGAMEOBJECTPOLYGON_HPP
#define EE_MAPS_CGAMEOBJECTPOLYGON_HPP

#include <eepp/maps/gameobjectobject.hpp>

namespace EE { namespace Maps {

class EE_MAPS_API GameObjectPolygon : public GameObjectObject {
  public:
	GameObjectPolygon( Uint32 DataId, Polygon2f poly, MapLayer* Layer,
					   const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

	virtual ~GameObjectPolygon();

	virtual void draw();

	virtual Sizei getSize();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type );

	virtual bool pointInside( const Vector2f& p );

	virtual void setPolygonPoint( Uint32 index, Vector2f p );

	virtual GameObjectObject* clone();
};

}} // namespace EE::Maps

#endif
