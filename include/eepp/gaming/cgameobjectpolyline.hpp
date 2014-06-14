#ifndef EE_GAMINGCGAMEOBJECTPOLYLINE_HPP
#define EE_GAMINGCGAMEOBJECTPOLYLINE_HPP

#include <eepp/gaming/cgameobjectpolygon.hpp>

namespace EE { namespace Gaming {

class EE_API cGameObjectPolyline : public cGameObjectPolygon {
	public:
		cGameObjectPolyline( Uint32 DataId, Polygon2f poly, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~cGameObjectPolyline();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual void Draw();

		virtual cGameObjectObject * Copy();
};

}}

#endif
