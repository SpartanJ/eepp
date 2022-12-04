#ifndef EE_MAPS_CLIGHTMANAGER_HPP
#define EE_MAPS_CLIGHTMANAGER_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/maplight.hpp>
#include <list>

namespace EE { namespace Maps {

class TileMap;

class EE_MAPS_API MapLightManager {
  public:
	typedef std::list<MapLight*> LightsList;

	MapLightManager( TileMap* Map, bool ByVertex );

	virtual ~MapLightManager();

	virtual void update();

	void addLight( MapLight* Light );

	void removeLight( const Vector2f& OverPos );

	void removeLight( MapLight* Light );

	Uint32 getCount();

	const Color* getTileColor( const Vector2i& TilePos );

	const Color* getTileColor( const Vector2i& TilePos, const Uint32& Vertex );

	Color getColorFromPos( const Vector2f& Pos );

	const bool& isByVertex() const;

	LightsList& getLights();

	MapLight* getLightOver( const Vector2f& OverPos, MapLight* LightCurrent = NULL );

  protected:
	TileMap* mMap;
	Int32 mNumVertex;
	Color**** mTileColors;
	LightsList mLights;
	bool mIsByVertex;

	void allocateColors();

	void deallocateColors();

	void destroyLights();

	virtual void updateByVertex();

	virtual void updateByTile();
};

}} // namespace EE::Maps

#endif
