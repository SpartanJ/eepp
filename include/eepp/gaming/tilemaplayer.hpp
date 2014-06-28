#ifndef EE_GAMINGCTILELAYER_HPP
#define EE_GAMINGCTILELAYER_HPP

#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/gameobject.hpp>

namespace EE { namespace Gaming {

class EE_API TileMapLayer : public MapLayer {
	public:
		virtual ~TileMapLayer();

		virtual void Draw( const Vector2f &Offset = Vector2f(0,0) );

		virtual void Update();

		virtual void AddGameObject( GameObject * obj, const Vector2i& TilePos );

		virtual void RemoveGameObject( const Vector2i& TilePos );

		virtual void MoveTileObject( const Vector2i& FromPos, const Vector2i& ToPos );

		virtual GameObject * GetGameObject( const Vector2i& TilePos );

		const Vector2i& GetCurrentTile() const;

		Vector2i GetTilePosFromPos( const Vector2f& Pos );

		Vector2f GetPosFromTilePos( const Vector2i& TilePos );
	protected:
		friend class TileMap;

		GameObject***	mTiles;
		Sizei			mSize;
		Vector2i		mCurTile;

		TileMapLayer( TileMap * map, Sizei size, Uint32 flags, std::string name = "", Vector2f offset = Vector2f(0,0) );

		void AllocateLayer();

		void DeallocateLayer();
};

}}

#endif
