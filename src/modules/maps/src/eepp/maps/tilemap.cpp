#include <algorithm>
#include <eepp/maps/gameobjectobject.hpp>
#include <eepp/maps/gameobjectpolygon.hpp>
#include <eepp/maps/gameobjectpolyline.hpp>
#include <eepp/maps/gameobjectsprite.hpp>
#include <eepp/maps/gameobjecttextureregion.hpp>
#include <eepp/maps/gameobjecttextureregionex.hpp>
#include <eepp/maps/gameobjectvirtual.hpp>
#include <eepp/maps/mapobjectlayer.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/tilemaplayer.hpp>

#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/virtualfilesystem.hpp>
using namespace EE::Graphics;

#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Maps {

TileMap::TileMap() :
	mWindow( Engine::instance()->getCurrentWindow() ),
	mLayers( NULL ),
	mFlags( 0 ),
	mMaxLayers( 0 ),
	mLayerCount( 0 ),
	mViewSize( 800, 600 ),
	mBaseColor( 255, 255, 255, 255 ),
	mTileTex( NULL ),
	mLightManager( NULL ),
	mData( NULL ),
	mTileOverColor( 255, 0, 0, 200 ),
	mBackColor( 204, 204, 204, 255 ),
	mGridLinesColor( 255, 255, 255, 255 ),
	mBackAlpha( 255 ),
	mMouseOver( false ),
	mScale( 1 ),
	mOffscale( 1, 1 ),
	mLastObjId( 0 ),
	mForcedHeaders( NULL ) {
	setViewSize( mViewSize );
}

TileMap::~TileMap() {
	deleteLayers();
	disableForcedHeaders();
}

void TileMap::reset() {
	deleteLayers();

	mWindow = NULL;
	mLayers = NULL;
	mData = NULL;
	mFlags = 0;
	mMaxLayers = 0;
	mMouseOver = false;
	mViewSize = Sizef( 800, 600 );
	mBaseColor = Color( 255, 255, 255, 255 );
}

void TileMap::forceHeadersOnLoad( Sizei mapSize, Sizei tileSize, Uint32 numLayers, Uint32 flags ) {
	disableForcedHeaders();
	mForcedHeaders = eeNew( ForcedHeaders, ( mapSize, tileSize, numLayers, flags ) );
}

void TileMap::disableForcedHeaders() {
	eeSAFE_DELETE( mForcedHeaders );
}

void TileMap::deleteLayers() {
	eeSAFE_DELETE( mLightManager );

	for ( Uint32 i = 0; i < mLayerCount; i++ )
		eeSAFE_DELETE( mLayers[i] );

	eeSAFE_DELETE_ARRAY( mLayers );

	mLayerCount = 0;
}

void TileMap::create( Sizei Size, Uint32 MaxLayers, Sizei TileSize, Uint32 Flags, Sizef viewSize,
					  EE::Window::Window* Window ) {
	reset();

	mWindow = Window;

	if ( NULL == mWindow )
		mWindow = Engine::instance()->getCurrentWindow();

	mFlags = Flags;
	mMaxLayers = MaxLayers;
	mSize = Size;
	mTileSize = TileSize;
	mPixelSize = Size * TileSize;
	mLayers = eeNewArray( MapLayer*, mMaxLayers );

	if ( getLightsEnabled() )
		createLightManager();

	for ( Uint32 i = 0; i < mMaxLayers; i++ )
		mLayers[i] = NULL;

	setViewSize( viewSize );

	createEmptyTile();
}

void TileMap::createLightManager() {
	eeSAFE_DELETE( mLightManager );
	mLightManager =
		eeNew( MapLightManager, ( this, ( mFlags & MAP_FLAG_LIGHTS_BYVERTEX ) ? true : false ) );
}

void TileMap::createEmptyTile() {
	//! I create a texture representing an empty tile to render instead of rendering with primitives
	//! because is a lot faster, at least with NVIDIA GPUs.
	TextureFactory* TF = TextureFactory::instance();

	std::string tileName( String::format( "maptile-%dx%d-%u", mTileSize.getWidth(),
										  mTileSize.getHeight(), mGridLinesColor.getValue() ) );

	Texture* Tex = TF->getByName( tileName );

	if ( NULL == Tex ) {
		Uint32 x, y;
		Color Col( mGridLinesColor );

		Image Img( mTileSize.getWidth(), mTileSize.getHeight(), 4 );

		Img.fillWithColor( Color( 0, 0, 0, 0 ) );

		for ( x = 0; x < Img.getWidth(); x++ ) {
			Img.setPixel( x, 0, Col );
			Img.setPixel( x, mTileSize.y - 1, Col );
		}

		for ( y = 0; y < Img.getHeight(); y++ ) {
			Img.setPixel( 0, y, Col );
			Img.setPixel( mTileSize.x - 1, y, Col );
		}

		mTileTex = TF->loadFromPixels( Img.getPixelsPtr(), Img.getWidth(), Img.getHeight(),
									   Img.getChannels(), true, Texture::ClampMode::ClampToEdge,
									   false, false, tileName );
	} else {
		mTileTex = Tex;
	}
}

MapLayer* TileMap::addLayer( Uint32 Type, Uint32 flags, std::string name ) {
	eeASSERT( NULL != mLayers );

	if ( mLayerCount >= mMaxLayers )
		return NULL;

	switch ( Type ) {
		case MAP_LAYER_TILED:
			mLayers[mLayerCount] = eeNew( TileMapLayer, ( this, mSize, flags, name ) );
			break;
		case MAP_LAYER_OBJECT:
			mLayers[mLayerCount] = eeNew( MapObjectLayer, ( this, flags, name ) );
			break;
		default:
			return NULL;
	}

	mLayerCount++;

	return mLayers[mLayerCount - 1];
}

MapLayer* TileMap::getLayer( Uint32 index ) {
	eeASSERT( index < mLayerCount );
	return mLayers[index];
}

MapLayer* TileMap::getLayerByHash( String::HashType hash ) {
	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->getId() == hash )
			return mLayers[i];
	}

	return NULL;
}

Uint32 TileMap::getLayerIndex( MapLayer* Layer ) {
	if ( NULL != Layer ) {
		for ( Uint32 i = 0; i < mLayerCount; i++ ) {
			if ( mLayers[i] == Layer )
				return i;
		}
	}

	return EE_MAP_LAYER_UNKNOWN;
}

MapLayer* TileMap::getLayer( const std::string& name ) {
	return getLayerByHash( String::hash( name ) );
}

void TileMap::draw() {
	GlobalBatchRenderer::instance()->draw();

	if ( getClippedArea() ) {
		GLi->getClippingMask()->clipPlaneEnable( mScreenPos.x, mScreenPos.y, mViewSize.x,
												 mViewSize.y );
	}

	if ( getDrawBackground() ) {
		Primitives P;

		Uint8 Alpha = static_cast<Uint8>( (Float)mBackColor.a * ( (Float)mBackAlpha / 255.f ) );

		P.setColor( Color( mBackColor.r, mBackColor.g, mBackColor.b, Alpha ) );
		P.drawRectangle(
			Rectf( Vector2f( mScreenPos.x, mScreenPos.y ), Sizef( mViewSize.x, mViewSize.y ) ), 0.f,
			Vector2f::One );
		P.setColor( Color( 255, 255, 255, 255 ) );
	}

	GLi->pushMatrix();
	GLi->translatef( ( Float ) static_cast<Int32>( mScreenPos.x + mOffset.x ),
					 ( Float ) static_cast<Int32>( mScreenPos.y + mOffset.y ), 0 );
	GLi->scalef( mScale, mScale, 0 );

	gridDraw();

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->isVisible() )
			mLayers[i]->draw();
	}

	mouseOverDraw();

	if ( mDrawCb )
		mDrawCb();

	GlobalBatchRenderer::instance()->draw();

	GLi->popMatrix();

	if ( getClippedArea() ) {
		GLi->getClippingMask()->clipPlaneDisable();
	}
}

void TileMap::mouseOverDraw() {
	if ( !getDrawTileOver() || NULL == mTileTex )
		return;

	mTileTex->draw( mMouseOverTileFinal.x * mTileSize.x, mMouseOverTileFinal.y * mTileSize.y, 0,
					Vector2f::One, mTileOverColor );
}

void TileMap::gridDraw() {
	if ( !getDrawGrid() )
		return;

	if ( 0 == mSize.x || 0 == mSize.y || NULL == mTileTex )
		return;

	GlobalBatchRenderer::instance()->draw();

	Vector2i start = getStartTile();
	Vector2i end = getEndTile();

	Float tx, ty;
	Color TileTexCol( 255, 255, 255, mBackAlpha );

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			tx = x * mTileSize.x;

			ty = y * mTileSize.y;

			if ( getLightsEnabled() ) {
				Vector2i TPos( x, y );

				if ( mLightManager->isByVertex() ) {
					Color TileTexCol0( *mLightManager->getTileColor( TPos, 0 ) );
					Color TileTexCol1( *mLightManager->getTileColor( TPos, 1 ) );
					Color TileTexCol2( *mLightManager->getTileColor( TPos, 2 ) );
					Color TileTexCol3( *mLightManager->getTileColor( TPos, 3 ) );

					TileTexCol0.a = TileTexCol1.a = TileTexCol2.a = TileTexCol3.a = mBackAlpha;

					mTileTex->drawEx( tx, ty, 0, 0, 0, Vector2f::One, TileTexCol0, TileTexCol1,
									  TileTexCol2, TileTexCol3 );
				} else {
					TileTexCol = *mLightManager->getTileColor( TPos );
					TileTexCol.a = mBackAlpha;

					mTileTex->draw( tx, ty, 0, Vector2f::One, TileTexCol );
				}
			} else {
				mTileTex->draw( tx, ty, 0, Vector2f::One, TileTexCol );
			}
		}
	}

	GlobalBatchRenderer::instance()->draw();
}

const bool& TileMap::isMouseOver() const {
	return mMouseOver;
}

void TileMap::getMouseOverTile() {
	Vector2i mouse = mWindow->getInput()->getMousePos();

	Vector2i MapPos( static_cast<Float>( mouse.x - mScreenPos.x - mOffset.x ) / mScale,
					 static_cast<Float>( mouse.y - mScreenPos.y - mOffset.y ) / mScale );

	mMouseOver =
		!( MapPos.x < 0 || MapPos.y < 0 || MapPos.x > mPixelSize.x || MapPos.y > mPixelSize.y );

	MapPos.x = eemax( MapPos.x, 0 );
	MapPos.y = eemax( MapPos.y, 0 );
	MapPos.x = eemin( MapPos.x, mPixelSize.x );
	MapPos.y = eemin( MapPos.y, mPixelSize.y );

	mMouseOverTile.x = MapPos.x / mTileSize.getWidth();
	mMouseOverTile.y = MapPos.y / mTileSize.getHeight();

	// Clamped pos
	mMouseOverTileFinal.x = eemin( mMouseOverTile.x, mSize.getWidth() - 1 );
	mMouseOverTileFinal.y = eemin( mMouseOverTile.y, mSize.getHeight() - 1 );
	mMouseOverTileFinal.x = eemax( mMouseOverTileFinal.x, 0 );
	mMouseOverTileFinal.y = eemax( mMouseOverTileFinal.y, 0 );

	mMouseMapPos = MapPos;
}

void TileMap::calcTilesClip() {
	if ( mTileSize.x > 0 && mTileSize.y > 0 ) {
		Vector2f ffoff( mOffset );
		Vector2i foff( (Int32)ffoff.x, (Int32)ffoff.y );

		mStartTile.x = -foff.x / ( mTileSize.x * mScale ) - mExtraTiles.x;
		mStartTile.y = -foff.y / ( mTileSize.y * mScale ) - mExtraTiles.y;

		if ( mStartTile.x < 0 )
			mStartTile.x = 0;

		if ( mStartTile.y < 0 )
			mStartTile.y = 0;

		mEndTile.x = mStartTile.x +
					 Math::roundUp( (Float)mViewSize.x / ( (Float)mTileSize.x * mScale ) ) + 1 +
					 mExtraTiles.x;
		mEndTile.y = mStartTile.y +
					 Math::roundUp( (Float)mViewSize.y / ( (Float)mTileSize.y * mScale ) ) + 1 +
					 mExtraTiles.y;

		if ( mEndTile.x > mSize.x )
			mEndTile.x = mSize.x;

		if ( mEndTile.y > mSize.y )
			mEndTile.y = mSize.y;
	}
}

void TileMap::clamp() {
	if ( !getClampBorders() )
		return;

	if ( mOffset.x > 0 )
		mOffset.x = 0;

	if ( mOffset.y > 0 )
		mOffset.y = 0;

	Vector2f totSize( mTileSize.x * mSize.x * mScale, mTileSize.y * mSize.y * mScale );

	if ( -mOffset.x + mViewSize.x > totSize.x )
		mOffset.x = -( totSize.x - mViewSize.x );

	if ( -mOffset.y + mViewSize.y > totSize.y )
		mOffset.y = -( totSize.y - mViewSize.y );

	if ( totSize.x < mViewSize.x )
		mOffset.x = 0;

	if ( totSize.y < mViewSize.y )
		mOffset.y = 0;

	totSize.x = (Int32)( (Float)( mTileSize.x * mSize.x ) * mScale );
	totSize.y = (Int32)( (Float)( mTileSize.y * mSize.y ) * mScale );

	if ( -mOffset.x + mViewSize.x > totSize.x )
		mOffset.x = -( totSize.x - mViewSize.x );

	if ( -mOffset.y + mViewSize.y > totSize.y )
		mOffset.y = -( totSize.y - mViewSize.y );

	if ( totSize.x < mViewSize.x )
		mOffset.x = 0;

	if ( totSize.y < mViewSize.y )
		mOffset.y = 0;
}

void TileMap::setOffset( const Vector2f& offset ) {
	mOffset = offset;

	clamp();

	calcTilesClip();
}

Vector2i TileMap::getMaxOffset() {
	Vector2i v( ( mTileSize.x * mSize.x * mScale ) - mViewSize.x,
				( mTileSize.y * mSize.y * mScale ) - mViewSize.y );

	eemax( 0, v.x );
	eemax( 0, v.y );

	return v;
}

const Float& TileMap::getScale() const {
	return mScale;
}

void TileMap::setScale( const Float& scale ) {
	mScale = scale;

	setOffset( mOffset );
}

void TileMap::updateScreenAABB() {
	mScreenAABB = Rectf( -mOffset.x, -mOffset.y, -mOffset.x + mViewSize.getWidth(),
						 -mOffset.y + mViewSize.getHeight() );
}

const Rectf& TileMap::getViewAreaAABB() const {
	return mScreenAABB;
}

void TileMap::update() {
	getMouseOverTile();

	updateScreenAABB();

	if ( NULL != mLightManager )
		mLightManager->update();

	for ( Uint32 i = 0; i < mLayerCount; i++ )
		mLayers[i]->update( mWindow->getElapsed() );

	if ( mUpdateCb )
		mUpdateCb();
}

const Sizef& TileMap::getViewSize() const {
	return mViewSize;
}

const Vector2i& TileMap::getMouseTilePos() const {
	return mMouseOverTileFinal;
}

const Vector2i& TileMap::getRealMouseTilePos() const {
	return mMouseOverTile;
}

const Vector2i& TileMap::getMouseMapPos() const {
	return mMouseMapPos;
}

Vector2f TileMap::getMouseMapPosf() const {
	return Vector2f( (Float)mMouseMapPos.x, (Float)mMouseMapPos.y );
}

Vector2i TileMap::getMouseTilePosCoords() {
	return getTileCoords( getMouseTilePos() );
}

Vector2f TileMap::getMouseTilePosCoordsf() {
	return getTileCoords( Vector2f( getMouseTilePos().x, getMouseTilePos().y ) );
}

Vector2i TileMap::getTileCoords( const Vector2i& TilePos ) {
	return ( TilePos * mTileSize );
}

Vector2f TileMap::getTileCoords( const Vector2f& TilePos ) {
	return ( TilePos * Vector2f( mTileSize.x, mTileSize.y ) );
}

void TileMap::setViewSize( const Sizef& viewSize ) {
	mViewSize = viewSize;

	clamp();

	calcTilesClip();
}

const Vector2i& TileMap::getPosition() const {
	return mScreenPos;
}

void TileMap::setPosition( const Vector2i& position ) {
	mScreenPos = position;
}

const Vector2f& TileMap::getOffset() const {
	return mOffset;
}

const Vector2i& TileMap::getStartTile() const {
	return mStartTile;
}

const Vector2i& TileMap::getEndTile() const {
	return mEndTile;
}

void TileMap::setExtraTiles( const Vector2i& extra ) {
	mExtraTiles = extra;
}

const Vector2i& TileMap::getExtraTiles() const {
	return mExtraTiles;
}

void TileMap::setBaseColor( const Color& color ) {
	mBaseColor = color;
}

const Color& TileMap::getBaseColor() const {
	return mBaseColor;
}

void TileMap::setDrawGrid( const bool& draw ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_DRAW_GRID, draw ? 1 : 0 );
}

Uint32 TileMap::getDrawGrid() const {
	return mFlags & MAP_FLAG_DRAW_GRID;
}

void TileMap::setDrawBackground( const bool& draw ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_DRAW_BACKGROUND, draw ? 1 : 0 );
}

void TileMap::setShowBlocked( const bool& show ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_SHOW_BLOCKED, show ? 1 : 0 );
}

Uint32 TileMap::getShowBlocked() const {
	return mFlags & MAP_FLAG_SHOW_BLOCKED;
}

Uint32 TileMap::getDrawBackground() const {
	return mFlags & MAP_FLAG_DRAW_BACKGROUND;
}

bool TileMap::getClippedArea() const {
	return 0 != ( mFlags & MAP_FLAG_CLIP_AREA );
}

void TileMap::setClippedArea( const bool& clip ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_CLIP_AREA, clip ? 1 : 0 );
}

bool TileMap::getClampBorders() const {
	return 0 != ( mFlags & MAP_FLAG_CLAMP_BORDERS );
}

void TileMap::setClampBorders( const bool& clamp ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_CLAMP_BORDERS, clamp ? 1 : 0 );
}

Uint32 TileMap::getDrawTileOver() const {
	return 0 != ( mFlags & MAP_FLAG_DRAW_TILE_OVER );
}

void TileMap::setDrawTileOver( const bool& draw ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_DRAW_TILE_OVER, draw ? 1 : 0 );
}

bool TileMap::getLightsEnabled() {
	return 0 != ( mFlags & MAP_FLAG_LIGHTS_ENABLED );
}

void TileMap::setLightsEnabled( const bool& enabled ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_LIGHTS_ENABLED, enabled ? 1 : 0 );
}

bool TileMap::getLightsByVertex() {
	return 0 != ( mFlags & MAP_FLAG_LIGHTS_BYVERTEX );
}

void TileMap::move( const Vector2f& offset ) {
	move( offset.x, offset.y );
}

void TileMap::move( const Float& offsetx, const Float& offsety ) {
	setOffset( Vector2f( mOffset.x + offsetx, mOffset.y + offsety ) );
}

GameObjectPolyData& TileMap::getPolyObjData( Uint32 Id ) {
	return mPolyObjs[Id];
}

GameObject* TileMap::createGameObject( const Uint32& Type, const Uint32& Flags, MapLayer* Layer,
									   const Uint32& DataId ) {
	switch ( Type ) {
		case GAMEOBJECT_TYPE_TEXTUREREGION: {
			GameObjectTextureRegion* tTextureRegion =
				eeNew( GameObjectTextureRegion, ( Flags, Layer ) );

			tTextureRegion->setDataId( DataId );

			return tTextureRegion;
		}
		case GAMEOBJECT_TYPE_TEXTUREREGIONEX: {
			GameObjectTextureRegionEx* tTextureRegionEx =
				eeNew( GameObjectTextureRegionEx, ( Flags, Layer ) );

			tTextureRegionEx->setDataId( DataId );

			return tTextureRegionEx;
		}
		case GAMEOBJECT_TYPE_SPRITE: {
			GameObjectSprite* tSprite = eeNew( GameObjectSprite, ( Flags, Layer ) );

			tSprite->setDataId( DataId );

			return tSprite;
		}
		case GAMEOBJECT_TYPE_OBJECT:
		case GAMEOBJECT_TYPE_POLYGON:
		case GAMEOBJECT_TYPE_POLYLINE: {
			GameObjectPolyData& ObjData = getPolyObjData( DataId );

			GameObjectObject* tObject = NULL;

			if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
				tObject =
					eeNew( GameObjectObject, ( DataId, ObjData.Poly.getBounds(), Layer, Flags ) );
			} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
				tObject = eeNew( GameObjectPolygon, ( DataId, ObjData.Poly, Layer, Flags ) );
			} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
				tObject = eeNew( GameObjectPolyline, ( DataId, ObjData.Poly, Layer, Flags ) );
			}

			if ( NULL != tObject ) {
				tObject->setName( ObjData.Name );
				tObject->setTypeName( ObjData.Type );
				tObject->setProperties( ObjData.Properties );
			}

			return tObject;
		}
		default: {
			if ( mCreateGOCb ) {
				return mCreateGOCb( Type, Flags, Layer, DataId );
			} else {
				GameObjectVirtual* tVirtual;
				TextureRegion* tIsTextureRegion =
					TextureAtlasManager::instance()->getTextureRegionById( DataId );

				if ( NULL != tIsTextureRegion ) {
					tVirtual = eeNew( GameObjectVirtual, ( tIsTextureRegion, Layer, Flags, Type ) );
				} else {
					tVirtual = eeNew( GameObjectVirtual, ( DataId, Layer, Flags, Type ) );
				}

				return tVirtual;
			}
		}
	}

	return NULL;
}

MapLightManager* TileMap::getLightManager() const {
	return mLightManager;
}

const Sizei& TileMap::getTotalSize() const {
	return mPixelSize;
}

const Sizei& TileMap::getTileSize() const {
	return mTileSize;
}

const Sizei& TileMap::getSize() const {
	return mSize;
}

const Uint32& TileMap::getLayerCount() const {
	return mLayerCount;
}

const Uint32& TileMap::getMaxLayers() const {
	return mMaxLayers;
}

bool TileMap::moveLayerUp( MapLayer* Layer ) {
	Uint32 Lindex = getLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN && mLayerCount > 1 && ( Lindex < mLayerCount - 1 ) &&
		 ( Lindex + 1 < mLayerCount ) ) {
		MapLayer* tLayer = mLayers[Lindex + 1];

		mLayers[Lindex] = tLayer;
		mLayers[Lindex + 1] = Layer;

		return true;
	}

	return false;
}

bool TileMap::moveLayerDown( MapLayer* Layer ) {
	Uint32 Lindex = getLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN && mLayerCount > 1 && Lindex >= 1 ) {
		MapLayer* tLayer = mLayers[Lindex - 1];

		mLayers[Lindex] = tLayer;
		mLayers[Lindex - 1] = Layer;

		return true;
	}

	return false;
}

bool TileMap::removeLayer( MapLayer* Layer ) {
	Uint32 Lindex = getLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN ) {
		eeSAFE_DELETE( mLayers[Lindex] );

		MapLayer* LastLayer = NULL;

		// Reorder layers, to clean empty layers in between layers.
		for ( Uint32 i = 0; i < mLayerCount; i++ ) {
			if ( i > 0 && NULL != mLayers[i] && NULL == LastLayer ) {
				mLayers[i - 1] = mLayers[i];
				mLayers[i] = NULL;
			}

			LastLayer = mLayers[i];
		}

		mLayerCount--;

		return true;
	}

	return false;
}

void TileMap::clearProperties() {
	mProperties.clear();
}

void TileMap::addProperty( std::string Text, std::string Value ) {
	mProperties[Text] = Value;
}

void TileMap::editProperty( std::string Text, std::string Value ) {
	mProperties[Text] = Value;
}

void TileMap::removeProperty( std::string Text ) {
	mProperties.erase( Text );
}

TileMap::PropertiesMap& TileMap::getProperties() {
	return mProperties;
}

void TileMap::addVirtualObjectType( const std::string& name ) {
	if ( std::find( mObjTypes.begin(), mObjTypes.end(), name ) == mObjTypes.end() )
		mObjTypes.push_back( name );
}

void TileMap::removeVirtualObjectType( const std::string& name ) {
	auto found = std::find( mObjTypes.begin(), mObjTypes.end(), name );
	if ( found == mObjTypes.end() )
		mObjTypes.erase( found );
}

void TileMap::clearVirtualObjectTypes() {
	mObjTypes.clear();
}

TileMap::GOTypesList& TileMap::getVirtualObjectTypes() {
	return mObjTypes;
}

void TileMap::setCreateGameObjectCallback( const CreateGOCb& Cb ) {
	mCreateGOCb = Cb;
}

bool TileMap::loadFromStream( IOStream& IOS ) {
	sMapHdr MapHdr;
	Uint32 i, z;

	if ( IOS.isOpen() ) {
		IOS.read( (char*)&MapHdr, sizeof( sMapHdr ) );

		if ( MapHdr.Magic == EE_MAP_MAGIC ) {
			if ( NULL == mForcedHeaders ) {
				create( Sizei( MapHdr.SizeX, MapHdr.SizeY ), MapHdr.MaxLayers,
						Sizei( MapHdr.TileSizeX, MapHdr.TileSizeY ), MapHdr.Flags );
			} else {
				create( mForcedHeaders->MapSize, mForcedHeaders->NumLayers,
						mForcedHeaders->TileSize, mForcedHeaders->Flags );
			}

			setBaseColor( Color( MapHdr.BaseColor ) );

			//! Load Properties
			if ( MapHdr.PropertyCount ) {
				sPropertyHdr* tProp = eeNewArray( sPropertyHdr, MapHdr.PropertyCount );

				IOS.read( (char*)&tProp[0], sizeof( sPropertyHdr ) * MapHdr.PropertyCount );

				for ( i = 0; i < MapHdr.PropertyCount; i++ ) {
					addProperty( std::string( tProp[i].Name ), std::string( tProp[i].Value ) );
				}

				eeSAFE_DELETE_ARRAY( tProp );
			}

			//! Load Texture Atlases
			if ( MapHdr.TextureAtlasCount ) {
				sMapTextureAtlas* tSG = eeNewArray( sMapTextureAtlas, MapHdr.TextureAtlasCount );

				IOS.read( (char*)&tSG[0], sizeof( sMapTextureAtlas ) * MapHdr.TextureAtlasCount );

				std::vector<std::string> TextureAtlases;

				for ( i = 0; i < MapHdr.TextureAtlasCount; i++ ) {
					TextureAtlases.push_back( std::string( tSG[i].Path ) );
				}

				//! Load the Texture Atlases if needed
				for ( i = 0; i < TextureAtlases.size(); i++ ) {
					std::string sgname = FileSystem::fileRemoveExtension(
						FileSystem::fileNameFromPath( TextureAtlases[i] ) );

					if ( NULL == TextureAtlasManager::instance()->getByName( sgname ) ) {
						TextureAtlasLoader* tgl = eeNew( TextureAtlasLoader, () );

						if ( !VirtualFileSystem::instance()->fileExists( TextureAtlases[i] ) &&
							 !FileSystem::fileExists( TextureAtlases[i] ) ) {
							std::string path( FileSystem::fileRemoveFileName( mPath ) );

							if ( FileSystem::fileExists( path + TextureAtlases[i] ) ||
								 VirtualFileSystem::instance()->fileExists( path +
																			TextureAtlases[i] ) ) {
								TextureAtlases[i] = path + TextureAtlases[i];
							}
						}

						tgl->loadFromFile( TextureAtlases[i] );

						eeSAFE_DELETE( tgl );
					}
				}

				eeSAFE_DELETE_ARRAY( tSG );
			}

			//! Load Virtual Object Types
			if ( MapHdr.VirtualObjectTypesCount ) {
				sVirtualObj* tVObj = eeNewArray( sVirtualObj, MapHdr.VirtualObjectTypesCount );

				IOS.read( (char*)&tVObj[0],
						  sizeof( sVirtualObj ) * MapHdr.VirtualObjectTypesCount );

				for ( i = 0; i < MapHdr.VirtualObjectTypesCount; i++ ) {
					addVirtualObjectType( std::string( tVObj[i].Name ) );
				}

				eeSAFE_DELETE_ARRAY( tVObj );
			}

			//! Load Layers
			if ( MapHdr.LayerCount ) {
				sLayerHdr* tLayersHdr = eeNewArray( sLayerHdr, MapHdr.LayerCount );
				sLayerHdr* tLayerHdr;

				for ( i = 0; i < MapHdr.LayerCount; i++ ) {
					IOS.read( (char*)&tLayersHdr[i], sizeof( sLayerHdr ) );

					tLayerHdr = &( tLayersHdr[i] );

					MapLayer* tLayer = addLayer( tLayerHdr->Type, tLayerHdr->Flags,
												 std::string( tLayerHdr->Name ) );

					if ( NULL != tLayer ) {
						tLayer->setOffset(
							Vector2f( (Float)tLayerHdr->OffsetX, (Float)tLayerHdr->OffsetY ) );

						sPropertyHdr* tProps = eeNewArray( sPropertyHdr, tLayerHdr->PropertyCount );

						IOS.read( (char*)&tProps[0],
								  sizeof( sPropertyHdr ) * tLayerHdr->PropertyCount );

						for ( z = 0; z < tLayerHdr->PropertyCount; z++ ) {
							tLayer->addProperty( std::string( tProps[z].Name ),
												 std::string( tProps[z].Value ) );
						}

						eeSAFE_DELETE_ARRAY( tProps );
					}
				}

				bool ThereIsTiled = false;

				for ( i = 0; i < mLayerCount; i++ ) {
					if ( NULL != mLayers[i] && mLayers[i]->getType() == MAP_LAYER_TILED ) {
						ThereIsTiled = true;
					}
				}

				Int32 x, y;
				Uint32 tReadFlag = 0;
				TileMapLayer* tTLayer;
				GameObject* tGO;

				if ( NULL != mForcedHeaders ) {
					mSize = Sizei( MapHdr.SizeX, MapHdr.SizeY );
				}

				if ( ThereIsTiled ) {
					//! First we read the tiled layers.
					for ( y = 0; y < mSize.y; y++ ) {
						for ( x = 0; x < mSize.x; x++ ) {

							//! Read the current tile flags
							IOS.read( (char*)&tReadFlag, sizeof( Uint32 ) );

							//! Read every game object header corresponding to this tile
							for ( i = 0; i < mLayerCount; i++ ) {
								if ( tReadFlag & ( 1 << i ) ) {
									tTLayer = reinterpret_cast<TileMapLayer*>( mLayers[i] );

									sMapTileGOHdr tTGOHdr;

									IOS.read( (char*)&tTGOHdr, sizeof( sMapTileGOHdr ) );

									tGO = createGameObject( tTGOHdr.Type, tTGOHdr.Flags, mLayers[i],
															tTGOHdr.Id );

									tTLayer->addGameObject( tGO, Vector2i( x, y ) );
								}
							}
						}
					}
				}

				if ( NULL != mForcedHeaders ) {
					mSize = mForcedHeaders->MapSize;
				}

				//! Load the game objects from the object layers
				MapObjectLayer* tOLayer;

				for ( i = 0; i < mLayerCount; i++ ) {
					if ( NULL != mLayers[i] && mLayers[i]->getType() == MAP_LAYER_OBJECT ) {
						tLayerHdr = &( tLayersHdr[i] );
						tOLayer = reinterpret_cast<MapObjectLayer*>( mLayers[i] );

						for ( Uint32 objCount = 0; objCount < tLayerHdr->ObjectCount; objCount++ ) {
							sMapObjGOHdr tOGOHdr;

							IOS.read( (char*)&tOGOHdr, sizeof( sMapObjGOHdr ) );

							//! For the polygon objects wee need to read the polygon points, the
							//! Name, the TypeName and the Properties.
							if ( tOGOHdr.Type == GAMEOBJECT_TYPE_OBJECT ||
								 tOGOHdr.Type == GAMEOBJECT_TYPE_POLYGON ||
								 tOGOHdr.Type == GAMEOBJECT_TYPE_POLYLINE ) {
								GameObjectPolyData tObjData;

								//! First we read the poly obj header
								sMapObjObjHdr tObjObjHdr;

								IOS.read( (char*)&tObjObjHdr, sizeof( sMapObjObjHdr ) );

								tObjData.Name = std::string( tObjObjHdr.Name );
								tObjData.Type = std::string( tObjObjHdr.Type );

								//! Reads the properties
								for ( Uint32 iProp = 0; iProp < tObjObjHdr.PropertyCount;
									  iProp++ ) {
									sPropertyHdr tObjProp;

									IOS.read( (char*)&tObjProp, sizeof( sPropertyHdr ) );

									tObjData.Properties[std::string( tObjProp.Name )] =
										std::string( tObjProp.Value );
								}

								//! Reads the polygon points
								for ( Uint32 iPoint = 0; iPoint < tObjObjHdr.PointCount;
									  iPoint++ ) {
									Vector2if p;

									IOS.read( (char*)&p, sizeof( Vector2if ) );

									tObjData.Poly.pushBack( Vector2f( p.x, p.y ) );
								}

								mPolyObjs[tOGOHdr.Id] = tObjData;

								//! Recover the last max id
								mLastObjId = eemax( mLastObjId, tOGOHdr.Id );
							}

							tGO = createGameObject( tOGOHdr.Type, tOGOHdr.Flags, mLayers[i],
													tOGOHdr.Id );

							tGO->setPosition( Vector2f( tOGOHdr.PosX, tOGOHdr.PosY ) );

							tOLayer->addGameObject( tGO );
						}
					}
				}

				//! Load the lights
				if ( MapHdr.LightsCount ) {
					createLightManager();

					sMapLightHdr* tLighsHdr = eeNewArray( sMapLightHdr, MapHdr.LightsCount );
					sMapLightHdr* tLightHdr;

					IOS.read( (char*)tLighsHdr, sizeof( sMapLightHdr ) * MapHdr.LightsCount );

					for ( i = 0; i < MapHdr.LightsCount; i++ ) {
						tLightHdr = &( tLighsHdr[i] );

						Color color( tLightHdr->Color );
						RGB rgb( color.toRGB() );

						mLightManager->addLight(
							eeNew( MapLight, ( tLightHdr->Radius, tLightHdr->PosX, tLightHdr->PosY,
											   rgb, (MapLightType)tLightHdr->Type ) ) );
					}

					eeSAFE_DELETE_ARRAY( tLighsHdr );
				}

				eeSAFE_DELETE_ARRAY( tLayersHdr );
			}

			onMapLoaded();

			mPolyObjs.clear();

			return true;
		}
	}

	return false;
}

const std::string& TileMap::getPath() const {
	return mPath;
}

bool TileMap::loadFromFile( const std::string& path ) {
	if ( FileSystem::fileExists( path ) ) {
		mPath = path;

		IOStreamFile IOS( mPath );

		return loadFromStream( IOS );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tPath( path );
		Pack* tPack = PackManager::instance()->exists( tPath );

		if ( NULL != tPack ) {
			return loadFromPack( tPack, tPath );
		}
	}

	return false;
}

bool TileMap::loadFromPack( Pack* Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( FilePackPath ) ) {
		ScopedBuffer buffer;

		Pack->extractFileToMemory( FilePackPath, buffer );

		mPath = FilePackPath;

		return loadFromMemory( reinterpret_cast<const char*>( buffer.get() ), buffer.length() );
	}

	return false;
}

bool TileMap::loadFromMemory( const char* Data, const Uint32& DataSize ) {
	IOStreamMemory IOS( Data, DataSize );

	return loadFromStream( IOS );
}

void TileMap::saveToStream( IOStream& IOS ) {
	Uint32 i;
	sMapHdr MapHdr;
	MapLayer* tLayer;

	std::vector<std::string> TextureAtlases = getTextureAtlases();

	MapHdr.Magic = EE_MAP_MAGIC;
	MapHdr.Flags = mFlags;
	MapHdr.MaxLayers = mMaxLayers;
	MapHdr.SizeX = mSize.getWidth();
	MapHdr.SizeY = mSize.getHeight();
	MapHdr.TileSizeX = mTileSize.getWidth();
	MapHdr.TileSizeY = mTileSize.getHeight();
	MapHdr.LayerCount = mLayerCount;
	MapHdr.PropertyCount = mProperties.size();
	MapHdr.TextureAtlasCount = TextureAtlases.size();
	MapHdr.VirtualObjectTypesCount =
		mObjTypes.size(); //! This is only useful for the Map Editor, to auto add on the load the
						  //! virtual object types that where used to create the map.
	MapHdr.BaseColor = mBaseColor.getValue();

	if ( getLightsEnabled() && NULL != mLightManager )
		MapHdr.LightsCount = mLightManager->getCount();
	else
		MapHdr.LightsCount = 0;

	if ( IOS.isOpen() ) {
		//! Writes the map header
		IOS.write( (const char*)&MapHdr, sizeof( sMapHdr ) );

		//! Writes the properties of the map
		for ( TileMap::PropertiesMap::iterator it = mProperties.begin(); it != mProperties.end();
			  ++it ) {
			sPropertyHdr tProp;

			memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
			memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

			String::strCopy( tProp.Name, it->first.c_str(), MAP_PROPERTY_SIZE );
			String::strCopy( tProp.Value, it->second.c_str(), MAP_PROPERTY_SIZE );

			IOS.write( (const char*)&tProp, sizeof( sPropertyHdr ) );
		}

		//! Writes the texture atlases that the map will need and load
		for ( i = 0; i < TextureAtlases.size(); i++ ) {
			sMapTextureAtlas tSG;

			memset( tSG.Path, 0, MAP_TEXTUREATLAS_PATH_SIZE );

			if ( !mPath.empty() && String::startsWith( TextureAtlases[i],
													   FileSystem::fileRemoveFileName( mPath ) ) ) {
				TextureAtlases[i] =
					TextureAtlases[i].substr( FileSystem::fileRemoveFileName( mPath ).size() );
			}

			String::strCopy( tSG.Path, TextureAtlases[i].c_str(), MAP_TEXTUREATLAS_PATH_SIZE );

			IOS.write( (const char*)&tSG, sizeof( sMapTextureAtlas ) );
		}

		//! Writes the names of the virtual object types created in the map editor
		for ( GOTypesList::iterator votit = mObjTypes.begin(); votit != mObjTypes.end(); ++votit ) {
			sVirtualObj tVObjH;

			memset( tVObjH.Name, 0, MAP_PROPERTY_SIZE );

			String::strCopy( tVObjH.Name, ( *votit ).c_str(), MAP_PROPERTY_SIZE );

			IOS.write( (const char*)&tVObjH, sizeof( sVirtualObj ) );
		}

		//! Writes every layer header
		for ( i = 0; i < mLayerCount; i++ ) {
			tLayer = mLayers[i];
			sLayerHdr tLayerH;

			memset( tLayerH.Name, 0, LAYER_NAME_SIZE );

			String::strCopy( tLayerH.Name, tLayer->getName().c_str(), LAYER_NAME_SIZE );

			tLayerH.Type = tLayer->getType();
			tLayerH.Flags = tLayer->getFlags();
			tLayerH.OffsetX = tLayer->getOffset().x;
			tLayerH.OffsetY = tLayer->getOffset().y;

			if ( MAP_LAYER_OBJECT == tLayerH.Type )
				tLayerH.ObjectCount = reinterpret_cast<MapObjectLayer*>( tLayer )->getObjectCount();
			else
				tLayerH.ObjectCount = 0;

			MapLayer::PropertiesMap& tLayerProp = tLayer->getProperties();

			tLayerH.PropertyCount = tLayerProp.size();

			//! Writes the layer header
			IOS.write( (const char*)&tLayerH, sizeof( sLayerHdr ) );

			//! Writes the properties of the current layer
			for ( MapLayer::PropertiesMap::iterator lit = tLayerProp.begin();
				  lit != tLayerProp.end(); ++lit ) {
				sPropertyHdr tProp;

				memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
				memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

				String::strCopy( tProp.Name, ( *lit ).first.c_str(), MAP_PROPERTY_SIZE );
				String::strCopy( tProp.Value, ( *lit ).second.c_str(), MAP_PROPERTY_SIZE );

				IOS.write( (const char*)&tProp, sizeof( sPropertyHdr ) );
			}
		}

		bool ThereIsTiled = false;

		for ( i = 0; i < mLayerCount; i++ ) {
			if ( NULL != mLayers[i] && mLayers[i]->getType() == MAP_LAYER_TILED ) {
				ThereIsTiled = true;
			}
		}

		//! This method is slow, but allows to save big maps with little space needed, i'll add an
		//! alternative save method ( just plain layer -> tile object saving )
		Int32 x, y;
		Uint32 tReadFlag = 0, z;
		TileMapLayer* tTLayer;
		GameObject* tObj;

		std::vector<GameObject*> tObjects( mLayerCount );

		if ( ThereIsTiled ) {
			//! First we save the tiled layers.
			for ( y = 0; y < mSize.y; y++ ) {
				for ( x = 0; x < mSize.x; x++ ) {
					//! Reset Layer Read Flags and temporal objects
					tReadFlag = 0;

					for ( z = 0; z < mLayerCount; z++ )
						tObjects[z] = NULL;

					//! Look at every layer if it's some data on the current tile, in that case it
					//! will write a bit flag to inform that it's an object on the current tile
					//! layer, and it will store a temporal reference to the object to write layer
					//! the object header information
					for ( i = 0; i < mLayerCount; i++ ) {
						tLayer = mLayers[i];

						if ( NULL != tLayer && tLayer->getType() == MAP_LAYER_TILED ) {
							tTLayer = reinterpret_cast<TileMapLayer*>( tLayer );

							tObj = tTLayer->getGameObject( Vector2i( x, y ) );

							if ( NULL != tObj ) {
								tReadFlag |= 1 << i;

								tObjects[i] = tObj;
							}
						}
					}

					//! Writes the current tile flags
					IOS.write( (const char*)&tReadFlag, sizeof( Uint32 ) );

					//! Writes every game object header corresponding to this tile
					for ( i = 0; i < mLayerCount; i++ ) {
						if ( tReadFlag & ( 1 << i ) ) {
							tObj = tObjects[i];

							sMapTileGOHdr tTGOHdr;

							//! The DataId should be the TextureRegion hash name ( at least in the
							//! cases of type TextureRegion, TextureRegionEx and Sprite.
							tTGOHdr.Id = tObj->getDataId();

							//! If the object type is virtual, means that the real type is stored
							//! elsewhere.
							if ( tObj->getType() != GAMEOBJECT_TYPE_VIRTUAL ) {
								tTGOHdr.Type = tObj->getType();
							} else {
								GameObjectVirtual* tObjV =
									reinterpret_cast<GameObjectVirtual*>( tObj );

								tTGOHdr.Type = tObjV->getRealType();
							}

							tTGOHdr.Flags = tObj->getFlags();

							IOS.write( (const char*)&tTGOHdr, sizeof( sMapTileGOHdr ) );
						}
					}
				}
			}
		}

		//! Then we save the Object layers.
		MapObjectLayer* tOLayer;

		for ( i = 0; i < mLayerCount; i++ ) {
			tLayer = mLayers[i];

			if ( NULL != tLayer && tLayer->getType() == MAP_LAYER_OBJECT ) {
				tOLayer = reinterpret_cast<MapObjectLayer*>( tLayer );

				MapObjectLayer::ObjList ObjList = tOLayer->getObjectList();

				for ( MapObjectLayer::ObjList::iterator MapObjIt = ObjList.begin();
					  MapObjIt != ObjList.end(); ++MapObjIt ) {
					tObj = ( *MapObjIt );

					sMapObjGOHdr tOGOHdr;

					//! The DataId should be the TextureRegion hash name ( at least in the cases of
					//! type TextureRegion, TextureRegionEx and Sprite. And for the Poly Obj should
					//! be an arbitrary value assigned by the map on the moment of creation
					tOGOHdr.Id = tObj->getDataId();

					//! If the object type is virtual, means that the real type is stored elsewhere.
					if ( tObj->getType() != GAMEOBJECT_TYPE_VIRTUAL ) {
						tOGOHdr.Type = tObj->getType();
					} else {
						GameObjectVirtual* tObjV = reinterpret_cast<GameObjectVirtual*>( tObj );

						tOGOHdr.Type = tObjV->getRealType();
					}

					tOGOHdr.Flags = tObj->getFlags();

					tOGOHdr.PosX = (Int32)tObj->getPosition().x;

					tOGOHdr.PosY = (Int32)tObj->getPosition().y;

					IOS.write( (const char*)&tOGOHdr, sizeof( sMapObjGOHdr ) );

					//! For the polygon objects wee need to write the polygon points, the Name, the
					//! TypeName and the Properties.
					if ( tObj->getType() == GAMEOBJECT_TYPE_OBJECT ||
						 tObj->getType() == GAMEOBJECT_TYPE_POLYGON ||
						 tObj->getType() == GAMEOBJECT_TYPE_POLYLINE ) {
						GameObjectObject* tObjObj = reinterpret_cast<GameObjectObject*>( tObj );
						Polygon2f tPoly = tObjObj->getPolygon();
						GameObjectObject::PropertiesMap tObjObjProp = tObjObj->getProperties();
						sMapObjObjHdr tObjObjHdr;

						memset( tObjObjHdr.Name, 0, MAP_PROPERTY_SIZE );
						memset( tObjObjHdr.Type, 0, MAP_PROPERTY_SIZE );

						String::strCopy( tObjObjHdr.Name, tObjObj->getName().c_str(),
										 MAP_PROPERTY_SIZE );
						String::strCopy( tObjObjHdr.Type, tObjObj->getTypeName().c_str(),
										 MAP_PROPERTY_SIZE );

						tObjObjHdr.PointCount = tPoly.getSize();
						tObjObjHdr.PropertyCount = tObjObjProp.size();

						//! Writes the ObjObj header
						IOS.write( (const char*)&tObjObjHdr, sizeof( sMapObjObjHdr ) );

						//! Writes the properties of the current polygon object
						for ( GameObjectObject::PropertiesMap::iterator ooit = tObjObjProp.begin();
							  ooit != tObjObjProp.end(); ++ooit ) {
							sPropertyHdr tProp;

							memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
							memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

							String::strCopy( tProp.Name, ooit->first.c_str(), MAP_PROPERTY_SIZE );
							String::strCopy( tProp.Value, ooit->second.c_str(), MAP_PROPERTY_SIZE );

							IOS.write( (const char*)&tProp, sizeof( sPropertyHdr ) );
						}

						//! Writes the polygon points
						for ( Uint32 tPoint = 0; tPoint < tPoly.getSize(); tPoint++ ) {
							Vector2f pf( tPoly.getAt( tPoint ) );
							Vector2if p( pf.x, pf.y ); //! Convert it to Int32

							IOS.write( (const char*)&p, sizeof( Vector2if ) );
						}
					}
				}
			}
		}

		//! Saves the lights
		if ( MapHdr.LightsCount && NULL != mLightManager ) {
			MapLightManager::LightsList& Lights = mLightManager->getLights();

			for ( MapLightManager::LightsList::iterator LightsIt = Lights.begin();
				  LightsIt != Lights.end(); ++LightsIt ) {
				MapLight* Light = ( *LightsIt );

				sMapLightHdr tLightHdr;

				tLightHdr.Radius = Light->getRadius();
				tLightHdr.PosX = (Int32)Light->getPosition().x;
				tLightHdr.PosY = (Int32)Light->getPosition().y;
				tLightHdr.Color = Color( Light->getColor() ).getValue();
				tLightHdr.Type = Light->getType();

				IOS.write( (const char*)&tLightHdr, sizeof( sMapLightHdr ) );
			}
		}
	}
}

void TileMap::saveToFile( const std::string& path ) {
	if ( !FileSystem::isDirectory( path ) ) {
		mPath = path;

		IOStreamFile IOS( path, "wb" );

		saveToStream( IOS );
	}
}

std::vector<std::string> TileMap::getTextureAtlases() {
	TextureAtlasManager* SGM = TextureAtlasManager::instance();
	auto& res = SGM->getResources();

	std::vector<std::string> items;

	//! Ugly ugly ugly, but i don't see another way
	Uint32 Restricted1 = String::hash( std::string( "global" ) );
	Uint32 Restricted2 = String::hash( SceneManager::instance()
										   ->getUISceneNode()
										   ->getUIThemeManager()
										   ->getDefaultTheme()
										   ->getTextureAtlas()
										   ->getName() );

	for ( auto& it : res ) {
		if ( it.second->getId() != Restricted1 && it.second->getId() != Restricted2 )
			items.push_back( it.second->getPath() );
	}

	return items;
}

void TileMap::setDrawCallback( MapDrawCb Cb ) {
	mDrawCb = Cb;
}

void TileMap::setUpdateCallback( MapUpdateCb Cb ) {
	mUpdateCb = Cb;
}

Texture* TileMap::getBlankTileTexture() {
	return mTileTex;
}

bool TileMap::isTileBlocked( const Vector2i& TilePos ) {
	TileMapLayer* TLayer;
	GameObject* TObj;

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->getType() == MAP_LAYER_TILED ) {
			TLayer = static_cast<TileMapLayer*>( mLayers[i] );
			TObj = TLayer->getGameObject( TilePos );

			if ( NULL != TObj && TObj->isBlocked() ) {
				return true;
			}
		}
	}

	return false;
}

void TileMap::setData( void* value ) {
	mData = value;
}

void* TileMap::getData() const {
	return mData;
}

void TileMap::onMapLoaded() {}

GameObject* TileMap::IsTypeInTilePos( const Uint32& Type, const Vector2i& TilePos ) {
	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->getType() == MAP_LAYER_TILED ) {
			TileMapLayer* tLayer = reinterpret_cast<TileMapLayer*>( mLayers[i] );
			GameObject* tObj = NULL;

			if ( ( tObj = tLayer->getGameObject( TilePos ) ) ) {
				if ( tObj->isType( Type ) ) {
					return tObj;
				}
			}
		}
	}

	return NULL;
}

const Uint8& TileMap::getBackAlpha() const {
	return mBackAlpha;
}

void TileMap::setBackAlpha( const Uint8& alpha ) {
	mBackAlpha = alpha;
}

const Color& TileMap::getBackColor() const {
	return mBackColor;
}

void TileMap::setBackColor( const Color& col ) {
	mBackColor = col;
}

Uint32 TileMap::getNewObjectId() {
	return ++mLastObjId;
}

void TileMap::setGridLinesColor( const Color& Col ) {
	mGridLinesColor = Col;
}

const Color& TileMap::setGridLinesColor() const {
	return mGridLinesColor;
}

}} // namespace EE::Maps
