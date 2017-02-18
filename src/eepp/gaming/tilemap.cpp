#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/gameobjectvirtual.hpp>
#include <eepp/gaming/gameobjectsubtexture.hpp>
#include <eepp/gaming/gameobjectsubtextureex.hpp>
#include <eepp/gaming/gameobjectsprite.hpp>
#include <eepp/gaming/gameobjectobject.hpp>
#include <eepp/gaming/gameobjectpolygon.hpp>
#include <eepp/gaming/gameobjectpolyline.hpp>
#include <eepp/gaming/tilemaplayer.hpp>
#include <eepp/gaming/mapobjectlayer.hpp>

#include <eepp/system/packmanager.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;

#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Gaming {

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
	mGridLinesColor( 255, 255, 255 ,255 ),
	mBackAlpha( 255 ),
	mMouseOver( false ),
	mScale( 1 ),
	mOffscale( 1, 1 ),
	mLastObjId( 0 ),
	mForcedHeaders( NULL )
{
	ViewSize( mViewSize );
}

TileMap::~TileMap() {
	DeleteLayers();
	DisableForcedHeaders();
}

void TileMap::Reset() {
	DeleteLayers();

	mWindow = NULL;
	mLayers = NULL;
	mData = NULL;
	mFlags	= 0;
	mMaxLayers	= 0;
	mMouseOver = false;
	mViewSize = Sizei( 800, 600 );
	mBaseColor = ColorA( 255, 255, 255, 255 );
}

void TileMap::ForceHeadersOnLoad( Sizei mapSize, Sizei tileSize, Uint32 numLayers, Uint32 flags ) {
	DisableForcedHeaders();
	mForcedHeaders = eeNew( ForcedHeaders, ( mapSize, tileSize, numLayers, flags ) );
}

void TileMap::DisableForcedHeaders() {
	eeSAFE_DELETE( mForcedHeaders );
}

void TileMap::DeleteLayers() {
	eeSAFE_DELETE( mLightManager );

	for ( Uint32 i = 0; i < mLayerCount; i++ )
		eeSAFE_DELETE( mLayers[i] );

	eeSAFE_DELETE_ARRAY( mLayers );

	mLayerCount = 0;
}

void TileMap::Create( Sizei Size, Uint32 MaxLayers, Sizei TileSize, Uint32 Flags, Sizei viewSize, EE::Window::Window * Window ) {
	Reset();

	mWindow		= Window;

	if ( NULL == mWindow )
		mWindow	= Engine::instance()->getCurrentWindow();

	mFlags		= Flags;
	mMaxLayers	= MaxLayers;
	mSize		= Size;
	mTileSize	= TileSize;
	mPixelSize	= Size * TileSize;
	mLayers		= eeNewArray( MapLayer*, mMaxLayers );

	if ( LightsEnabled() )
		CreateLightManager();

	for ( Uint32 i = 0; i < mMaxLayers; i++ )
		mLayers[i] = NULL;

	ViewSize( viewSize );

	CreateEmptyTile();
}

void TileMap::CreateLightManager() {
	eeSAFE_DELETE( mLightManager );
	mLightManager = eeNew( MapLightManager, ( this, ( mFlags & MAP_FLAG_LIGHTS_BYVERTEX ) ? true : false ) );
}

void TileMap::CreateEmptyTile() {
	//! I create a texture representing an empty tile to render instead of rendering with primitives because is a lot faster, at least with NVIDIA GPUs.
	TextureFactory * TF = TextureFactory::instance();

	std::string tileName( String::strFormated( "maptile-%dx%d-%ul", mTileSize.getWidth(), mTileSize.getHeight(), mGridLinesColor.getValue() ) );

	Texture * Tex = TF->getByName( tileName );

	if ( NULL == Tex ) {
		Uint32 x, y;
		ColorA Col( mGridLinesColor );

		Image Img( mTileSize.getWidth(), mTileSize.getHeight(), 4 );

		Img.fillWithColor( ColorA( 0, 0, 0, 0 ) );

		for ( x = 0; x < Img.getWidth(); x++ ) {
			Img.setPixel( x, 0, Col );
			Img.setPixel( x, mTileSize.y - 1, Col );
		}

		for ( y = 0; y < Img.getHeight(); y++ ) {
			Img.setPixel( 0, y, Col );
			Img.setPixel( mTileSize.x - 1, y, Col );
		}

		Uint32 TileTexId = TF->loadFromPixels(
			Img.getPixelsPtr(),
			Img.getWidth(),
			Img.getHeight(),
			Img.getChannels(),
			true,
			CLAMP_TO_EDGE,
			false,
			false,
			tileName
		);

		mTileTex = TF->getTexture( TileTexId );
	} else {
		mTileTex = Tex;
	}
}

MapLayer * TileMap::AddLayer( Uint32 Type, Uint32 flags, std::string name ) {
	eeASSERT( NULL != mLayers );

	if ( mLayerCount >= mMaxLayers )
		return NULL;

	switch ( Type ) {
		case MAP_LAYER_TILED:
			mLayers[ mLayerCount ] = eeNew( TileMapLayer, ( this, mSize, flags, name ) );
			break;
		case MAP_LAYER_OBJECT:
			mLayers[ mLayerCount ] = eeNew( MapObjectLayer, ( this, flags, name ) );
			break;
		default:
			return NULL;
	}

	mLayerCount++;

	return mLayers[ mLayerCount - 1 ];
}

MapLayer* TileMap::GetLayer( Uint32 index ) {
	eeASSERT( index < mLayerCount );
	return mLayers[ index ];
}

MapLayer* TileMap::GetLayerByHash( Uint32 hash ) {
	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Id() == hash )
			return mLayers[i];
	}

	return NULL;
}

Uint32 TileMap::GetLayerIndex( MapLayer * Layer ) {
	if ( NULL != Layer ) {
		for ( Uint32 i = 0; i < mLayerCount; i++ ) {
			if ( mLayers[i] == Layer )
				return i;
		}
	}

	return EE_MAP_LAYER_UNKNOWN;
}

MapLayer* TileMap::GetLayer( const std::string& name ) {
	return GetLayerByHash( String::hash( name ) );
}

void TileMap::Draw() {
	GlobalBatchRenderer::instance()->draw();

	if ( ClipedArea() ) {
		mWindow->clipEnable( mScreenPos.x, mScreenPos.y, mViewSize.x, mViewSize.y );
	}

	if ( DrawBackground() ) {
		Primitives P;

		Uint8 Alpha = static_cast<Uint8>( (Float)mBackColor.a() * ( (Float)mBackAlpha / 255.f ) );

		P.setColor( ColorA( mBackColor.r(), mBackColor.g(), mBackColor.b(), Alpha ) );
		P.drawRectangle( Rectf( Vector2f( mScreenPos.x, mScreenPos.y ), Sizef( mViewSize.x, mViewSize.y ) ), 0.f, Vector2f::One );
		P.setColor( ColorA( 255, 255, 255, 255 ) );
	}

	float oldM[16];
	GLi->getCurrentMatrix( GL_MODELVIEW_MATRIX, oldM );
	GLi->loadIdentity();
	GLi->pushMatrix();
	GLi->translatef( (Float)static_cast<Int32>( mScreenPos.x + mOffset.x ), (Float)static_cast<Int32>( mScreenPos.y + mOffset.y ), 0 );
	GLi->scalef( mScale, mScale, 0 );

	GridDraw();

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Visible() )
			mLayers[i]->Draw();
	}

	MouseOverDraw();

	if ( mDrawCb.IsSet() )
		mDrawCb();

	GlobalBatchRenderer::instance()->draw();

	GLi->popMatrix();
	GLi->loadMatrixf( oldM );

	if ( ClipedArea() ) {
		mWindow->clipDisable();
	}
}

void TileMap::MouseOverDraw() {
	if ( !DrawTileOver() || NULL == mTileTex )
		return;

	mTileTex->draw( mMouseOverTileFinal.x * mTileSize.x, mMouseOverTileFinal.y * mTileSize.y, 0, Vector2f::One, mTileOverColor );
}

void TileMap::GridDraw() {
	if ( !DrawGrid() )
		return;

	if ( 0 == mSize.x || 0 == mSize.y || NULL == mTileTex )
		return;

	GlobalBatchRenderer::instance()->draw();

	Vector2i start = StartTile();
	Vector2i end = EndTile();

	Float tx, ty;
	ColorA TileTexCol( 255, 255, 255, mBackAlpha );

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			tx = x * mTileSize.x;

			ty = y * mTileSize.y;

			if ( LightsEnabled() ) {
				Vector2i TPos( x, y );

				if ( mLightManager->IsByVertex() ) {
					ColorA TileTexCol0( *mLightManager->GetTileColor( TPos, 0 ) );
					ColorA TileTexCol1( *mLightManager->GetTileColor( TPos, 1 ) );
					ColorA TileTexCol2( *mLightManager->GetTileColor( TPos, 2 ) );
					ColorA TileTexCol3( *mLightManager->GetTileColor( TPos, 3 ) );

					TileTexCol0.Alpha = TileTexCol1.Alpha = TileTexCol2.Alpha = TileTexCol3.Alpha	= mBackAlpha;

					mTileTex->drawEx( tx, ty, 0, 0, 0, Vector2f::One, TileTexCol0, TileTexCol1, TileTexCol2, TileTexCol3 );
				} else {
					TileTexCol			= *mLightManager->GetTileColor( TPos );
					TileTexCol.Alpha	= mBackAlpha;

					mTileTex->draw( tx, ty, 0, Vector2f::One, TileTexCol );
				}
			} else {
				mTileTex->draw( tx, ty, 0, Vector2f::One, TileTexCol );
			}
		}
	}

	GlobalBatchRenderer::instance()->draw();
}

const bool& TileMap::IsMouseOver() const {
	return mMouseOver;
}

void TileMap::GetMouseOverTile() {
	Vector2i mouse = mWindow->getInput()->getMousePos();

	Vector2i MapPos( static_cast<Float>( mouse.x - mScreenPos.x - mOffset.x ) / mScale, static_cast<Float>( mouse.y - mScreenPos.y - mOffset.y ) / mScale );

	mMouseOver = !( MapPos.x < 0 || MapPos.y < 0 || MapPos.x > mPixelSize.x || MapPos.y > mPixelSize.y );

	MapPos.x = eemax( MapPos.x, 0 );
	MapPos.y = eemax( MapPos.y, 0 );
	MapPos.x = eemin( MapPos.x, mPixelSize.x );
	MapPos.y = eemin( MapPos.y, mPixelSize.y );

	mMouseOverTile.x = MapPos.x / mTileSize.getWidth();
	mMouseOverTile.y = MapPos.y / mTileSize.getHeight();

	// Clamped pos
	mMouseOverTileFinal.x = eemin( mMouseOverTile.x, mSize.getWidth()	- 1 );
	mMouseOverTileFinal.y = eemin( mMouseOverTile.y, mSize.getHeight()	- 1 );
	mMouseOverTileFinal.x = eemax( mMouseOverTileFinal.x, 0 );
	mMouseOverTileFinal.y = eemax( mMouseOverTileFinal.y, 0 );

	mMouseMapPos = MapPos;
}

void TileMap::CalcTilesClip() {
	if ( mTileSize.x > 0 && mTileSize.y > 0 ) {
		Vector2f ffoff( mOffset );
		Vector2i foff( (Int32)ffoff.x, (Int32)ffoff.y );

		mStartTile.x	= -foff.x / ( mTileSize.x * mScale ) - mExtraTiles.x;
		mStartTile.y	= -foff.y / ( mTileSize.y * mScale ) - mExtraTiles.y;

		if ( mStartTile.x < 0 )
			mStartTile.x = 0;

		if ( mStartTile.y < 0 )
			mStartTile.y = 0;

		mEndTile.x		= mStartTile.x + Math::roundUp( (Float)mViewSize.x / ( (Float)mTileSize.x * mScale ) ) + 1 + mExtraTiles.x;
		mEndTile.y		= mStartTile.y + Math::roundUp( (Float)mViewSize.y / ( (Float)mTileSize.y * mScale ) ) + 1 + mExtraTiles.y;

		if ( mEndTile.x > mSize.x )
			mEndTile.x = mSize.x;

		if ( mEndTile.y > mSize.y )
			mEndTile.y = mSize.y;
	}
}

void TileMap::Clamp() {
	if ( !ClampBorders() )
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

void TileMap::Offset( const Vector2f& offset ) {
	mOffset			= offset;

	Clamp();

	CalcTilesClip();
}

Vector2i TileMap::GetMaxOffset() {
	Vector2i v(  ( mTileSize.x * mSize.x * mScale ) - mViewSize.x,
				   ( mTileSize.y * mSize.y * mScale ) - mViewSize.y );

	eemax( 0, v.x );
	eemax( 0, v.y );

	return v;
}

const Float& TileMap::Scale() const {
	return mScale;
}

void TileMap::Scale( const Float& scale ) {
	mScale = scale;

	Offset( mOffset );
}

void TileMap::UpdateScreenAABB() {
	mScreenAABB = eeAABB( -mOffset.x, -mOffset.y, -mOffset.x + mViewSize.getWidth(), -mOffset.y + mViewSize.getHeight() );
}

const eeAABB& TileMap::GetViewAreaAABB() const {
	return mScreenAABB;
}

void TileMap::Update() {
	GetMouseOverTile();

	UpdateScreenAABB();

	if ( NULL != mLightManager )
		mLightManager->Update();

	for ( Uint32 i = 0; i < mLayerCount; i++ )
		mLayers[i]->Update();

	if ( mUpdateCb.IsSet() )
		mUpdateCb();
}

const Sizei& TileMap::ViewSize() const {
	return mViewSize;
}

const Vector2i& TileMap::GetMouseTilePos() const {
	return mMouseOverTileFinal;
}

const Vector2i& TileMap::GetRealMouseTilePos() const {
	return mMouseOverTile;
}

const Vector2i& TileMap::GetMouseMapPos() const {
	return mMouseMapPos;
}

Vector2f TileMap::GetMouseMapPosf() const {
	return Vector2f( (Float)mMouseMapPos.x, (Float)mMouseMapPos.y );
}

Vector2i TileMap::GetMouseTilePosCoords() {
	return GetTileCoords( GetMouseTilePos() );
}

Vector2i TileMap::GetTileCoords( const Vector2i& TilePos ) {
	return ( TilePos * mTileSize );
}

void TileMap::ViewSize( const Sizei& viewSize ) {
	mViewSize = viewSize;

	Clamp();

	CalcTilesClip();
}

const Vector2i& TileMap::Position() const {
	return mScreenPos;
}

void TileMap::Position( const Vector2i& position ) {
	mScreenPos = position;
}

const Vector2f& TileMap::Offset() const {
	return mOffset;
}

const Vector2i& TileMap::StartTile() const {
	return mStartTile;
}

const Vector2i& TileMap::EndTile() const {
	return mEndTile;
}

void TileMap::ExtraTiles( const Vector2i& extra ) {
	mExtraTiles = extra;
}

const Vector2i& TileMap::ExtraTiles() const {
	return mExtraTiles;
}

void TileMap::BaseColor( const ColorA& color ) {
	mBaseColor = color;
}

const ColorA& TileMap::BaseColor() const {
	return mBaseColor;
}

void TileMap::DrawGrid( const bool& draw ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_DRAW_GRID, draw ? 1 : 0 );
}

Uint32 TileMap::DrawGrid() const {
	return mFlags & MAP_FLAG_DRAW_GRID;
}

void TileMap::DrawBackground( const bool& draw ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_DRAW_BACKGROUND, draw ? 1 : 0 );
}

void TileMap::ShowBlocked( const bool& show ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_SHOW_BLOCKED, show ? 1 : 0 );
}

Uint32 TileMap::ShowBlocked() const {
	return mFlags & MAP_FLAG_SHOW_BLOCKED;
}

Uint32 TileMap::DrawBackground() const {
	return mFlags & MAP_FLAG_DRAW_BACKGROUND;
}

bool TileMap::ClipedArea() const {
	return 0 != ( mFlags & MAP_FLAG_CLIP_AREA );
}

void TileMap::ClipedArea( const bool& clip ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_CLIP_AREA, clip ? 1 : 0 );
}

bool TileMap::ClampBorders() const {
	return 0 != ( mFlags & MAP_FLAG_CLAMP_BORDERS );
}

void TileMap::ClampBorders( const bool& clamp ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_CLAMP_BORDERS, clamp ? 1 : 0 );
}

Uint32 TileMap::DrawTileOver() const {
	return 0 != ( mFlags & MAP_FLAG_DRAW_TILE_OVER );
}

void TileMap::DrawTileOver( const bool& draw ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_DRAW_TILE_OVER, draw ? 1 : 0 );
}

bool TileMap::LightsEnabled() {
	return 0 != ( mFlags & MAP_FLAG_LIGHTS_ENABLED );
}

void TileMap::LightsEnabled( const bool& enabled ) {
	BitOp::setBitFlagValue( &mFlags, MAP_FLAG_LIGHTS_ENABLED, enabled ? 1 : 0 );
}

bool TileMap::LightsByVertex() {
	return 0 != ( mFlags & MAP_FLAG_LIGHTS_BYVERTEX );
}

void TileMap::Move( const Vector2f& offset )  {
	Move( offset.x, offset.y );
}

void TileMap::Move( const Float& offsetx, const Float& offsety ) {
	Offset( Vector2f( mOffset.x + offsetx, mOffset.y + offsety ) );
}

GameObjectPolyData& TileMap::GetPolyObjData( Uint32 Id ) {
	return mPolyObjs[ Id ];
}

GameObject * TileMap::CreateGameObject( const Uint32& Type, const Uint32& Flags, MapLayer * Layer, const Uint32& DataId ) {
	switch ( Type ) {
		case GAMEOBJECT_TYPE_SUBTEXTURE:
		{
			GameObjectSubTexture * tSubTexture = eeNew( GameObjectSubTexture, ( Flags, Layer ) );

			tSubTexture->DataId( DataId );

			return tSubTexture;
		}
		case GAMEOBJECT_TYPE_SUBTEXTUREEX:
		{
			GameObjectSubTextureEx * tSubTextureEx = eeNew( GameObjectSubTextureEx, ( Flags, Layer ) );

			tSubTextureEx->DataId( DataId );

			return tSubTextureEx;
		}
		case GAMEOBJECT_TYPE_SPRITE:
		{
			GameObjectSprite * tSprite = eeNew( GameObjectSprite, ( Flags, Layer ) );

			tSprite->DataId( DataId );

			return tSprite;
		}
		case GAMEOBJECT_TYPE_OBJECT:
		case GAMEOBJECT_TYPE_POLYGON:
		case GAMEOBJECT_TYPE_POLYLINE:
		{
			GameObjectPolyData& ObjData = GetPolyObjData( DataId );

			GameObjectObject * tObject = NULL;

			if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
				tObject = eeNew( GameObjectObject, ( DataId, ObjData.Poly.toAABB(), Layer, Flags ) );
			} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
				tObject = eeNew( GameObjectPolygon, ( DataId, ObjData.Poly, Layer, Flags ) );
			} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
				tObject = eeNew( GameObjectPolyline, ( DataId, ObjData.Poly, Layer, Flags ) );
			}

			if ( NULL != tObject ) {
				tObject->Name( ObjData.Name );
				tObject->TypeName( ObjData.Type );
				tObject->SetProperties( ObjData.Properties );
			}

			return tObject;
		}
		default:
		{
			if ( mCreateGOCb.IsSet() ) {
				return mCreateGOCb( Type, Flags, Layer, DataId );
			} else {
				GameObjectVirtual * tVirtual;
				SubTexture * tIsSubTexture = TextureAtlasManager::instance()->getSubTextureById( DataId );

				if ( NULL != tIsSubTexture ) {
					tVirtual = eeNew( GameObjectVirtual, ( tIsSubTexture, Layer, Flags, Type ) );
				} else {
					tVirtual = eeNew( GameObjectVirtual, ( DataId, Layer, Flags, Type ) );
				}

				return tVirtual;
			}
		}
	}

	return NULL;
}

MapLightManager * TileMap::GetLightManager() const {
	return mLightManager;
}

const Sizei& TileMap::TotalSize() const {
	return mPixelSize;
}

const Sizei& TileMap::TileSize() const {
	return mTileSize;
}

const Sizei& TileMap::Size() const {
	return mSize;
}

const Uint32& TileMap::LayerCount() const {
	return mLayerCount;
}

const Uint32& TileMap::MaxLayers() const {
	return mMaxLayers;
}

bool TileMap::MoveLayerUp( MapLayer * Layer ) {
	Uint32 Lindex = GetLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN && mLayerCount > 1 && ( Lindex < mLayerCount - 1 ) && ( Lindex + 1 < mLayerCount ) ) {
		MapLayer * tLayer = mLayers[ Lindex + 1 ];

		mLayers[ Lindex ]		= tLayer;
		mLayers[ Lindex + 1 ]	= Layer;

		return true;
	}

	return false;
}

bool TileMap::MoveLayerDown( MapLayer * Layer ) {
	Uint32 Lindex = GetLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN && mLayerCount > 1 && Lindex >= 1 ) {
		MapLayer * tLayer = mLayers[ Lindex - 1 ];

		mLayers[ Lindex ]		= tLayer;
		mLayers[ Lindex - 1 ]	= Layer;

		return true;
	}

	return false;
}

bool TileMap::RemoveLayer( MapLayer * Layer ) {
	Uint32 Lindex = GetLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN ) {
		eeSAFE_DELETE( mLayers[ Lindex ] );

		MapLayer * LastLayer = NULL;

		// Reorder layers, to clean empty layers in between layers.
		for ( Uint32 i = 0; i < mLayerCount; i++ ) {
			if ( i > 0 && NULL != mLayers[i] && NULL == LastLayer ) {
				mLayers[ i - 1 ]	= mLayers[ i ];
				mLayers[ i ]		= NULL;
			}

			LastLayer = mLayers[i];
		}

		mLayerCount--;

		return true;
	}

	return false;
}

void TileMap::ClearProperties() {
	mProperties.clear();
}

void TileMap::AddProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void TileMap::EditProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void TileMap::RemoveProperty( std::string Text ) {
	mProperties.erase( Text );
}

TileMap::PropertiesMap& TileMap::GetProperties() {
	return mProperties;
}

void TileMap::AddVirtualObjectType( const std::string& name ) {
	mObjTypes.push_back( name );
	mObjTypes.unique();
}

void TileMap::RemoveVirtualObjectType( const std::string& name ) {
	mObjTypes.remove( name );
}

void TileMap::ClearVirtualObjectTypes() {
	mObjTypes.clear();
}

TileMap::GOTypesList& TileMap::GetVirtualObjectTypes() {
	return mObjTypes;
}

void TileMap::SetCreateGameObjectCallback( const CreateGOCb& Cb ) {
	mCreateGOCb = Cb;
}

bool TileMap::LoadFromStream( IOStream& IOS ) {
	sMapHdr MapHdr;
	Uint32 i, z;

	if ( IOS.isOpen() ) {
		IOS.read( (char*)&MapHdr, sizeof(sMapHdr) );

		if ( MapHdr.Magic == EE_MAP_MAGIC ) {
			if ( NULL == mForcedHeaders ) {
				Create( Sizei( MapHdr.SizeX, MapHdr.SizeY ), MapHdr.MaxLayers, Sizei( MapHdr.TileSizeX, MapHdr.TileSizeY ), MapHdr.Flags );
			} else {
				Create( mForcedHeaders->MapSize, mForcedHeaders->NumLayers, mForcedHeaders->TileSize, mForcedHeaders->Flags );
			}

			BaseColor( ColorA( MapHdr.BaseColor ) );

			//! Load Properties
			if ( MapHdr.PropertyCount ) {
				sPropertyHdr * tProp = eeNewArray( sPropertyHdr, MapHdr.PropertyCount );

				IOS.read( (char*)&tProp[0], sizeof(sPropertyHdr) * MapHdr.PropertyCount );

				for ( i = 0; i < MapHdr.PropertyCount; i++ ) {
					AddProperty( std::string( tProp[i].Name ), std::string( tProp[i].Value ) );
				}

				eeSAFE_DELETE_ARRAY( tProp );
			}

			//! Load Texture Atlases
			if ( MapHdr.TextureAtlasCount ) {
				sMapTextureAtlas * tSG = eeNewArray( sMapTextureAtlas, MapHdr.TextureAtlasCount );

				IOS.read( (char*)&tSG[0], sizeof(sMapTextureAtlas) * MapHdr.TextureAtlasCount );

				std::vector<std::string> TextureAtlases;

				for ( i = 0; i < MapHdr.TextureAtlasCount; i++ ) {
					TextureAtlases.push_back( std::string( tSG[i].Path ) );
				}

				//! Load the Texture Atlases if needed
				for ( i = 0; i < TextureAtlases.size(); i++ ) {
					std::string sgname = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( TextureAtlases[i] ) );

					if ( NULL == TextureAtlasManager::instance()->getByName( sgname ) ) {
						TextureAtlasLoader * tgl = eeNew( TextureAtlasLoader, () );

						tgl->load( Sys::getProcessPath() + TextureAtlases[i] );

						eeSAFE_DELETE( tgl );
					}
				}

				eeSAFE_DELETE_ARRAY( tSG );
			}

			//! Load Virtual Object Types
			if ( MapHdr.VirtualObjectTypesCount ) {
				sVirtualObj * tVObj = eeNewArray( sVirtualObj, MapHdr.VirtualObjectTypesCount );

				IOS.read( (char*)&tVObj[0], sizeof(sVirtualObj) * MapHdr.VirtualObjectTypesCount );

				for ( i = 0; i < MapHdr.VirtualObjectTypesCount; i++ ) {
					AddVirtualObjectType( std::string( tVObj[i].Name ) );
				}

				eeSAFE_DELETE_ARRAY( tVObj );
			}

			//! Load Layers
			if ( MapHdr.LayerCount ) {
				sLayerHdr * tLayersHdr = eeNewArray( sLayerHdr, MapHdr.LayerCount );
				sLayerHdr * tLayerHdr;

				for ( i = 0; i < MapHdr.LayerCount; i++ ) {
					IOS.read( (char*)&tLayersHdr[i], sizeof(sLayerHdr) );

					tLayerHdr = &(tLayersHdr[i]);

					MapLayer * tLayer = AddLayer( tLayerHdr->Type, tLayerHdr->Flags, std::string( tLayerHdr->Name ) );

					if ( NULL != tLayer ) {
						tLayer->Offset( Vector2f( (Float)tLayerHdr->OffsetX, (Float)tLayerHdr->OffsetY ) );

						sPropertyHdr * tProps = eeNewArray( sPropertyHdr, tLayerHdr->PropertyCount );

						IOS.read( (char*)&tProps[0], sizeof(sPropertyHdr) * tLayerHdr->PropertyCount );

						for ( z = 0; z < tLayerHdr->PropertyCount; z++ ) {
							tLayer->AddProperty( std::string( tProps[z].Name ), std::string( tProps[z].Value ) );
						}

						eeSAFE_DELETE_ARRAY( tProps );
					}
				}

				bool ThereIsTiled = false;

				for ( i = 0; i < mLayerCount; i++ ) {
					if ( NULL != mLayers[i] && mLayers[i]->Type() == MAP_LAYER_TILED ) {
						ThereIsTiled = true;
					}
				}

				Int32 x, y;
				Uint32 tReadFlag = 0;
				TileMapLayer * tTLayer;
				GameObject * tGO;

				if ( NULL != mForcedHeaders ) {
					mSize = Sizei( MapHdr.SizeX, MapHdr.SizeY );
				}

				if ( ThereIsTiled ) {
					//! First we read the tiled layers.
					for ( y = 0; y < mSize.y; y++ ) {
						for ( x = 0; x < mSize.x; x++ ) {

							//! Read the current tile flags
							IOS.read( (char*)&tReadFlag, sizeof(Uint32) );

							//! Read every game object header corresponding to this tile
							for ( i = 0; i < mLayerCount; i++ ) {
								if ( tReadFlag & ( 1 << i ) ) {
									tTLayer = reinterpret_cast<TileMapLayer*> ( mLayers[i] );

									sMapTileGOHdr tTGOHdr;

									IOS.read( (char*)&tTGOHdr, sizeof(sMapTileGOHdr) );

									tGO = CreateGameObject( tTGOHdr.Type, tTGOHdr.Flags, mLayers[i], tTGOHdr.Id );

									tTLayer->AddGameObject( tGO, Vector2i( x, y ) );
								}
							}
						}
					}
				}

				if ( NULL != mForcedHeaders ) {
					mSize = mForcedHeaders->MapSize;
				}

				//! Load the game objects from the object layers
				MapObjectLayer * tOLayer;

				for ( i = 0; i < mLayerCount; i++ ) {
					if ( NULL != mLayers[i] && mLayers[i]->Type() == MAP_LAYER_OBJECT ) {
						tLayerHdr	= &( tLayersHdr[i] );
						tOLayer		= reinterpret_cast<MapObjectLayer*> ( mLayers[i] );

						for ( Uint32 objCount = 0; objCount < tLayerHdr->ObjectCount; objCount++ ) {
							sMapObjGOHdr tOGOHdr;

							IOS.read( (char*)&tOGOHdr, sizeof(sMapObjGOHdr) );

							//! For the polygon objects wee need to read the polygon points, the Name, the TypeName and the Properties.
							if (	tOGOHdr.Type == GAMEOBJECT_TYPE_OBJECT		||
									tOGOHdr.Type == GAMEOBJECT_TYPE_POLYGON		||
									tOGOHdr.Type == GAMEOBJECT_TYPE_POLYLINE )
							{
								GameObjectPolyData tObjData;

								//! First we read the poly obj header
								sMapObjObjHdr tObjObjHdr;

								IOS.read( (char*)&tObjObjHdr, sizeof(sMapObjObjHdr) );

								tObjData.Name = std::string( tObjObjHdr.Name );
								tObjData.Type = std::string( tObjObjHdr.Type );

								//! Reads the properties
								for ( Uint32 iProp = 0; iProp < tObjObjHdr.PropertyCount; iProp++ ) {
									sPropertyHdr tObjProp;

									IOS.read( (char*)&tObjProp, sizeof(sPropertyHdr) );

									tObjData.Properties[ std::string( tObjProp.Name ) ] = std::string( tObjProp.Value );
								}

								//! Reads the polygon points
								for ( Uint32 iPoint = 0; iPoint < tObjObjHdr.PointCount; iPoint++ ) {
									Vector2if p;

									IOS.read( (char*)&p, sizeof(Vector2if) );

									tObjData.Poly.pushBack( Vector2f( p.x, p.y ) );
								}

								mPolyObjs[ tOGOHdr.Id ] = tObjData;

								//! Recover the last max id
								mLastObjId	= eemax( mLastObjId, tOGOHdr.Id );
							}

							tGO = CreateGameObject( tOGOHdr.Type, tOGOHdr.Flags, mLayers[i], tOGOHdr.Id );

							tGO->Pos( Vector2f( tOGOHdr.PosX, tOGOHdr.PosY ) );

							tOLayer->AddGameObject( tGO );
						}
					}
				}

				//! Load the lights
				if ( MapHdr.LightsCount ) {
					CreateLightManager();

					sMapLightHdr * tLighsHdr = eeNewArray( sMapLightHdr, MapHdr.LightsCount );
					sMapLightHdr * tLightHdr;

					IOS.read( (char*)tLighsHdr, sizeof(sMapLightHdr) * MapHdr.LightsCount );

					for ( i = 0; i < MapHdr.LightsCount; i++ ) {
						tLightHdr = &( tLighsHdr[ i ] );

						ColorA color( tLightHdr->Color );
						RGB rgb( color.toColor() );

						mLightManager->AddLight(
							eeNew( MapLight, ( tLightHdr->Radius, tLightHdr->PosX, tLightHdr->PosY, rgb, (LIGHT_TYPE)tLightHdr->Type ) )
						);
					}

					eeSAFE_DELETE_ARRAY( tLighsHdr );
				}

				eeSAFE_DELETE_ARRAY( tLayersHdr );
			}

			OnMapLoaded();

			mPolyObjs.clear();

			return true;
		}
	}

	return false;
}

const std::string& TileMap::Path() const {
	return mPath;
}

bool TileMap::Load( const std::string& path ) {
	if ( FileSystem::fileExists( path ) ) {
		mPath = path;

		IOStreamFile IOS( mPath, std::ios::in | std::ios::binary );

		return LoadFromStream( IOS );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tPath( path );
		Pack * tPack = PackManager::instance()->exists( tPath ) ;

		if ( NULL != tPack ) {
			mPath = tPath;
			return LoadFromPack( tPack, tPath );
		}
	}

	return false;
}

bool TileMap::LoadFromPack( Pack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( FilePackPath ) ) {
		SafeDataPointer PData;

		Pack->extractFileToMemory( FilePackPath, PData );

		return LoadFromMemory( reinterpret_cast<const char*> ( PData.Data ), PData.DataSize );
	}

	return false;
}

bool TileMap::LoadFromMemory( const char * Data, const Uint32& DataSize ) {
	IOStreamMemory IOS( Data, DataSize );

	return LoadFromStream( IOS );
}

void TileMap::SaveToStream( IOStream& IOS ) {
	Uint32 i;
	sMapHdr MapHdr;
	MapLayer * tLayer;

	std::vector<std::string> TextureAtlases = GetTextureAtlases();

	MapHdr.Magic					= EE_MAP_MAGIC;
	MapHdr.Flags					= mFlags;
	MapHdr.MaxLayers				= mMaxLayers;
	MapHdr.SizeX					= mSize.getWidth();
	MapHdr.SizeY					= mSize.getHeight();
	MapHdr.TileSizeX				= mTileSize.getWidth();
	MapHdr.TileSizeY				= mTileSize.getHeight();
	MapHdr.LayerCount				= mLayerCount;
	MapHdr.PropertyCount			= mProperties.size();
	MapHdr.TextureAtlasCount		= TextureAtlases.size();
	MapHdr.VirtualObjectTypesCount	= mObjTypes.size();	//! This is only useful for the Map Editor, to auto add on the load the virtual object types that where used to create the map.
	MapHdr.BaseColor				= mBaseColor.getValue();

	if ( LightsEnabled() && NULL != mLightManager )
		MapHdr.LightsCount = mLightManager->Count();
	else
		MapHdr.LightsCount = 0;

	if ( IOS.isOpen() ) {
		//! Writes the map header
		IOS.write( (const char*)&MapHdr, sizeof(sMapHdr) );

		//! Writes the properties of the map
		for ( TileMap::PropertiesMap::iterator it = mProperties.begin(); it != mProperties.end(); it++ ) {
			sPropertyHdr tProp;

			memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
			memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

			String::strCopy( tProp.Name, it->first.c_str(), MAP_PROPERTY_SIZE );
			String::strCopy( tProp.Value, it->second.c_str(), MAP_PROPERTY_SIZE );

			IOS.write( (const char*)&tProp, sizeof(sPropertyHdr) );
		}

		//! Writes the texture atlases that the map will need and load
		for ( i = 0; i < TextureAtlases.size(); i++ ) {
			sMapTextureAtlas tSG;

			memset( tSG.Path, 0, MAP_TEXTUREATLAS_PATH_SIZE );

			String::strCopy( tSG.Path, TextureAtlases[i].c_str(), MAP_TEXTUREATLAS_PATH_SIZE );

			IOS.write( (const char*)&tSG, sizeof(sMapTextureAtlas) );
		}

		//! Writes the names of the virtual object types created in the map editor
		for ( GOTypesList::iterator votit = mObjTypes.begin(); votit != mObjTypes.end(); votit++ ) {
			sVirtualObj tVObjH;

			memset( tVObjH.Name, 0, MAP_PROPERTY_SIZE );

			String::strCopy( tVObjH.Name, (*votit).c_str(), MAP_PROPERTY_SIZE );

			IOS.write( (const char*)&tVObjH, sizeof(sVirtualObj) );
		}

		//! Writes every layer header
		for ( i = 0; i < mLayerCount; i++ ) {
			tLayer = mLayers[i];
			sLayerHdr tLayerH;

			memset( tLayerH.Name, 0, LAYER_NAME_SIZE );

			String::strCopy( tLayerH.Name, tLayer->Name().c_str(), LAYER_NAME_SIZE );

			tLayerH.Type			= tLayer->Type();
			tLayerH.Flags			= tLayer->Flags();
			tLayerH.OffsetX			= tLayer->Offset().x;
			tLayerH.OffsetY			= tLayer->Offset().y;

			if ( MAP_LAYER_OBJECT == tLayerH.Type )
				tLayerH.ObjectCount = reinterpret_cast<MapObjectLayer*> ( tLayer )->GetObjectCount();
			else
				tLayerH.ObjectCount		= 0;

			MapLayer::PropertiesMap& tLayerProp = tLayer->GetProperties();

			tLayerH.PropertyCount	= tLayerProp.size();

			//! Writes the layer header
			IOS.write( (const char*)&tLayerH, sizeof(sLayerHdr) );

			//! Writes the properties of the current layer
			for ( MapLayer::PropertiesMap::iterator lit = tLayerProp.begin(); lit != tLayerProp.end(); lit++ ) {
				sPropertyHdr tProp;

				memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
				memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

				String::strCopy( tProp.Name, (*lit).first.c_str(), MAP_PROPERTY_SIZE );
				String::strCopy( tProp.Value, (*lit).second.c_str(), MAP_PROPERTY_SIZE );

				IOS.write( (const char*)&tProp, sizeof(sPropertyHdr) );
			}
		}

		bool ThereIsTiled = false;

		for ( i = 0; i < mLayerCount; i++ ) {
			if ( NULL != mLayers[i] && mLayers[i]->Type() == MAP_LAYER_TILED ) {
				ThereIsTiled = true;
			}
		}

		//! This method is slow, but allows to save big maps with little space needed, i'll add an alternative save method ( just plain layer -> tile object saving )
		Int32 x, y;
		Uint32 tReadFlag = 0, z;
		TileMapLayer * tTLayer;
		GameObject * tObj;

		std::vector<GameObject*> tObjects( mLayerCount );

		if ( ThereIsTiled ) {
			//! First we save the tiled layers.
			for ( y = 0; y < mSize.y; y++ ) {
				for ( x = 0; x < mSize.x; x++ ) {
					//! Reset Layer Read Flags and temporal objects
					tReadFlag		= 0;

					for ( z = 0; z < mLayerCount; z++ )
						tObjects[z] = NULL;

					//! Look at every layer if it's some data on the current tile, in that case it will write a bit flag to
					//! inform that it's an object on the current tile layer, and it will store a temporal reference to the
					//! object to write layer the object header information
					for ( i = 0; i < mLayerCount; i++ ) {
						tLayer = mLayers[i];

						if ( NULL != tLayer && tLayer->Type() == MAP_LAYER_TILED ) {
							tTLayer = reinterpret_cast<TileMapLayer*> ( tLayer );

							tObj = tTLayer->GetGameObject( Vector2i( x, y ) );

							if ( NULL != tObj ) {
								tReadFlag |= 1 << i;

								tObjects[i] = tObj;
							}
						}
					}

					//! Writes the current tile flags
					IOS.write( (const char*)&tReadFlag, sizeof(Uint32) );

					//! Writes every game object header corresponding to this tile
					for ( i = 0; i < mLayerCount; i++ ) {
						if ( tReadFlag & ( 1 << i ) ) {
							tObj = tObjects[i];

							sMapTileGOHdr tTGOHdr;

							//! The DataId should be the SubTexture hash name ( at least in the cases of type SubTexture, SubTextureEx and Sprite.
							tTGOHdr.Id		= tObj->DataId();

							//! If the object type is virtual, means that the real type is stored elsewhere.
							if ( tObj->Type() != GAMEOBJECT_TYPE_VIRTUAL ) {
								tTGOHdr.Type	= tObj->Type();
							} else {
								GameObjectVirtual * tObjV = reinterpret_cast<GameObjectVirtual*> ( tObj );

								tTGOHdr.Type	= tObjV->RealType();
							}

							tTGOHdr.Flags	= tObj->Flags();

							IOS.write( (const char*)&tTGOHdr, sizeof(sMapTileGOHdr) );
						}
					}
				}
			}
		}

		//! Then we save the Object layers.
		MapObjectLayer * tOLayer;

		for ( i = 0; i < mLayerCount; i++ ) {
			tLayer = mLayers[i];

			if ( NULL != tLayer && tLayer->Type() == MAP_LAYER_OBJECT ) {
				tOLayer = reinterpret_cast<MapObjectLayer*> ( tLayer );

				MapObjectLayer::ObjList ObjList = tOLayer->GetObjectList();

				for ( MapObjectLayer::ObjList::iterator MapObjIt = ObjList.begin(); MapObjIt != ObjList.end(); MapObjIt++ ) {
					tObj = (*MapObjIt);

					sMapObjGOHdr tOGOHdr;

					//! The DataId should be the SubTexture hash name ( at least in the cases of type SubTexture, SubTextureEx and Sprite.
					//! And for the Poly Obj should be an arbitrary value assigned by the map on the moment of creation
					tOGOHdr.Id		= tObj->DataId();

					//! If the object type is virtual, means that the real type is stored elsewhere.
					if ( tObj->Type() != GAMEOBJECT_TYPE_VIRTUAL ) {
						tOGOHdr.Type	= tObj->Type();
					} else {
						GameObjectVirtual * tObjV = reinterpret_cast<GameObjectVirtual*> ( tObj );

						tOGOHdr.Type	= tObjV->RealType();
					}

					tOGOHdr.Flags	= tObj->Flags();

					tOGOHdr.PosX	= (Int32)tObj->Pos().x;

					tOGOHdr.PosY	= (Int32)tObj->Pos().y;

					IOS.write( (const char*)&tOGOHdr, sizeof(sMapObjGOHdr) );

					//! For the polygon objects wee need to write the polygon points, the Name, the TypeName and the Properties.
					if (	tObj->Type() == GAMEOBJECT_TYPE_OBJECT		||
							tObj->Type() == GAMEOBJECT_TYPE_POLYGON		||
							tObj->Type() == GAMEOBJECT_TYPE_POLYLINE )
					{
						GameObjectObject * tObjObj						= reinterpret_cast<GameObjectObject*>( tObj );
						Polygon2f tPoly								= tObjObj->GetPolygon();
						GameObjectObject::PropertiesMap tObjObjProp	= tObjObj->GetProperties();
						sMapObjObjHdr tObjObjHdr;

						memset( tObjObjHdr.Name, 0, MAP_PROPERTY_SIZE );
						memset( tObjObjHdr.Type, 0, MAP_PROPERTY_SIZE );

						String::strCopy( tObjObjHdr.Name, tObjObj->Name().c_str(), MAP_PROPERTY_SIZE );
						String::strCopy( tObjObjHdr.Type, tObjObj->TypeName().c_str(), MAP_PROPERTY_SIZE );

						tObjObjHdr.PointCount		= tPoly.getSize();
						tObjObjHdr.PropertyCount	= tObjObjProp.size();

						//! Writes the ObjObj header
						IOS.write( (const char*)&tObjObjHdr, sizeof(sMapObjObjHdr) );

						//! Writes the properties of the current polygon object
						for ( GameObjectObject::PropertiesMap::iterator ooit = tObjObjProp.begin(); ooit != tObjObjProp.end(); ooit++ ) {
							sPropertyHdr tProp;

							memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
							memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

							String::strCopy( tProp.Name, ooit->first.c_str(), MAP_PROPERTY_SIZE );
							String::strCopy( tProp.Value, ooit->second.c_str(), MAP_PROPERTY_SIZE );

							IOS.write( (const char*)&tProp, sizeof(sPropertyHdr) );
						}

						//! Writes the polygon points
						for ( Uint32 tPoint = 0; tPoint < tPoly.getSize(); tPoint++ ) {
							Vector2f pf( tPoly.getAt( tPoint ) );
							Vector2if p( pf.x, pf.y );	//! Convert it to Int32

							IOS.write( (const char*)&p, sizeof(Vector2if) );
						}
					}
				}
			}
		}

		//! Saves the lights
		if ( MapHdr.LightsCount && NULL != mLightManager ) {
			MapLightManager::LightsList& Lights = mLightManager->GetLights();

			for ( MapLightManager::LightsList::iterator LightsIt = Lights.begin(); LightsIt != Lights.end(); LightsIt++ ) {
				MapLight * Light = (*LightsIt);

				sMapLightHdr tLightHdr;

				tLightHdr.Radius	= Light->Radius();
				tLightHdr.PosX		= (Int32)Light->Position().x;
				tLightHdr.PosY		= (Int32)Light->Position().y;
				tLightHdr.Color		= ColorA( Light->Color() ).getValue();
				tLightHdr.Type		= Light->Type();

				IOS.write( (const char*)&tLightHdr, sizeof(sMapLightHdr) );
			}
		}
	}
}

void TileMap::Save( const std::string& path ) {
	if ( !FileSystem::isDirectory( path ) ) {
		IOStreamFile IOS( path, std::ios::out | std::ios::binary );

		SaveToStream( IOS );

		mPath = path;
	}
}

std::vector<std::string> TileMap::GetTextureAtlases() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	std::list<TextureAtlas*>& Res = SGM->getResources();

	std::vector<std::string> items;

	//! Ugly ugly ugly, but i don't see another way
	Uint32 Restricted1 = String::hash( std::string( "global" ) );
	Uint32 Restricted2 = String::hash( UI::UIThemeManager::instance()->defaultTheme()->getTextureAtlas()->getName() );

	for ( std::list<TextureAtlas*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
		if ( (*it)->getId() != Restricted1 && (*it)->getId() != Restricted2 )
			items.push_back( (*it)->getPath() );
	}

	return items;
}

void TileMap::SetDrawCallback( MapDrawCb Cb ) {
	mDrawCb = Cb;
}

void TileMap::SetUpdateCallback( MapUpdateCb Cb ) {
	mUpdateCb = Cb;
}

Texture * TileMap::GetBlankTileTexture() {
	return mTileTex;
}

bool TileMap::IsTileBlocked( const Vector2i& TilePos ) {
	TileMapLayer * TLayer;
	GameObject * TObj;

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Type() == MAP_LAYER_TILED ) {
			TLayer	= static_cast<TileMapLayer*>( mLayers[i] );
			TObj	= TLayer->GetGameObject( TilePos );

			if ( NULL != TObj && TObj->Blocked() ) {
				return true;
			}
		}
	}

	return false;
}

void TileMap::Data( void * value ) {
	mData = value;
}

void * TileMap::Data() const {
	return mData;
}

void TileMap::OnMapLoaded() {
}

GameObject * TileMap::IsTypeInTilePos( const Uint32& Type, const Vector2i& TilePos ) {
	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Type() == MAP_LAYER_TILED ) {
			TileMapLayer * tLayer = reinterpret_cast<TileMapLayer*> ( mLayers[i] );
			GameObject * tObj = NULL;

			if ( ( tObj = tLayer->GetGameObject( TilePos ) ) ) {
				if ( tObj->IsType( Type ) ) {
					return tObj;
				}
			}
		}
	}

	return NULL;
}

const Uint8& TileMap::BackAlpha() const {
	return mBackAlpha;
}

void TileMap::BackAlpha( const Uint8& alpha ) {
	mBackAlpha = alpha;
}

const ColorA& TileMap::BackColor() const {
	return mBackColor;
}

void TileMap::BackColor( const ColorA& col ) {
	mBackColor = col;
}

Uint32 TileMap::GetNewObjectId() {
	return ++mLastObjId;
}

void TileMap::GridLinesColor( const ColorA& Col ) {
	mGridLinesColor = Col;
}

const ColorA& TileMap::GridLinesColor() const {
	return mGridLinesColor;
}

}}
