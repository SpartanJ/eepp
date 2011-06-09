#ifndef EE_GAMINGCTILELAYER_HPP
#define EE_GAMINGCTILELAYER_HPP

#include "clayer.hpp"
#include "cgameobject.hpp"

namespace EE { namespace Gaming {

class cTileLayer : public cLayer {
	public:
		virtual ~cTileLayer();

		virtual void Draw( const eeVector2f &Offset = eeVector2f(0,0) );

		virtual void Update();

		virtual void AddGameObject( cGameObject * obj, const eeVector2i& TilePos );

		virtual void RemoveGameObject( const eeVector2i& TilePos );

		virtual cGameObject * GetGameObject( const eeVector2i& TilePos );
	protected:
		friend class cMap;

		cGameObject***	mTiles;
		eeSize			mSize;

		cTileLayer( cMap * map, eeSize size, Uint32 flags, std::string name = "", eeVector2f offset = eeVector2f(0,0) );

		void AllocateLayer();

		void DeallocateLayer();
};

}}

#endif
