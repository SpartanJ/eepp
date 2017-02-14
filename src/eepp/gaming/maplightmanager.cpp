#include <eepp/gaming/maplightmanager.hpp>
#include <eepp/gaming/tilemap.hpp>

namespace EE { namespace Gaming {

MapLightManager::MapLightManager( TileMap * Map, bool ByVertex ) :
	mMap( Map ),
	mTileColors( NULL )
{
	mIsByVertex = ByVertex;

	if ( mIsByVertex )
		mNumVertex = 4;
	else
		mNumVertex = 1;

	AllocateColors();
}

MapLightManager::~MapLightManager() {
	DeallocateColors();
	DestroyLights();
}

void MapLightManager::Update() {
	if ( mIsByVertex ) {
		UpdateByVertex();
	} else {
		UpdateByTile();
	}
}

const bool& MapLightManager::IsByVertex() const {
	return mIsByVertex;
}

void MapLightManager::UpdateByVertex() {
	Vector2i start	= mMap->StartTile();
	Vector2i end		= mMap->EndTile();
	eeAABB VisibleArea	= mMap->GetViewAreaAABB();
	Sizei TileSize		= mMap->TileSize();
	ColorA BaseColor	= mMap->BaseColor();
	bool firstLight		= true;
	Vector2i Pos;

	if ( !mLights.size() )
		return;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);

		if ( firstLight || VisibleArea.Intersect( Light->GetAABB() ) ) {
			for ( Int32 x = start.x; x < end.x; x++ ) {
				for ( Int32 y = start.y; y < end.y; y++ ) {
					if ( firstLight ) {
						mTileColors[x][y][0]->Red = mTileColors[x][y][1]->Red = mTileColors[x][y][2]->Red = mTileColors[x][y][3]->Red = BaseColor.Red;
						mTileColors[x][y][0]->Green = mTileColors[x][y][1]->Green = mTileColors[x][y][2]->Green = mTileColors[x][y][3]->Green = BaseColor.Green;
						mTileColors[x][y][0]->Blue = mTileColors[x][y][1]->Blue = mTileColors[x][y][2]->Blue = mTileColors[x][y][3]->Blue = BaseColor.Blue;
					}

					Pos.x = x * TileSize.x;
					Pos.y = y * TileSize.y;

					eeAABB TileAABB( Pos.x, Pos.y, Pos.x + TileSize.x, Pos.y + TileSize.y );

					if ( TileAABB.Intersect( Light->GetAABB() ) ) {
						if ( y > 0 )
							mTileColors[x][y][0]->assign( *mTileColors[x][y - 1][1] );
						else
							mTileColors[x][y][0]->assign( Light->ProcessVertex( Pos.x, Pos.y, *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );

						mTileColors[x][y][1]->assign( Light->ProcessVertex( Pos.x, Pos.y + TileSize.Height(), *(mTileColors[x][y][1]), *(mTileColors[x][y][1]) ) );

						mTileColors[x][y][2]->assign( Light->ProcessVertex( Pos.x + TileSize.Width(), Pos.y + TileSize.Height(), *(mTileColors[x][y][2]), *(mTileColors[x][y][2]) ) );

						if ( y > 0 )
							mTileColors[x][y][3]->assign( *mTileColors[x][y - 1][2] );
						else
							mTileColors[x][y][3]->assign( Light->ProcessVertex( Pos.x + TileSize.Width(), Pos.y, *(mTileColors[x][y][3]), *(mTileColors[x][y][3]) ) );
					}
				}
			}

			firstLight = false;
		}
	}
}

void MapLightManager::UpdateByTile() {
	Vector2i start	= mMap->StartTile();
	Vector2i end		= mMap->EndTile();
	eeAABB VisibleArea	= mMap->GetViewAreaAABB();
	Sizei TileSize		= mMap->TileSize();
	Sizei HalfTileSize = mMap->TileSize() / 2;
	ColorA BaseColor	= mMap->BaseColor();
	bool firstLight		= true;
	Vector2i Pos;

	if ( !mLights.size() )
		return;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);

		if (  firstLight || VisibleArea.Intersect( Light->GetAABB() ) ) {
			for ( Int32 x = start.x; x < end.x; x++ ) {
				for ( Int32 y = start.y; y < end.y; y++ ) {
					if ( firstLight ) {
						mTileColors[x][y][0]->Red = BaseColor.Red;
						mTileColors[x][y][0]->Green = BaseColor.Green;
						mTileColors[x][y][0]->Blue = BaseColor.Blue;
					}

					Pos.x = x * TileSize.x;
					Pos.y = y * TileSize.y;

					eeAABB TileAABB( Pos.x, Pos.y, Pos.x + TileSize.x, Pos.y + TileSize.y );

					if ( TileAABB.Intersect( Light->GetAABB() ) ) {
						mTileColors[x][y][0]->assign( Light->ProcessVertex( Pos.x + HalfTileSize.Width(), Pos.y + HalfTileSize.Height(), *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );
					}
				}
			}

			firstLight = false;
		}
	}
}

ColorA MapLightManager::GetColorFromPos( const Vector2f& Pos ) {
	ColorA Col( mMap->BaseColor() );

	if ( !mLights.size() )
		return Col;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);

		if ( Light->GetAABB().Contains( Pos ) ) {
			Col = Light->ProcessVertex( Pos, Col, Col );
		}
	}

	return Col;
}

void MapLightManager::AddLight( MapLight * Light ) {
	mLights.push_back( Light );

	if ( mLights.size() == 1 )
		Update();
}

void MapLightManager::RemoveLight( MapLight * Light ) {
	mLights.remove( Light );
}

void MapLightManager::RemoveLight( const Vector2f& OverPos ) {
	for ( LightsList::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		MapLight * Light = (*it);

		if ( Light->GetAABB().Contains( OverPos ) ) {
			mLights.remove( Light );
			eeSAFE_DELETE( Light );
			break;
		}
	}
}

const ColorA * MapLightManager::GetTileColor( const Vector2i& TilePos ) {
	eeASSERT( 1 == mNumVertex );

	if ( !mLights.size() )
		return &mMap->BaseColor();

	return mTileColors[ TilePos.x ][ TilePos.y ][0];
}

const ColorA * MapLightManager::GetTileColor( const Vector2i& TilePos, const Uint32& Vertex ) {
	eeASSERT( 4 == mNumVertex );

	if ( !mLights.size() )
		return &mMap->BaseColor();

	return mTileColors[ TilePos.x ][ TilePos.y ][Vertex];
}

void MapLightManager::AllocateColors() {
	Sizei Size		= mMap->Size();
	mTileColors		= eeNewArray( ColorA***, Size.Width() );

	for ( Int32 x = 0; x < Size.x; x++ ) {
		mTileColors[x] = eeNewArray( ColorA**, Size.Height() );

		for ( Int32 y = 0; y < Size.y; y++ ) {
			mTileColors[x][y] = eeNewArray( ColorA*, mNumVertex );

			for ( Int32 v = 0; v < mNumVertex; v++ ) {
				mTileColors[x][y][v] = eeNew( ColorA, (255,255,255,255) );
			}
		}
	}
}

void MapLightManager::DeallocateColors() {
	Sizei Size		= mMap->Size();

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

void MapLightManager::DestroyLights() {
	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		MapLight * Light = (*it);
		eeSAFE_DELETE( Light );
	}
}

Uint32 MapLightManager::Count() {
	return (Uint32)mLights.size();
}

MapLightManager::LightsList& MapLightManager::GetLights() {
	return mLights;
}

MapLight * MapLightManager::GetLightOver( const Vector2f& OverPos, MapLight * LightCurrent ) {
	MapLight * PivotLight = NULL;
	MapLight * LastLight	= NULL;
	MapLight * FirstLight = NULL;

	for ( LightsList::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		MapLight * Light = (*it);

		if ( Light->GetAABB().Contains( OverPos ) ) {
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

	if ( NULL == PivotLight && NULL != LightCurrent && LightCurrent->GetAABB().Contains( OverPos ) ) {
		return LightCurrent;
	}

	return PivotLight;
}

}}
