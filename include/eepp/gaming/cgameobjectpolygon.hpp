#ifndef EE_GAMINGCGAMEOBJECTPOLYGON_HPP
#define EE_GAMINGCGAMEOBJECTPOLYGON_HPP

#include <eepp/gaming/cgameobjectobject.hpp>

namespace EE { namespace Gaming {

class EE_API cGameObjectPolygon : public cGameObjectObject {
	public:
		cGameObjectPolygon( Uint32 DataId, eePolygon2f poly, cLayer * Layer, const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC );

		virtual ~cGameObjectPolygon();

		virtual void Draw();

		virtual eeVector2f Pos() const;

		virtual eeSize Size();

		virtual void Pos( eeVector2f pos );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual eePolygon2f GetPolygon();

		virtual bool PointInside( const eeVector2f& p );
	protected:
		eePolygon2f mPoly;
};

}}

#endif
