#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/gaming/cmap.hpp>

#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/gl.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cTileLayer::cTileLayer( cMap * map, Sizei size, Uint32 flags, std::string name, Vector2f offset ) :
	cLayer( map, MAP_LAYER_TILED, flags, name, offset ),
	mSize( size )
{
	AllocateLayer();
}

cTileLayer::~cTileLayer() {
	DeallocateLayer();
}

void cTileLayer::Draw( const Vector2f& Offset ) {
	GlobalBatchRenderer::instance()->Draw();

	GLi->PushMatrix();
	GLi->Translatef( mOffset.x, mOffset.y, 0.0f );

	Vector2i start = mMap->StartTile();
	Vector2i end = mMap->EndTile();

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			mCurTile.x = x;
			mCurTile.y = y;

			if ( NULL != mTiles[x][y] ) {
				mTiles[x][y]->Draw();
			}
		}
	}

	Texture * Tex = mMap->GetBlankTileTexture();

	if ( mMap->ShowBlocked() && NULL != Tex ) {
		for ( Int32 x = start.x; x < end.x; x++ ) {
			for ( Int32 y = start.y; y < end.y; y++ ) {
				if ( NULL != mTiles[x][y] ) {
					if ( mTiles[x][y]->Blocked() ) {
						Tex->Draw( x * mMap->TileSize().x, y * mMap->TileSize().y, 0 , Vector2f::One, ColorA( 255, 0, 0, 200 ) );
					}
				}
			}
		}
	}

	GlobalBatchRenderer::instance()->Draw();

	GLi->PopMatrix();
}

void cTileLayer::Update() {
	Vector2i start = mMap->StartTile();
	Vector2i end = mMap->EndTile();

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

void cTileLayer::AddGameObject( cGameObject * obj, const Vector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( TilePos.x < mSize.x && TilePos.y < mSize.y ) {
		RemoveGameObject( TilePos );

		mTiles[ TilePos.x ][ TilePos.y ] = obj;

		obj->Pos( Vector2f( TilePos.x * mMap->TileSize().x, TilePos.y * mMap->TileSize().y ) );
	}
}

void cTileLayer::RemoveGameObject( const Vector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( TilePos.x < mSize.x && TilePos.y < mSize.y ) {
		if ( NULL != mTiles[ TilePos.x ][ TilePos.y ] ) {
			eeSAFE_DELETE( mTiles[ TilePos.x ][ TilePos.y ] );
		}
	}
}

void cTileLayer::MoveTileObject( const Vector2i& FromPos, const Vector2i& ToPos ) {
	RemoveGameObject( ToPos );

	cGameObject * tObj = mTiles[ FromPos.x ][ FromPos.y ];

	mTiles[ FromPos.x ][ FromPos.y ] = NULL;

	mTiles[ ToPos.x ][ ToPos.y ] = tObj;
}

cGameObject * cTileLayer::GetGameObject( const Vector2i& TilePos ) {
	return mTiles[ TilePos.x ][ TilePos.y ];
}

const Vector2i& cTileLayer::GetCurrentTile() const {
	return mCurTile;
}

Vector2i cTileLayer::GetTilePosFromPos( const Vector2f& Pos ) {
	return Vector2i( ( (Int32)Pos.x + mOffset.x ) / mMap->TileSize().Width(), ( (Int32)Pos.y + mOffset.y ) / mMap->TileSize().Height() );
}

Vector2f cTileLayer::GetPosFromTilePos( const Vector2i& TilePos ) {
	return Vector2f( TilePos.x * mMap->TileSize().Width() + mOffset.x, TilePos.y * mMap->TileSize().Height() + mOffset.y );
}

}}
