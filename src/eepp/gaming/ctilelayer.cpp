#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/gaming/cmap.hpp>

#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cTileLayer::cTileLayer( cMap * map, eeSize size, Uint32 flags, std::string name, eeVector2f offset ) :
	cLayer( map, MAP_LAYER_TILED, flags, name, offset ),
	mSize( size )
{
	AllocateLayer();
}

cTileLayer::~cTileLayer() {
	DeallocateLayer();
}

void cTileLayer::cTileLayer::Draw( const eeVector2f &Offset ) {
	cGlobalBatchRenderer::instance()->Draw();

	GLi->PushMatrix();
	GLi->Translatef( mOffset.x, mOffset.y, 0.0f );

	eeVector2i start = mMap->StartTile();
	eeVector2i end = mMap->EndTile();

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			mCurTile.x = x;
			mCurTile.y = y;

			if ( NULL != mTiles[x][y] ) {
				mTiles[x][y]->Draw();
			}
		}
	}

	cTexture * Tex = mMap->GetBlankTileTexture();

	if ( mMap->ShowBlocked() && NULL != Tex ) {
		for ( Int32 x = start.x; x < end.x; x++ ) {
			for ( Int32 y = start.y; y < end.y; y++ ) {
				if ( NULL != mTiles[x][y] ) {
					if ( mTiles[x][y]->Blocked() ) {
						Tex->Draw( x * mMap->TileSize().x, y * mMap->TileSize().y, 0 , 1, eeColorA( 255, 0, 0, 200 ) );
					}
				}
			}
		}
	}

	cGlobalBatchRenderer::instance()->Draw();

	GLi->PopMatrix();
}

void cTileLayer::Update() {
	eeVector2i start = mMap->StartTile();
	eeVector2i end = mMap->EndTile();

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			mCurTile.x = x;
			mCurTile.y = y;

			if ( NULL != mTiles[x][y] ) {
				mTiles[x][y]->Update();
			}
		}
	}
}

void cTileLayer::AllocateLayer() {
	mTiles		= eeNewArray( cGameObject**, mSize.Width() );

	for ( Int32 x = 0; x < mSize.x; x++ ) {
		mTiles[x] = eeNewArray( cGameObject*, mSize.Height() );

		for ( Int32 y = 0; y < mSize.y; y++ ) {
			mTiles[x][y] = NULL;
		}
	}
}

void cTileLayer::DeallocateLayer() {
	for ( Int32 x = 0; x < mSize.x; x++ ) {
		for ( Int32 y = 0; y < mSize.y; y++ ) {
			eeSAFE_DELETE( mTiles[x][y] );
		}

		eeSAFE_DELETE_ARRAY( mTiles[x] );
	}

	eeSAFE_DELETE_ARRAY( mTiles );
}

void cTileLayer::AddGameObject( cGameObject * obj, const eeVector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	RemoveGameObject( TilePos );

	mTiles[ TilePos.x ][ TilePos.y ] = obj;

	obj->Pos( eeVector2f( TilePos.x * mMap->TileSize().x, TilePos.y * mMap->TileSize().y ) );
}

void cTileLayer::RemoveGameObject( const eeVector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( NULL != mTiles[ TilePos.x ][ TilePos.y ] ) {
		eeSAFE_DELETE( mTiles[ TilePos.x ][ TilePos.y ] );
	}
}

void cTileLayer::MoveTileObject( const eeVector2i& FromPos, const eeVector2i& ToPos ) {
	RemoveGameObject( ToPos );

	cGameObject * tObj = mTiles[ FromPos.x ][ FromPos.y ];

	mTiles[ FromPos.x ][ FromPos.y ] = NULL;

	mTiles[ ToPos.x ][ ToPos.y ] = tObj;
}

cGameObject * cTileLayer::GetGameObject( const eeVector2i& TilePos ) {
	return mTiles[ TilePos.x ][ TilePos.y ];
}

const eeVector2i& cTileLayer::GetCurrentTile() const {
	return mCurTile;
}

eeVector2i cTileLayer::GetTilePosFromPos( const eeVector2f& Pos ) {
	return eeVector2i( ( (Int32)Pos.x + mOffset.x ) / mMap->TileSize().Width(), ( (Int32)Pos.y + mOffset.y ) / mMap->TileSize().Height() );
}

eeVector2f cTileLayer::GetPosFromTilePos( const eeVector2i& TilePos ) {
	return eeVector2f( TilePos.x * mMap->TileSize().Width() + mOffset.x, TilePos.y * mMap->TileSize().Height() + mOffset.y );
}

}}
