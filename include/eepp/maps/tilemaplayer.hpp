#ifndef EE_MAPS_CTILELAYER_HPP
#define EE_MAPS_CTILELAYER_HPP

#include <eepp/maps/maplayer.hpp>
#include <eepp/maps/gameobject.hpp>

namespace EE { namespace Maps {

class EE_API TileMapLayer : public MapLayer {
	public:
		virtual ~TileMapLayer();

		virtual void draw( const Vector2f &Offset = Vector2f(0,0) );

		virtual void update( const Time& dt );

		virtual void addGameObject( GameObject * obj, const Vector2i& TilePos );

		virtual void removeGameObject( const Vector2i& TilePos );

		virtual void moveTileObject( const Vector2i& FromPos, const Vector2i& ToPos );

		virtual GameObject * getGameObject( const Vector2i& TilePos );

		const Vector2i& getCurrentTile() const;

		Vector2i getTilePosFromPos( const Vector2f& Pos );

		Vector2f getPosFromTilePos( const Vector2i& TilePos );
	protected:
		friend class TileMap;

		GameObject***	mTiles;
		Sizei			mSize;
		Vector2i		mCurTile;

		TileMapLayer( TileMap * map, Sizei size, Uint32 flags, std::string name = "", Vector2f offset = Vector2f(0,0) );

		void allocateLayer();

		void deallocateLayer();
};

}}

#endif
