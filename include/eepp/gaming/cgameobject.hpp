#ifndef EE_GAMINGCGAMEOBJECT_HPP
#define EE_GAMINGCGAMEOBJECT_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/maphelper.hpp>
#include <eepp/gaming/clayer.hpp>

#include <eepp/graphics/renders.hpp>
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

		virtual bool IsType( const Uint32& type );

		const Uint32& Flags() const;

		Uint32 FlagGet( const Uint32& Flag );

		virtual void FlagSet( const Uint32& Flag );

		void FlagClear( const Uint32& Flag );

		Uint32 Blocked() const;

		void Blocked( bool blocked );

		Uint32 Rotated() const;

		void Rotated( bool rotated );

		Uint32 Mirrored() const;

		void Mirrored( bool mirrored );

		Uint32 Fliped() const;

		void Fliped( bool fliped );

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
