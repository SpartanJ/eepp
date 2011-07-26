#include "clightmanager.hpp"
#include "cmap.hpp"

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

	for ( std::list<cLight*>::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);

		if ( Intersect( VisibleArea, Light->GetAABB() ) ) {
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

					if ( Intersect( TileAABB, Light->GetAABB() ) ) {
						mTileColors[x][y][0]->Assign( Light->ProcessVertex( Pos.x, Pos.y, *(mTileColors[x][y][0]), *(mTileColors[x][y][0]) ) );
						mTileColors[x][y][1]->Assign( Light->ProcessVertex( Pos.x, Pos.y + TileSize.Height(), *(mTileColors[x][y][1]), *(mTileColors[x][y][1]) ) );
						mTileColors[x][y][2]->Assign( Light->ProcessVertex( Pos.x + TileSize.Width(), Pos.y + TileSize.Height(), *(mTileColors[x][y][2]), *(mTileColors[x][y][2]) ) );
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

	for ( std::list<cLight*>::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);

		if ( Intersect( VisibleArea, Light->GetAABB() ) ) {
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

					if ( Intersect( TileAABB, Light->GetAABB() ) ) {
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

	for ( std::list<cLight*>::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);

		if ( Contains( Light->GetAABB(), Pos ) ) {
			Col = Light->ProcessVertex( Pos, Col, Col );
		}
	}

	return Col;
}

void cLightManager::AddLight( cLight * Light ) {
	mLights.push_back( Light );
}

void cLightManager::RemoveLight( const eeVector2f& OverPos ) {
	for ( std::list<cLight*>::reverse_iterator it = mLights.rbegin(); it != mLights.rend(); it++ ) {
		cLight * Light = (*it);

		if ( Contains( Light->GetAABB(), OverPos ) ) {
			mLights.remove( Light );
			eeSAFE_DELETE( Light );
			break;
		}
	}
}

eeColorA * cLightManager::GetTileColor( const eeVector2i& TilePos ) {
	eeASSERT( 1 == mNumVertex );
	return mTileColors[ TilePos.x ][ TilePos.y ][0];
}

eeColorA * cLightManager::GetTileColor( const eeVector2i& TilePos, const Uint32& Vertex ) {
	eeASSERT( 4 == mNumVertex );
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
	for ( std::list<cLight*>::iterator it = mLights.begin(); it != mLights.end(); it++ ) {
		cLight * Light = (*it);
		eeSAFE_DELETE( Light );
	}
}

}}
