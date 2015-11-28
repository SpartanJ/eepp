#ifndef EE_GAMINGCLIGHTMANAGER_HPP
#define EE_GAMINGCLIGHTMANAGER_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/maplight.hpp>
#include <list>

namespace EE { namespace Gaming {

class TileMap;

class EE_API MapLightManager {
	public:
		typedef std::list<MapLight*> LightsList;

		MapLightManager( TileMap * Map, bool ByVertex );

		virtual ~MapLightManager();

		virtual void Update();

		void AddLight( MapLight * Light );

		void RemoveLight( const Vector2f& OverPos );

		void RemoveLight( MapLight * Light );

		Uint32 Count();

		const ColorA * GetTileColor( const Vector2i& TilePos );

		const ColorA * GetTileColor( const Vector2i& TilePos, const Uint32& Vertex );

		ColorA GetColorFromPos( const Vector2f& Pos );

		const bool& IsByVertex() const;

		LightsList& GetLights();

		MapLight * GetLightOver( const Vector2f& OverPos, MapLight * LightCurrent = NULL );
	protected:
		TileMap *				mMap;
		Int32				mNumVertex;
		ColorA****		mTileColors;
		LightsList			mLights;
		bool				mIsByVertex;

		void AllocateColors();

		void DeallocateColors();

		void DestroyLights();

		virtual void UpdateByVertex();

		virtual void UpdateByTile();
};

}}

#endif
