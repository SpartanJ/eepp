#ifndef EE_GAMINGCGAMEOBJECTPOLYLINE_HPP
#define EE_GAMINGCGAMEOBJECTPOLYLINE_HPP

#include <eepp/gaming/gameobjectpolygon.hpp>

namespace EE { namespace Gaming {

class EE_API GameObjectPolyline : public GameObjectPolygon {
	public:
		GameObjectPolyline( Uint32 DataId, Polygon2f poly, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~GameObjectPolyline();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type );

		virtual void draw();

		virtual GameObjectObject * clone();
};

}}

#endif
