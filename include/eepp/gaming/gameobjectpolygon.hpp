#ifndef EE_GAMINGCGAMEOBJECTPOLYGON_HPP
#define EE_GAMINGCGAMEOBJECTPOLYGON_HPP

#include <eepp/gaming/gameobjectobject.hpp>

namespace EE { namespace Gaming {

class EE_API GameObjectPolygon : public GameObjectObject {
	public:
		GameObjectPolygon( Uint32 DataId, Polygon2f poly, MapLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~GameObjectPolygon();

		virtual void Draw();

		virtual Sizei Size();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual bool PointInside( const Vector2f& p );

		virtual void SetPolygonPoint( Uint32 index, Vector2f p );

		virtual GameObjectObject * Copy();
};

}}

#endif
