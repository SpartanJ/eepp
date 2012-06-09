#ifndef EE_GAMINGCTILELAYER_HPP
#define EE_GAMINGCTILELAYER_HPP

#include <eepp/gaming/clayer.hpp>
#include <eepp/gaming/cgameobject.hpp>

namespace EE { namespace Gaming {

class EE_API cTileLayer : public cLayer {
	public:
		virtual ~cTileLayer();

		virtual void Draw( const eeVector2f &Offset = eeVector2f(0,0) );

		virtual void Update();

		virtual void AddGameObject( cGameObject * obj, const eeVector2i& TilePos );

		virtual void RemoveGameObject( const eeVector2i& TilePos );

		virtual void MoveTileObject( const eeVector2i& FromPos, const eeVector2i& ToPos );

		virtual cGameObject * GetGameObject( const eeVector2i& TilePos );

		const eeVector2i& GetCurrentTile() const;

		eeVector2i GetTilePosFromPos( const eeVector2f& Pos );

		eeVector2f GetPosFromTilePos( const eeVector2i& TilePos );
	protected:
		friend class cMap;

		cGameObject***	mTiles;
		eeSize			mSize;
		eeVector2i		mCurTile;

		cTileLayer( cMap * map, eeSize size, Uint32 flags, std::string name = "", eeVector2f offset = eeVector2f(0,0) );

		void AllocateLayer();

		void DeallocateLayer();
};

}}

#endif
