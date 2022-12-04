#ifndef EE_MAPS_CGAMEOBJECTPOLYLINE_HPP
#define EE_MAPS_CGAMEOBJECTPOLYLINE_HPP

#include <eepp/maps/gameobjectpolygon.hpp>

namespace EE { namespace Maps {

class EE_MAPS_API GameObjectPolyline : public GameObjectPolygon {
  public:
	GameObjectPolyline( Uint32 DataId, Polygon2f poly, MapLayer* Layer,
						const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

	virtual ~GameObjectPolyline();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type );

	virtual void draw();

	virtual GameObjectObject* clone();
};

}} // namespace EE::Maps

#endif
