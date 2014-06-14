#ifndef EE_GAMINGCGAMEOBJECTSPRITE_HPP
#define EE_GAMINGCGAMEOBJECTSPRITE_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/cgameobject.hpp>

namespace EE { namespace Graphics {
class cSprite;
}}

namespace EE { namespace Gaming {

class EE_API cGameObjectSprite : public cGameObject {
	public:
		cGameObjectSprite( const Uint32& Flags, cLayer * Layer, cSprite * Sprite = NULL );

		virtual ~cGameObjectSprite();

		virtual void Draw();

		Vector2f Pos() const;

		virtual void Pos( Vector2f pos );

		virtual Vector2i TilePos() const;

		virtual void TilePos( Vector2i pos );

		virtual Sizei Size();

		cSprite * Sprite() const;

		void Sprite( cSprite * sprite );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual void FlagSet( const Uint32& Flag );

		virtual Uint32 DataId();

		virtual void DataId( Uint32 Id );
	protected:
		cSprite *	mSprite;
		Vector2i	mTilePos;
};

}}

#endif
