#ifndef EE_GAMINGCGAMEOBJECT_HPP
#define EE_GAMINGCGAMEOBJECT_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/maphelper.hpp>
#include <eepp/gaming/maplayer.hpp>

#include <eepp/graphics/renders.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

class EE_API GameObject {
	public:
		GameObject( const Uint32& Flags, MapLayer * Layer );

		virtual ~GameObject();

		virtual void Draw();

		virtual void Update();

		virtual Vector2f Pos() const;

		virtual void Pos( Vector2f pos );

		virtual Vector2i TilePos() const;

		virtual void TilePos( Vector2i pos );

		virtual Sizei Size();

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

		MapLayer * Layer() const;
	protected:
		Uint32		mFlags;
		MapLayer *	mLayer;

		virtual EE_RENDER_MODE RenderModeFromFlags();

		void AutoFixTilePos();

		void AssignTilePos();

		Float GetAngle();
};

}}

#endif
