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

		void addLight( MapLight * Light );

		void removeLight( const Vector2f& OverPos );

		void removeLight( MapLight * Light );

		Uint32 getCount();

		const ColorA * getTileColor( const Vector2i& TilePos );

		const ColorA * getTileColor( const Vector2i& TilePos, const Uint32& Vertex );

		ColorA getColorFromPos( const Vector2f& Pos );

		const bool& isByVertex() const;

		LightsList& getLights();

		MapLight * getLightOver( const Vector2f& OverPos, MapLight * LightCurrent = NULL );
	protected:
		TileMap *		mMap;
		Int32			mNumVertex;
		ColorA****		mTileColors;
		LightsList		mLights;
		bool			mIsByVertex;

		void allocateColors();

		void deallocateColors();

		void destroyLights();

		virtual void updateByVertex();

		virtual void updateByTile();
};

}}

#endif
