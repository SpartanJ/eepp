#ifndef EE_GAMINGCGAMEOBJECT_HPP
#define EE_GAMINGCGAMEOBJECT_HPP

#include "base.hpp"
#include "maphelper.hpp"
#include "clayer.hpp"

#include "../graphics/renders.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class EE_API cGameObject {
	public:
		cGameObject( const Uint32& Flags, cLayer * Layer );

		virtual ~cGameObject();

		virtual void Draw();

		virtual void Update();

		virtual eeVector2f Pos() const;

		virtual void Pos( eeVector2f pos );

		virtual eeVector2i TilePos() const;

		virtual void TilePos( eeVector2i pos );

		virtual eeSize Size();

		virtual Uint32 Type() const;

		bool IsType( const Uint32& type );

		virtual bool InheritsFrom( const Uint32& Type );

		bool IsTypeOrInheritsFrom( const Uint32& Type );

		const Uint32& Flags() const;

		Uint32 FlagGet( const Uint32& Flag );

		virtual void FlagSet( const Uint32& Flag );

		void FlagClear( const Uint32& Flag );

		Uint32 IsBlocked() const;

		Uint32 IsRotated() const;

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );

		cLayer * Layer() const;
	protected:
		Uint32		mFlags;
		cLayer *	mLayer;

		virtual EE_RENDERTYPE RenderTypeFromFlags();

		void AutoFixTilePos();

		void AssignTilePos();

		eeFloat GetAngle();
};

}}

#endif
