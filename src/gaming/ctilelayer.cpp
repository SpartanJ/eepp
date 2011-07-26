#include "ctilelayer.hpp"
#include "cmap.hpp"

#include "../graphics/cglobalbatchrenderer.hpp"
#include "../graphics/renderer/cgl.hpp"
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

	GLi->LoadIdentity();
	GLi->PushMatrix();
	GLi->Translatef( mOffset.x + Offset.x, mOffset.y + Offset.y, 0.0f );

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

cGameObject * cTileLayer::GetGameObject( const eeVector2i& TilePos ) {
	return mTiles[ TilePos.x ][ TilePos.y ];
}

const eeVector2i& cTileLayer::GetCurrentTile() const {
	return mCurTile;
}

}}
