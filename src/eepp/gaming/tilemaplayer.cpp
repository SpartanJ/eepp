#include <eepp/gaming/tilemaplayer.hpp>
#include <eepp/gaming/tilemap.hpp>

#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/gl.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

TileMapLayer::TileMapLayer( TileMap * map, Sizei size, Uint32 flags, std::string name, Vector2f offset ) :
	MapLayer( map, MAP_LAYER_TILED, flags, name, offset ),
	mSize( size )
{
	AllocateLayer();
}

TileMapLayer::~TileMapLayer() {
	DeallocateLayer();
}

void TileMapLayer::Draw( const Vector2f& Offset ) {
	GlobalBatchRenderer::instance()->draw();

	GLi->pushMatrix();
	GLi->translatef( mOffset.x, mOffset.y, 0.0f );

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
						Tex->draw( x * mMap->TileSize().x, y * mMap->TileSize().y, 0 , Vector2f::One, ColorA( 255, 0, 0, 200 ) );
					}
				}
			}
		}
	}

	GlobalBatchRenderer::instance()->draw();

	GLi->popMatrix();
}

void TileMapLayer::Update() {
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

void TileMapLayer::AllocateLayer() {
	mTiles		= eeNewArray( GameObject**, mSize.getWidth() );

	for ( Int32 x = 0; x < mSize.x; x++ ) {
		mTiles[x] = eeNewArray( GameObject*, mSize.getHeight() );

		for ( Int32 y = 0; y < mSize.y; y++ ) {
			mTiles[x][y] = NULL;
		}
	}
}

void TileMapLayer::DeallocateLayer() {
	for ( Int32 x = 0; x < mSize.x; x++ ) {
		for ( Int32 y = 0; y < mSize.y; y++ ) {
			eeSAFE_DELETE( mTiles[x][y] );
		}

		eeSAFE_DELETE_ARRAY( mTiles[x] );
	}

	eeSAFE_DELETE_ARRAY( mTiles );
}

void TileMapLayer::AddGameObject( GameObject * obj, const Vector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( TilePos.x < mSize.x && TilePos.y < mSize.y ) {
		RemoveGameObject( TilePos );

		mTiles[ TilePos.x ][ TilePos.y ] = obj;

		obj->Pos( Vector2f( TilePos.x * mMap->TileSize().x, TilePos.y * mMap->TileSize().y ) );
	}
}

void TileMapLayer::RemoveGameObject( const Vector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( TilePos.x < mSize.x && TilePos.y < mSize.y ) {
		if ( NULL != mTiles[ TilePos.x ][ TilePos.y ] ) {
			eeSAFE_DELETE( mTiles[ TilePos.x ][ TilePos.y ] );
		}
	}
}

void TileMapLayer::MoveTileObject( const Vector2i& FromPos, const Vector2i& ToPos ) {
	RemoveGameObject( ToPos );

	GameObject * tObj = mTiles[ FromPos.x ][ FromPos.y ];

	mTiles[ FromPos.x ][ FromPos.y ] = NULL;

	mTiles[ ToPos.x ][ ToPos.y ] = tObj;
}

GameObject * TileMapLayer::GetGameObject( const Vector2i& TilePos ) {
	return mTiles[ TilePos.x ][ TilePos.y ];
}

const Vector2i& TileMapLayer::GetCurrentTile() const {
	return mCurTile;
}

Vector2i TileMapLayer::GetTilePosFromPos( const Vector2f& Pos ) {
	return Vector2i( ( (Int32)Pos.x + mOffset.x ) / mMap->TileSize().getWidth(), ( (Int32)Pos.y + mOffset.y ) / mMap->TileSize().getHeight() );
}

Vector2f TileMapLayer::GetPosFromTilePos( const Vector2i& TilePos ) {
	return Vector2f( TilePos.x * mMap->TileSize().getWidth() + mOffset.x, TilePos.y * mMap->TileSize().getHeight() + mOffset.y );
}

}}
