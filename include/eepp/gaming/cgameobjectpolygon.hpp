#ifndef EE_GAMINGCGAMEOBJECTPOLYGON_HPP
#define EE_GAMINGCGAMEOBJECTPOLYGON_HPP

#include <eepp/gaming/cgameobjectobject.hpp>

namespace EE { namespace Gaming {

class EE_API cGameObjectPolygon : public cGameObjectObject {
	public:
		cGameObjectPolygon( Uint32 DataId, eePolygon2f poly, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~cGameObjectPolygon();

		virtual void Draw();

		virtual eeSize Size();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual bool PointInside( const eeVector2f& p );

		virtual void SetPolygonPoint( Uint32 index, eeVector2f p );

		virtual cGameObjectObject * Copy();
};

}}

#endif
