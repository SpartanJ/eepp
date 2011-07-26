#ifndef EE_GAMINGCLIGHTMANAGER_HPP
#define EE_GAMINGCLIGHTMANAGER_HPP

#include "base.hpp"
#include "clight.hpp"

namespace EE { namespace Gaming {

class cMap;

class cLightManager {
	public:
		cLightManager( cMap * Map, bool ByVertex );

		virtual ~cLightManager();

		virtual void Update();

		void AddLight( cLight * Light );

		void RemoveLight( const eeVector2f& OverPos );

		eeColorA * GetTileColor( const eeVector2i& TilePos );

		eeColorA * GetTileColor( const eeVector2i& TilePos, const Uint32& Vertex );

		eeColorA GetColorFromPos( const eeVector2f& Pos );

		const bool& IsByVertex() const;
	protected:
		cMap *				mMap;
		Int32				mNumVertex;
		eeColorA****		mTileColors;
		std::list<cLight*>	mLights;
		bool				mIsByVertex;

		void AllocateColors();

		void DeallocateColors();

		void DestroyLights();

		virtual void UpdateByVertex();

		virtual void UpdateByTile();
};

}}

#endif
