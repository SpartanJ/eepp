#include <eepp/maps/maplightmanager.hpp>
#include <eepp/maps/tilemap.hpp>

namespace EE { namespace Maps {

MapLightManager::MapLightManager( TileMap * Map, bool ByVertex ) :
	mMap( Map ),
	mTileColors( NULL )
{
	mIsByVertex = ByVertex;

	if ( mIsByVertex )
		mNumVertex = 4;
	else
		mNumVertex = 1;

	allocateColors();
}

MapLightManager::~MapLightManager() {
	deallocateColors();
	destroyLights();
}

void MapLightManager::update() {
	if ( mIsByVertex ) {
		updateByVertex();
	} else {
		updateByTile();
	}
}

const bool& MapLightManager::isByVertex() const {
	return mIsByVertex;
}

void MapLightManager::updateByVertex() {
	Vector2i start	= mMap->getStartTile();
	Vector2i end		= mMap->getEndTile();
	Rectf VisibleArea	= mMap->getViewAreaAABB();
	Sizei TileSize		= mMap->getTileSize();
	Color BaseColor	= mMap->getBaseColor();
	bool firstLight		= true;
	Vector2i Pos;

	if ( !mLights.size() )
		return;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);

		if ( firstLight || VisibleArea.intersect( Light->getAABB() ) ) {
			for ( Int32 x = start.x; x < end.x; x++ ) {
				for ( Int32 y = start.y; y < end.y; y++ ) {
					if ( firstLight ) {
						mTileColors[x][y][0]->r = mTileColors[x][y][1]->r = mTileColors[x][y][2]->r = mTileColors[x][y][3]->r = BaseColor.r;
						mTileColors[x][y][0]->g = mTileColors[x][y][1]->g = mTileColors[x][y][2]->g = mTileColors[x][y][3]->g = BaseColor.g;
						mTileColors[x][y][0]->b = mTileColors[x][y][1]->b = mTileColors[x][y][2]->b = mTileColors[x][y][3]->b = BaseColor.b;
					}

					Pos.x = x * TileSize.x;
					Pos.y = y * TileSize.y;

					Rectf TileAABB( Pos.x, Pos.y, Pos.x + TileSize.x, Pos.y + TileSize.y );

					if ( TileAABB.intersect( Light->getAABB() ) ) {
						if ( y > 0 )
							mTileColors[x][y][0]->assign( *mTileColors[x][y - 1][1] );
						else
							mTileColors[x][y][0]->assign( Light->processVertex( Pos.x, Pos.y, *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );

						mTileColors[x][y][1]->assign( Light->processVertex( Pos.x, Pos.y + TileSize.getHeight(), *(mTileColors[x][y][1]), *(mTileColors[x][y][1]) ) );

						mTileColors[x][y][2]->assign( Light->processVertex( Pos.x + TileSize.getWidth(), Pos.y + TileSize.getHeight(), *(mTileColors[x][y][2]), *(mTileColors[x][y][2]) ) );

						if ( y > 0 )
							mTileColors[x][y][3]->assign( *mTileColors[x][y - 1][2] );
						else
							mTileColors[x][y][3]->assign( Light->processVertex( Pos.x + TileSize.getWidth(), Pos.y, *(mTileColors[x][y][3]), *(mTileColors[x][y][3]) ) );
					}
				}
			}

			firstLight = false;
		}
	}
}

void MapLightManager::updateByTile() {
	Vector2i start	= mMap->getStartTile();
	Vector2i end		= mMap->getEndTile();
	Rectf VisibleArea	= mMap->getViewAreaAABB();
	Sizei TileSize		= mMap->getTileSize();
	Sizei HalfTileSize = mMap->getTileSize() / 2;
	Color BaseColor	= mMap->getBaseColor();
	bool firstLight		= true;
	Vector2i Pos;

	if ( !mLights.size() )
		return;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);

		if (  firstLight || VisibleArea.intersect( Light->getAABB() ) ) {
			for ( Int32 x = start.x; x < end.x; x++ ) {
				for ( Int32 y = start.y; y < end.y; y++ ) {
					if ( firstLight ) {
						mTileColors[x][y][0]->r = BaseColor.r;
						mTileColors[x][y][0]->g = BaseColor.g;
						mTileColors[x][y][0]->b = BaseColor.b;
					}

					Pos.x = x * TileSize.x;
					Pos.y = y * TileSize.y;

					Rectf TileAABB( Pos.x, Pos.y, Pos.x + TileSize.x, Pos.y + TileSize.y );

					if ( TileAABB.intersect( Light->getAABB() ) ) {
						mTileColors[x][y][0]->assign( Light->processVertex( Pos.x + HalfTileSize.getWidth(), Pos.y + HalfTileSize.getHeight(), *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );
					}
				}
			}

			firstLight = false;
		}
	}
}

Color MapLightManager::getColorFromPos( const Vector2f& Pos ) {
	Color Col( mMap->getBaseColor() );

	if ( !mLights.size() )
		return Col;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);

		if ( Light->getAABB().contains( Pos ) ) {
			Col = Light->processVertex( Pos, Col, Col );
		}
	}

	return Col;
}

void MapLightManager::addLight( MapLight * Light ) {
	mLights.push_back( Light );

	if ( mLights.size() == 1 )
		update();
}

void MapLightManager::removeLight( MapLight * Light ) {
	mLights.remove( Light );
}

void MapLightManager::removeLight( const Vector2f& OverPos ) {
	for ( LightsList::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		MapLight * Light = (*it);

		if ( Light->getAABB().contains( OverPos ) ) {
			mLights.remove( Light );
			eeSAFE_DELETE( Light );
			break;
		}
	}
}

const Color * MapLightManager::getTileColor( const Vector2i& TilePos ) {
	eeASSERT( 1 == mNumVertex );

	if ( !mLights.size() )
		return &mMap->getBaseColor();

	return mTileColors[ TilePos.x ][ TilePos.y ][0];
}

const Color * MapLightManager::getTileColor( const Vector2i& TilePos, const Uint32& Vertex ) {
	eeASSERT( 4 == mNumVertex );

	if ( !mLights.size() )
		return &mMap->getBaseColor();

	return mTileColors[ TilePos.x ][ TilePos.y ][Vertex];
}

void MapLightManager::allocateColors() {
	Sizei Size		= mMap->getSize();
	mTileColors		= eeNewArray( Color***, Size.getWidth() );

	for ( Int32 x = 0; x < Size.x; x++ ) {
		mTileColors[x] = eeNewArray( Color**, Size.getHeight() );

		for ( Int32 y = 0; y < Size.y; y++ ) {
			mTileColors[x][y] = eeNewArray( Color*, mNumVertex );

			for ( Int32 v = 0; v < mNumVertex; v++ ) {
				mTileColors[x][y][v] = eeNew( Color, (255,255,255,255) );
			}
		}
	}
}

void MapLightManager::deallocateColors() {
	Sizei Size		= mMap->getSize();

	for ( Int32 x = 0; x < Size.x; x++ ) {
		for ( Int32 y = 0; y < Size.y; y++ ) {
			for ( Int32 v = 0; v < mNumVertex; v++ ) {
				eeSAFE_DELETE( mTileColors[x][y][v] );
			}

			eeSAFE_DELETE_ARRAY( mTileColors[x][y] );
		}

		eeSAFE_DELETE_ARRAY( mTileColors[x] );
	}

	eeSAFE_DELETE_ARRAY( mTileColors );
}

void MapLightManager::destroyLights() {
	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);
		eeSAFE_DELETE( Light );
	}
}

Uint32 MapLightManager::getCount() {
	return (Uint32)mLights.size();
}

MapLightManager::LightsList& MapLightManager::getLights() {
	return mLights;
}

MapLight * MapLightManager::getLightOver( const Vector2f& OverPos, MapLight * LightCurrent ) {
	MapLight * PivotLight = NULL;
	MapLight * LastLight	= NULL;
	MapLight * FirstLight = NULL;

	for ( LightsList::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		MapLight * Light = (*it);

		if ( Light->getAABB().contains( OverPos ) ) {
			if ( NULL == FirstLight ) {
				FirstLight = Light;
			}

			if ( NULL != LightCurrent ) {
				if ( Light != LightCurrent ) {
					PivotLight = Light;

					if ( LastLight == LightCurrent ) {
						return Light;
					}
				}
			} else {
				return Light;
			}

			LastLight = Light;
		}
	}

	if ( LastLight == LightCurrent && NULL != FirstLight ) {
		return FirstLight;
	}

	if ( NULL == PivotLight && NULL != LightCurrent && LightCurrent->getAABB().contains( OverPos ) ) {
		return LightCurrent;
	}

	return PivotLight;
}

}}
