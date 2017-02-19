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
	allocateLayer();
}

TileMapLayer::~TileMapLayer() {
	deallocateLayer();
}

void TileMapLayer::draw( const Vector2f& Offset ) {
	GlobalBatchRenderer::instance()->draw();

	GLi->pushMatrix();
	GLi->translatef( mOffset.x, mOffset.y, 0.0f );

	Vector2i start = mMap->getStartTile();
	Vector2i end = mMap->getEndTile();

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			mCurTile.x = x;
			mCurTile.y = y;

			if ( NULL != mTiles[x][y] ) {
				mTiles[x][y]->draw();
			}
		}
	}

	Texture * Tex = mMap->getBlankTileTexture();

	if ( mMap->getShowBlocked() && NULL != Tex ) {
		for ( Int32 x = start.x; x < end.x; x++ ) {
			for ( Int32 y = start.y; y < end.y; y++ ) {
				if ( NULL != mTiles[x][y] ) {
					if ( mTiles[x][y]->isBlocked() ) {
						Tex->draw( x * mMap->getTileSize().x, y * mMap->getTileSize().y, 0 , Vector2f::One, ColorA( 255, 0, 0, 200 ) );
					}
				}
			}
		}
	}

	GlobalBatchRenderer::instance()->draw();

	GLi->popMatrix();
}

void TileMapLayer::update() {
	Vector2i start = mMap->getStartTile();
	Vector2i end = mMap->getEndTile();

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			mCurTile.x = x;
			mCurTile.y = y;

			if ( NULL != mTiles[x][y] ) {
				mTiles[x][y]->update();
			}
		}
	}
}

void TileMapLayer::allocateLayer() {
	mTiles		= eeNewArray( GameObject**, mSize.getWidth() );

	for ( Int32 x = 0; x < mSize.x; x++ ) {
		mTiles[x] = eeNewArray( GameObject*, mSize.getHeight() );

		for ( Int32 y = 0; y < mSize.y; y++ ) {
			mTiles[x][y] = NULL;
		}
	}
}

void TileMapLayer::deallocateLayer() {
	for ( Int32 x = 0; x < mSize.x; x++ ) {
		for ( Int32 y = 0; y < mSize.y; y++ ) {
			eeSAFE_DELETE( mTiles[x][y] );
		}

		eeSAFE_DELETE_ARRAY( mTiles[x] );
	}

	eeSAFE_DELETE_ARRAY( mTiles );
}

void TileMapLayer::addGameObject( GameObject * obj, const Vector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( TilePos.x < mSize.x && TilePos.y < mSize.y ) {
		removeGameObject( TilePos );

		mTiles[ TilePos.x ][ TilePos.y ] = obj;

		obj->setPosition( Vector2f( TilePos.x * mMap->getTileSize().x, TilePos.y * mMap->getTileSize().y ) );
	}
}

void TileMapLayer::removeGameObject( const Vector2i& TilePos ) {
	eeASSERT( TilePos.x >= 0 && TilePos.y >= 0 );

	if ( TilePos.x < mSize.x && TilePos.y < mSize.y ) {
		if ( NULL != mTiles[ TilePos.x ][ TilePos.y ] ) {
			eeSAFE_DELETE( mTiles[ TilePos.x ][ TilePos.y ] );
		}
	}
}

void TileMapLayer::moveTileObject( const Vector2i& FromPos, const Vector2i& ToPos ) {
	removeGameObject( ToPos );

	GameObject * tObj = mTiles[ FromPos.x ][ FromPos.y ];

	mTiles[ FromPos.x ][ FromPos.y ] = NULL;

	mTiles[ ToPos.x ][ ToPos.y ] = tObj;
}

GameObject * TileMapLayer::getGameObject( const Vector2i& TilePos ) {
	return mTiles[ TilePos.x ][ TilePos.y ];
}

const Vector2i& TileMapLayer::getCurrentTile() const {
	return mCurTile;
}

Vector2i TileMapLayer::getTilePosFromPos( const Vector2f& Pos ) {
	return Vector2i( ( (Int32)Pos.x + mOffset.x ) / mMap->getTileSize().getWidth(), ( (Int32)Pos.y + mOffset.y ) / mMap->getTileSize().getHeight() );
}

Vector2f TileMapLayer::getPosFromTilePos( const Vector2i& TilePos ) {
	return Vector2f( TilePos.x * mMap->getTileSize().getWidth() + mOffset.x, TilePos.y * mMap->getTileSize().getHeight() + mOffset.y );
}

}}
