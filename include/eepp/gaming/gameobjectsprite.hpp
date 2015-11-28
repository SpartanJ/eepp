#ifndef EE_GAMINGCGAMEOBJECTSPRITE_HPP
#define EE_GAMINGCGAMEOBJECTSPRITE_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/gameobject.hpp>

namespace EE { namespace Graphics {
class Sprite;
}}

namespace EE { namespace Gaming {

class EE_API GameObjectSprite : public GameObject {
	public:
		GameObjectSprite( const Uint32& Flags, MapLayer * Layer, Graphics::Sprite * Sprite = NULL );

		virtual ~GameObjectSprite();

		virtual void Draw();

		Vector2f Pos() const;

		virtual void Pos( Vector2f pos );

		virtual Vector2i TilePos() const;

		virtual void TilePos( Vector2i pos );

		virtual Sizei Size();

		Graphics::Sprite * Sprite() const;

		void Sprite( Graphics::Sprite * sprite );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual void FlagSet( const Uint32& Flag );

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );
	protected:
		Graphics::Sprite *	mSprite;
		Vector2i			mTilePos;
};

}}

#endif
