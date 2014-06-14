#ifndef EE_GAMINGCTILELAYER_HPP
#define EE_GAMINGCTILELAYER_HPP

#include <eepp/gaming/clayer.hpp>
#include <eepp/gaming/cgameobject.hpp>

namespace EE { namespace Gaming {

class EE_API cTileLayer : public cLayer {
	public:
		virtual ~cTileLayer();

		virtual void Draw( const Vector2f &Offset = Vector2f(0,0) );

		virtual void Update();

		virtual void AddGameObject( cGameObject * obj, const Vector2i& TilePos );

		virtual void RemoveGameObject( const Vector2i& TilePos );

		virtual void MoveTileObject( const Vector2i& FromPos, const Vector2i& ToPos );

		virtual cGameObject * GetGameObject( const Vector2i& TilePos );

		const Vector2i& GetCurrentTile() const;

		Vector2i GetTilePosFromPos( const Vector2f& Pos );

		Vector2f GetPosFromTilePos( const Vector2i& TilePos );
	protected:
		friend class cMap;

		cGameObject***	mTiles;
		Sizei			mSize;
		Vector2i		mCurTile;

		cTileLayer( cMap * map, Sizei size, Uint32 flags, std::string name = "", Vector2f offset = Vector2f(0,0) );

		void AllocateLayer();

		void DeallocateLayer();
};

}}

#endif
