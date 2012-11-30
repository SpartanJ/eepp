#include <eepp/gaming/clightmanager.hpp>
#include <eepp/gaming/cmap.hpp>

namespace EE { namespace Gaming {

cLightManager::cLightManager( cMap * Map, bool ByVertex ) :
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

cLightManager::~cLightManager() {
	DeallocateColors();
	DestroyLights();
}

void cLightManager::Update() {
	if ( mIsByVertex ) {
		UpdateByVertex();
	} else {
		UpdateByTile();
	}
}

const bool& cLightManager::IsByVertex() const {
	return mIsByVertex;
}

void cLightManager::UpdateByVertex() {
	eeVector2i start	= mMap->StartTile();
	eeVector2i end		= mMap->EndTile();
	eeAABB VisibleArea	= mMap->GetViewAreaAABB();
	eeSize TileSize		= mMap->TileSize();
	eeColorA BaseColor	= mMap->BaseColor();
	bool firstLight		= true;
	eeVector2i Pos;

	if ( !mLights.size() )
		return;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);

		if ( firstLight || Math::Intersect( VisibleArea, Light->GetAABB() ) ) {
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

					if ( Math::Intersect( TileAABB, Light->GetAABB() ) ) {
						if ( y > 0 )
							mTileColors[x][y][0]->Assign( *mTileColors[x][y - 1][1] );
						else
							mTileColors[x][y][0]->Assign( Light->ProcessVertex( Pos.x, Pos.y, *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );

						mTileColors[x][y][1]->Assign( Light->ProcessVertex( Pos.x, Pos.y + TileSize.Height(), *(mTileColors[x][y][1]), *(mTileColors[x][y][1]) ) );

						mTileColors[x][y][2]->Assign( Light->ProcessVertex( Pos.x + TileSize.Width(), Pos.y + TileSize.Height(), *(mTileColors[x][y][2]), *(mTileColors[x][y][2]) ) );

						if ( y > 0 )
							mTileColors[x][y][3]->Assign( *mTileColors[x][y - 1][2] );
						else
							mTileColors[x][y][3]->Assign( Light->ProcessVertex( Pos.x + TileSize.Width(), Pos.y, *(mTileColors[x][y][3]), *(mTileColors[x][y][3]) ) );
					}
				}
			}

			firstLight = false;
		}
	}
}

void cLightManager::UpdateByTile() {
	eeVector2i start	= mMap->StartTile();
	eeVector2i end		= mMap->EndTile();
	eeAABB VisibleArea	= mMap->GetViewAreaAABB();
	eeSize TileSize		= mMap->TileSize();
	eeSize HalfTileSize = mMap->TileSize() / 2;
	eeColorA BaseColor	= mMap->BaseColor();
	bool firstLight		= true;
	eeVector2i Pos;

	if ( !mLights.size() )
		return;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);

		if (  firstLight || Math::Intersect( VisibleArea, Light->GetAABB() ) ) {
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

					if ( Math::Intersect( TileAABB, Light->GetAABB() ) ) {
						mTileColors[x][y][0]->Assign( Light->ProcessVertex( Pos.x + HalfTileSize.Width(), Pos.y + HalfTileSize.Height(), *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );
					}
				}
			}

			firstLight = false;
		}
	}
}

eeColorA cLightManager::GetColorFromPos( const eeVector2f& Pos ) {
	eeColorA Col( mMap->BaseColor() );

	if ( !mLights.size() )
		return Col;

	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);

		if ( Math::Contains( Light->GetAABB(), Pos ) ) {
			Col = Light->ProcessVertex( Pos, Col, Col );
		}
	}

	return Col;
}

void cLightManager::AddLight( cLight * Light ) {
	mLights.push_back( Light );

	if ( mLights.size() == 1 )
		Update();
}

void cLightManager::RemoveLight( cLight * Light ) {
	mLights.remove( Light );
}

void cLightManager::RemoveLight( const eeVector2f& OverPos ) {
	for ( LightsList::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		cLight * Light = (*it);

		if ( Math::Contains( Light->GetAABB(), OverPos ) ) {
			mLights.remove( Light );
			eeSAFE_DELETE( Light );
			break;
		}
	}
}

const eeColorA * cLightManager::GetTileColor( const eeVector2i& TilePos ) {
	eeASSERT( 1 == mNumVertex );

	if ( !mLights.size() )
		return &mMap->BaseColor();

	return mTileColors[ TilePos.x ][ TilePos.y ][0];
}

const eeColorA * cLightManager::GetTileColor( const eeVector2i& TilePos, const Uint32& Vertex ) {
	eeASSERT( 4 == mNumVertex );

	if ( !mLights.size() )
		return &mMap->BaseColor();

	return mTileColors[ TilePos.x ][ TilePos.y ][Vertex];
}

void cLightManager::AllocateColors() {
	eeSize Size		= mMap->Size();
	mTileColors		= eeNewArray( eeColorA***, Size.Width() );

	for ( Int32 x = 0; x < Size.x; x++ ) {
		mTileColors[x] = eeNewArray( eeColorA**, Size.Height() );

		for ( Int32 y = 0; y < Size.y; y++ ) {
			mTileColors[x][y] = eeNewArray( eeColorA*, mNumVertex );

			for ( Int32 v = 0; v < mNumVertex; v++ ) {
				mTileColors[x][y][v] = eeNew( eeColorA, (255,255,255,255) );
			}
		}
	}
}

void cLightManager::DeallocateColors() {
	eeSize Size		= mMap->Size();

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

void cLightManager::DestroyLights() {
	for ( LightsList::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);
		eeSAFE_DELETE( Light );
	}
}

Uint32 cLightManager::Count() {
	return (Uint32)mLights.size();
}

cLightManager::LightsList& cLightManager::GetLights() {
	return mLights;
}

cLight * cLightManager::GetLightOver( const eeVector2f& OverPos, cLight * LightCurrent ) {
	cLight * PivotLight = NULL;
	cLight * LastLight	= NULL;
	cLight * FirstLight = NULL;

	for ( LightsList::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		cLight * Light = (*it);

		if ( Math::Contains( Light->GetAABB(), OverPos ) ) {
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

	if ( NULL == PivotLight && NULL != LightCurrent && Math::Contains( LightCurrent->GetAABB(), OverPos ) ) {
		return LightCurrent;
	}

	return PivotLight;
}

}}
