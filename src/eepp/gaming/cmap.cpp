#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/cgameobjectvirtual.hpp>
#include <eepp/gaming/cgameobjectsubtexture.hpp>
#include <eepp/gaming/cgameobjectsubtextureex.hpp>
#include <eepp/gaming/cgameobjectsprite.hpp>
#include <eepp/gaming/cgameobjectobject.hpp>
#include <eepp/gaming/cgameobjectpolygon.hpp>
#include <eepp/gaming/cgameobjectpolyline.hpp>
#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/gaming/cobjectlayer.hpp>

#include <eepp/system/cpackmanager.hpp>

#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/cprimitives.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>
#include <eepp/graphics/ctextureatlasloader.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
using namespace EE::Graphics;

#include <eepp/ui/cuithememanager.hpp>

namespace EE { namespace Gaming {

cMap::cMap() :
	mWindow( cEngine::instance()->GetCurrentWindow() ),
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

cMap::~cMap() {
	DeleteLayers();
	DisableForcedHeaders();
}

void cMap::Reset() {
	DeleteLayers();

	mWindow = NULL;
	mLayers = NULL;
	mData = NULL;
	mFlags	= 0;
	mMaxLayers	= 0;
	mMouseOver = false;
	mViewSize = eeSize( 800, 600 );
	mBaseColor = eeColorA( 255, 255, 255, 255 );
}

void cMap::ForceHeadersOnLoad( eeSize mapSize, eeSize tileSize, Uint32 numLayers, Uint32 flags ) {
	DisableForcedHeaders();
	mForcedHeaders = eeNew( cForcedHeaders, ( mapSize, tileSize, numLayers, flags ) );
}

void cMap::DisableForcedHeaders() {
	eeSAFE_DELETE( mForcedHeaders );
}

void cMap::DeleteLayers() {
	eeSAFE_DELETE( mLightManager );

	for ( Uint32 i = 0; i < mLayerCount; i++ )
		eeSAFE_DELETE( mLayers[i] );

	eeSAFE_DELETE_ARRAY( mLayers );

	mLayerCount = 0;
}

void cMap::Create( eeSize Size, Uint32 MaxLayers, eeSize TileSize, Uint32 Flags, eeSize viewSize, Window::cWindow * Window ) {
	Reset();

	mWindow		= Window;

	if ( NULL == mWindow )
		mWindow	= cEngine::instance()->GetCurrentWindow();

	mFlags		= Flags;
	mMaxLayers	= MaxLayers;
	mSize		= Size;
	mTileSize	= TileSize;
	mPixelSize	= Size * TileSize;
	mLayers		= eeNewArray( cLayer*, mMaxLayers );

	if ( LightsEnabled() )
		CreateLightManager();

	for ( Uint32 i = 0; i < mMaxLayers; i++ )
		mLayers[i] = NULL;

	ViewSize( viewSize );

	CreateEmptyTile();
}

void cMap::CreateLightManager() {
	eeSAFE_DELETE( mLightManager );
	mLightManager = eeNew( cLightManager, ( this, ( mFlags & MAP_FLAG_LIGHTS_BYVERTEX ) ? true : false ) );
}

void cMap::CreateEmptyTile() {
	//! I create a texture representing an empty tile to render instead of rendering with primitives because is a lot faster, at least with NVIDIA GPUs.
	cTextureFactory * TF = cTextureFactory::instance();
	std::string tileName( "maptile-" + String::ToStr( mTileSize.Width() ) + "x" + String::ToStr( mTileSize.Height() ) );

	cTexture * Tex = TF->GetByName( tileName );

	if ( NULL == Tex ) {
		Uint32 x, y;
		eeColorA Col( mGridLinesColor );

		cImage Img( mTileSize.Width(), mTileSize.Height(), 4 );

		Img.FillWithColor( eeColorA( 0, 0, 0, 0 ) );

		for ( x = 0; x < Img.Width(); x++ ) {
			Img.SetPixel( x, 0, Col );
			Img.SetPixel( x, mTileSize.y - 1, Col );
		}

		for ( y = 0; y < Img.Height(); y++ ) {
			Img.SetPixel( 0, y, Col );
			Img.SetPixel( mTileSize.x - 1, y, Col );
		}

		Uint32 TileTexId = TF->LoadFromPixels(
			Img.GetPixelsPtr(),
			Img.Width(),
			Img.Height(),
			Img.Channels(),
			true,
			CLAMP_TO_EDGE,
			false,
			false,
			tileName
		);

		mTileTex = TF->GetTexture( TileTexId );
	} else {
		mTileTex = Tex;
	}
}

cLayer * cMap::AddLayer( Uint32 Type, Uint32 flags, std::string name ) {
	eeASSERT( NULL != mLayers );

	if ( mLayerCount >= mMaxLayers )
		return NULL;

	switch ( Type ) {
		case MAP_LAYER_TILED:
			mLayers[ mLayerCount ] = eeNew( cTileLayer, ( this, mSize, flags, name ) );
			break;
		case MAP_LAYER_OBJECT:
			mLayers[ mLayerCount ] = eeNew( cObjectLayer, ( this, flags, name ) );
			break;
		default:
			return NULL;
	}

	mLayerCount++;

	return mLayers[ mLayerCount - 1 ];
}

cLayer* cMap::GetLayer( Uint32 index ) {
	eeASSERT( index < mLayerCount );
	return mLayers[ index ];
}

cLayer* cMap::GetLayerByHash( Uint32 hash ) {
	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Id() == hash )
			return mLayers[i];
	}

	return NULL;
}

Uint32 cMap::GetLayerIndex( cLayer * Layer ) {
	if ( NULL != Layer ) {
		for ( Uint32 i = 0; i < mLayerCount; i++ ) {
			if ( mLayers[i] == Layer )
				return i;
		}
	}

	return EE_MAP_LAYER_UNKNOWN;
}

cLayer* cMap::GetLayer( const std::string& name ) {
	return GetLayerByHash( String::Hash( name ) );
}

void cMap::Draw() {
	cGlobalBatchRenderer::instance()->Draw();

	if ( ClipedArea() ) {
		mWindow->ClipEnable( mScreenPos.x, mScreenPos.y, mViewSize.x, mViewSize.y );
	}

	if ( DrawBackground() ) {
		cPrimitives P;

		Uint8 Alpha = static_cast<Uint8>( (eeFloat)mBackColor.A() * ( (eeFloat)mBackAlpha / 255.f ) );

		P.SetColor( eeColorA( mBackColor.R(), mBackColor.G(), mBackColor.B(), Alpha ) );
		P.DrawRectangle( eeRectf( eeVector2f( mScreenPos.x, mScreenPos.y ), eeSizef( mViewSize.x, mViewSize.y ) ), 0.f, eeVector2f::One );
		P.SetColor( eeColorA( 255, 255, 255, 255 ) );
	}

	GLfloat oldM[16];
	GLi->GetCurrentMatrix( GL_MODELVIEW_MATRIX, oldM );
	GLi->LoadIdentity();
	GLi->PushMatrix();
	GLi->Translatef( (eeFloat)static_cast<Int32>( mScreenPos.x + mOffset.x ), (eeFloat)static_cast<Int32>( mScreenPos.y + mOffset.y ), 0 );
	GLi->Scalef( mScale, mScale, 0 );

	GridDraw();

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Visible() )
			mLayers[i]->Draw();
	}

	MouseOverDraw();

	if ( mDrawCb.IsSet() )
		mDrawCb();

	cGlobalBatchRenderer::instance()->Draw();

	GLi->PopMatrix();
	GLi->LoadMatrixf( oldM );

	if ( ClipedArea() ) {
		mWindow->ClipDisable();
	}
}

void cMap::MouseOverDraw() {
	if ( !DrawTileOver() || NULL == mTileTex )
		return;

	mTileTex->Draw( mMouseOverTileFinal.x * mTileSize.x, mMouseOverTileFinal.y * mTileSize.y, 0, eeVector2f::One, mTileOverColor );
}

void cMap::GridDraw() {
	if ( !DrawGrid() )
		return;

	if ( 0 == mSize.x || 0 == mSize.y || NULL == mTileTex )
		return;

	cGlobalBatchRenderer::instance()->Draw();

	eeVector2i start = StartTile();
	eeVector2i end = EndTile();

	eeFloat tx, ty;
	eeColorA TileTexCol( 255, 255, 255, mBackAlpha );

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			tx = x * mTileSize.x;

			ty = y * mTileSize.y;

			if ( LightsEnabled() ) {
				eeVector2i TPos( x, y );

				if ( mLightManager->IsByVertex() ) {
					eeColorA TileTexCol0( *mLightManager->GetTileColor( TPos, 0 ) );
					eeColorA TileTexCol1( *mLightManager->GetTileColor( TPos, 1 ) );
					eeColorA TileTexCol2( *mLightManager->GetTileColor( TPos, 2 ) );
					eeColorA TileTexCol3( *mLightManager->GetTileColor( TPos, 3 ) );

					TileTexCol0.Alpha = TileTexCol1.Alpha = TileTexCol2.Alpha = TileTexCol3.Alpha	= mBackAlpha;

					mTileTex->DrawEx( tx, ty, 0, 0, 0, eeVector2f::One, TileTexCol0, TileTexCol1, TileTexCol2, TileTexCol3 );
				} else {
					TileTexCol			= *mLightManager->GetTileColor( TPos );
					TileTexCol.Alpha	= mBackAlpha;

					mTileTex->Draw( tx, ty, 0, eeVector2f::One, TileTexCol );
				}
			} else {
				mTileTex->Draw( tx, ty, 0, eeVector2f::One, TileTexCol );
			}
		}
	}

	cGlobalBatchRenderer::instance()->Draw();
}

const bool& cMap::IsMouseOver() const {
	return mMouseOver;
}

void cMap::GetMouseOverTile() {
	eeVector2i mouse = mWindow->GetInput()->GetMousePos();

	eeVector2i MapPos( static_cast<eeFloat>( mouse.x - mScreenPos.x - mOffset.x ) / mScale, static_cast<eeFloat>( mouse.y - mScreenPos.y - mOffset.y ) / mScale );

	mMouseOver = !( MapPos.x < 0 || MapPos.y < 0 || MapPos.x > mPixelSize.x || MapPos.y > mPixelSize.y );

	MapPos.x = eemax( MapPos.x, 0 );
	MapPos.y = eemax( MapPos.y, 0 );
	MapPos.x = eemin( MapPos.x, mPixelSize.x );
	MapPos.y = eemin( MapPos.y, mPixelSize.y );

	mMouseOverTile.x = MapPos.x / mTileSize.Width();
	mMouseOverTile.y = MapPos.y / mTileSize.Height();

	// Clamped pos
	mMouseOverTileFinal.x = eemin( mMouseOverTile.x, mSize.Width()	- 1 );
	mMouseOverTileFinal.y = eemin( mMouseOverTile.y, mSize.Height()	- 1 );
	mMouseOverTileFinal.x = eemax( mMouseOverTileFinal.x, 0 );
	mMouseOverTileFinal.y = eemax( mMouseOverTileFinal.y, 0 );

	mMouseMapPos = MapPos;
}

void cMap::CalcTilesClip() {
	if ( mTileSize.x > 0 && mTileSize.y > 0 ) {
		eeVector2f ffoff( mOffset );
		eeVector2i foff( (Int32)ffoff.x, (Int32)ffoff.y );

		mStartTile.x	= -foff.x / ( mTileSize.x * mScale ) - mExtraTiles.x;
		mStartTile.y	= -foff.y / ( mTileSize.y * mScale ) - mExtraTiles.y;

		if ( mStartTile.x < 0 )
			mStartTile.x = 0;

		if ( mStartTile.y < 0 )
			mStartTile.y = 0;

		mEndTile.x		= mStartTile.x + Math::RoundUp( (eeFloat)mViewSize.x / ( (eeFloat)mTileSize.x * mScale ) ) + 1 + mExtraTiles.x;
		mEndTile.y		= mStartTile.y + Math::RoundUp( (eeFloat)mViewSize.y / ( (eeFloat)mTileSize.y * mScale ) ) + 1 + mExtraTiles.y;

		if ( mEndTile.x > mSize.x )
			mEndTile.x = mSize.x;

		if ( mEndTile.y > mSize.y )
			mEndTile.y = mSize.y;
	}
}

void cMap::Clamp() {
	if ( !ClampBorders() )
		return;

	if ( mOffset.x > 0 )
		mOffset.x = 0;

	if ( mOffset.y > 0 )
		mOffset.y = 0;

	eeVector2f totSize( mTileSize.x * mSize.x * mScale, mTileSize.y * mSize.y * mScale );

	if ( -mOffset.x + mViewSize.x > totSize.x )
		mOffset.x = -( totSize.x - mViewSize.x );

	if ( -mOffset.y + mViewSize.y > totSize.y )
		mOffset.y = -( totSize.y - mViewSize.y );

	if ( totSize.x < mViewSize.x )
		mOffset.x = 0;

	if ( totSize.y < mViewSize.y )
		mOffset.y = 0;

	totSize.x = (Int32)( (eeFloat)( mTileSize.x * mSize.x ) * mScale );
	totSize.y = (Int32)( (eeFloat)( mTileSize.y * mSize.y ) * mScale );

	if ( -mOffset.x + mViewSize.x > totSize.x )
		mOffset.x = -( totSize.x - mViewSize.x );

	if ( -mOffset.y + mViewSize.y > totSize.y )
		mOffset.y = -( totSize.y - mViewSize.y );

	if ( totSize.x < mViewSize.x )
		mOffset.x = 0;

	if ( totSize.y < mViewSize.y )
		mOffset.y = 0;
}

void cMap::Offset( const eeVector2f& offset ) {
	mOffset			= offset;

	Clamp();

	CalcTilesClip();
}

eeVector2i cMap::GetMaxOffset() {
	eeVector2i v(  ( mTileSize.x * mSize.x * mScale ) - mViewSize.x,
				   ( mTileSize.y * mSize.y * mScale ) - mViewSize.y );

	eemax( 0, v.x );
	eemax( 0, v.y );

	return v;
}

const eeFloat& cMap::Scale() const {
	return mScale;
}

void cMap::Scale( const eeFloat& scale ) {
	mScale = scale;

	Offset( mOffset );
}

void cMap::UpdateScreenAABB() {
	mScreenAABB = eeAABB( -mOffset.x, -mOffset.y, -mOffset.x + mViewSize.Width(), -mOffset.y + mViewSize.Height() );
}

const eeAABB& cMap::GetViewAreaAABB() const {
	return mScreenAABB;
}

void cMap::Update() {
	GetMouseOverTile();

	UpdateScreenAABB();

	if ( NULL != mLightManager )
		mLightManager->Update();

	for ( Uint32 i = 0; i < mLayerCount; i++ )
		mLayers[i]->Update();

	if ( mUpdateCb.IsSet() )
		mUpdateCb();
}

const eeSize& cMap::ViewSize() const {
	return mViewSize;
}

const eeVector2i& cMap::GetMouseTilePos() const {
	return mMouseOverTileFinal;
}

const eeVector2i& cMap::GetRealMouseTilePos() const {
	return mMouseOverTile;
}

const eeVector2i& cMap::GetMouseMapPos() const {
	return mMouseMapPos;
}

eeVector2f cMap::GetMouseMapPosf() const {
	return eeVector2f( (eeFloat)mMouseMapPos.x, (eeFloat)mMouseMapPos.y );
}

eeVector2i cMap::GetMouseTilePosCoords() {
	return GetTileCoords( GetMouseTilePos() );
}

eeVector2i cMap::GetTileCoords( const eeVector2i& TilePos ) {
	return ( TilePos * mTileSize );
}

void cMap::ViewSize( const eeSize& viewSize ) {
	mViewSize = viewSize;

	Clamp();

	CalcTilesClip();
}

const eeVector2i& cMap::Position() const {
	return mScreenPos;
}

void cMap::Position( const eeVector2i& position ) {
	mScreenPos = position;
}

const eeVector2f& cMap::Offset() const {
	return mOffset;
}

const eeVector2i& cMap::StartTile() const {
	return mStartTile;
}

const eeVector2i& cMap::EndTile() const {
	return mEndTile;
}

void cMap::ExtraTiles( const eeVector2i& extra ) {
	mExtraTiles = extra;
}

const eeVector2i& cMap::ExtraTiles() const {
	return mExtraTiles;
}

void cMap::BaseColor( const eeColorA& color ) {
	mBaseColor = color;
}

const eeColorA& cMap::BaseColor() const {
	return mBaseColor;
}

void cMap::DrawGrid( const bool& draw ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_DRAW_GRID, draw ? 1 : 0 );
}

Uint32 cMap::DrawGrid() const {
	return mFlags & MAP_FLAG_DRAW_GRID;
}

void cMap::DrawBackground( const bool& draw ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_DRAW_BACKGROUND, draw ? 1 : 0 );
}

void cMap::ShowBlocked( const bool& show ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_SHOW_BLOCKED, show ? 1 : 0 );
}

Uint32 cMap::ShowBlocked() const {
	return mFlags & MAP_FLAG_SHOW_BLOCKED;
}

Uint32 cMap::DrawBackground() const {
	return mFlags & MAP_FLAG_DRAW_BACKGROUND;
}

bool cMap::ClipedArea() const {
	return 0 != ( mFlags & MAP_FLAG_CLIP_AREA );
}

void cMap::ClipedArea( const bool& clip ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_CLIP_AREA, clip ? 1 : 0 );
}

bool cMap::ClampBorders() const {
	return 0 != ( mFlags & MAP_FLAG_CLAMP_BORDERS );
}

void cMap::ClampBorders( const bool& clamp ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_CLAMP_BORDERS, clamp ? 1 : 0 );
}

Uint32 cMap::DrawTileOver() const {
	return 0 != ( mFlags & MAP_FLAG_DRAW_TILE_OVER );
}

void cMap::DrawTileOver( const bool& draw ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_DRAW_TILE_OVER, draw ? 1 : 0 );
}

bool cMap::LightsEnabled() {
	return 0 != ( mFlags & MAP_FLAG_LIGHTS_ENABLED );
}

void cMap::LightsEnabled( const bool& enabled ) {
	BitOp::SetBitFlagValue( &mFlags, MAP_FLAG_LIGHTS_ENABLED, enabled ? 1 : 0 );
}

bool cMap::LightsByVertex() {
	return 0 != ( mFlags & MAP_FLAG_LIGHTS_BYVERTEX );
}

void cMap::Move( const eeVector2f& offset )  {
	Move( offset.x, offset.y );
}

void cMap::Move( const eeFloat& offsetx, const eeFloat& offsety ) {
	Offset( eeVector2f( mOffset.x + offsetx, mOffset.y + offsety ) );
}

cGameObjectPolyData& cMap::GetPolyObjData( Uint32 Id ) {
	return mPolyObjs[ Id ];
}

cGameObject * cMap::CreateGameObject( const Uint32& Type, const Uint32& Flags, cLayer * Layer, const Uint32& DataId ) {
	switch ( Type ) {
		case GAMEOBJECT_TYPE_SUBTEXTURE:
		{
			cGameObjectSubTexture * tSubTexture = eeNew( cGameObjectSubTexture, ( Flags, Layer ) );

			tSubTexture->DataId( DataId );

			return tSubTexture;
		}
		case GAMEOBJECT_TYPE_SUBTEXTUREEX:
		{
			cGameObjectSubTextureEx * tSubTextureEx = eeNew( cGameObjectSubTextureEx, ( Flags, Layer ) );

			tSubTextureEx->DataId( DataId );

			return tSubTextureEx;
		}
		case GAMEOBJECT_TYPE_SPRITE:
		{
			cGameObjectSprite * tSprite = eeNew( cGameObjectSprite, ( Flags, Layer ) );

			tSprite->DataId( DataId );

			return tSprite;
		}
		case GAMEOBJECT_TYPE_OBJECT:
		case GAMEOBJECT_TYPE_POLYGON:
		case GAMEOBJECT_TYPE_POLYLINE:
		{
			cGameObjectPolyData& ObjData = GetPolyObjData( DataId );

			cGameObjectObject * tObject = NULL;

			if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
				tObject = eeNew( cGameObjectObject, ( DataId, ObjData.Poly.ToAABB(), Layer, Flags ) );
			} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
				tObject = eeNew( cGameObjectPolygon, ( DataId, ObjData.Poly, Layer, Flags ) );
			} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
				tObject = eeNew( cGameObjectPolyline, ( DataId, ObjData.Poly, Layer, Flags ) );
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
				cGameObjectVirtual * tVirtual;
				cSubTexture * tIsSubTexture = cTextureAtlasManager::instance()->GetSubTextureById( DataId );

				if ( NULL != tIsSubTexture ) {
					tVirtual = eeNew( cGameObjectVirtual, ( tIsSubTexture, Layer, Flags, Type ) );
				} else {
					tVirtual = eeNew( cGameObjectVirtual, ( DataId, Layer, Flags, Type ) );
				}

				return tVirtual;
			}
		}
	}

	return NULL;
}

cLightManager * cMap::GetLightManager() const {
	return mLightManager;
}

const eeSize& cMap::TotalSize() const {
	return mPixelSize;
}

const eeSize& cMap::TileSize() const {
	return mTileSize;
}

const eeSize& cMap::Size() const {
	return mSize;
}

const Uint32& cMap::LayerCount() const {
	return mLayerCount;
}

const Uint32& cMap::MaxLayers() const {
	return mMaxLayers;
}

bool cMap::MoveLayerUp( cLayer * Layer ) {
	Uint32 Lindex = GetLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN && mLayerCount > 1 && ( Lindex < mLayerCount - 1 ) && ( Lindex + 1 < mLayerCount ) ) {
		cLayer * tLayer = mLayers[ Lindex + 1 ];

		mLayers[ Lindex ]		= tLayer;
		mLayers[ Lindex + 1 ]	= Layer;

		return true;
	}

	return false;
}

bool cMap::MoveLayerDown( cLayer * Layer ) {
	Uint32 Lindex = GetLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN && mLayerCount > 1 && Lindex >= 1 ) {
		cLayer * tLayer = mLayers[ Lindex - 1 ];

		mLayers[ Lindex ]		= tLayer;
		mLayers[ Lindex - 1 ]	= Layer;

		return true;
	}

	return false;
}

bool cMap::RemoveLayer( cLayer * Layer ) {
	Uint32 Lindex = GetLayerIndex( Layer );

	if ( Lindex != EE_MAP_LAYER_UNKNOWN ) {
		eeSAFE_DELETE( mLayers[ Lindex ] );

		cLayer * LastLayer = NULL;

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

void cMap::ClearProperties() {
	mProperties.clear();
}

void cMap::AddProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void cMap::EditProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void cMap::RemoveProperty( std::string Text ) {
	mProperties.erase( Text );
}

cMap::PropertiesMap& cMap::GetProperties() {
	return mProperties;
}

void cMap::AddVirtualObjectType( const std::string& name ) {
	mObjTypes.push_back( name );
	mObjTypes.unique();
}

void cMap::RemoveVirtualObjectType( const std::string& name ) {
	mObjTypes.remove( name );
}

void cMap::ClearVirtualObjectTypes() {
	mObjTypes.clear();
}

cMap::GOTypesList& cMap::GetVirtualObjectTypes() {
	return mObjTypes;
}

void cMap::SetCreateGameObjectCallback( const CreateGOCb& Cb ) {
	mCreateGOCb = Cb;
}

bool cMap::LoadFromStream( cIOStream& IOS ) {
	sMapHdr MapHdr;
	Uint32 i, z;

	if ( IOS.IsOpen() ) {
		IOS.Read( (char*)&MapHdr, sizeof(sMapHdr) );

		if ( MapHdr.Magic == EE_MAP_MAGIC ) {
			if ( NULL == mForcedHeaders ) {
				Create( eeSize( MapHdr.SizeX, MapHdr.SizeY ), MapHdr.MaxLayers, eeSize( MapHdr.TileSizeX, MapHdr.TileSizeY ), MapHdr.Flags );
			} else {
				Create( mForcedHeaders->MapSize, mForcedHeaders->NumLayers, mForcedHeaders->TileSize, mForcedHeaders->Flags );
			}

			BaseColor( eeColorA( MapHdr.BaseColor ) );

			//! Load Properties
			if ( MapHdr.PropertyCount ) {
				sPropertyHdr * tProp = eeNewArray( sPropertyHdr, MapHdr.PropertyCount );

				IOS.Read( (char*)&tProp[0], sizeof(sPropertyHdr) * MapHdr.PropertyCount );

				for ( i = 0; i < MapHdr.PropertyCount; i++ ) {
					AddProperty( std::string( tProp[i].Name ), std::string( tProp[i].Value ) );
				}

				eeSAFE_DELETE_ARRAY( tProp );
			}

			//! Load Texture Atlases
			if ( MapHdr.TextureAtlasCount ) {
				sMapTextureAtlas * tSG = eeNewArray( sMapTextureAtlas, MapHdr.TextureAtlasCount );

				IOS.Read( (char*)&tSG[0], sizeof(sMapTextureAtlas) * MapHdr.TextureAtlasCount );

				std::vector<std::string> TextureAtlases;

				for ( i = 0; i < MapHdr.TextureAtlasCount; i++ ) {
					TextureAtlases.push_back( std::string( tSG[i].Path ) );
				}

				//! Load the Texture Atlases if needed
				for ( i = 0; i < TextureAtlases.size(); i++ ) {
					std::string sgname = FileSystem::FileRemoveExtension( FileSystem::FileNameFromPath( TextureAtlases[i] ) );

					if ( NULL == cTextureAtlasManager::instance()->GetByName( sgname ) ) {
						cTextureAtlasLoader * tgl = eeNew( cTextureAtlasLoader, () );

						tgl->Load( Sys::GetProcessPath() + TextureAtlases[i] );

						eeSAFE_DELETE( tgl );
					}
				}

				eeSAFE_DELETE_ARRAY( tSG );
			}

			//! Load Virtual Object Types
			if ( MapHdr.VirtualObjectTypesCount ) {
				sVirtualObj * tVObj = eeNewArray( sVirtualObj, MapHdr.VirtualObjectTypesCount );

				IOS.Read( (char*)&tVObj[0], sizeof(sVirtualObj) * MapHdr.VirtualObjectTypesCount );

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
					IOS.Read( (char*)&tLayersHdr[i], sizeof(sLayerHdr) );

					tLayerHdr = &(tLayersHdr[i]);

					cLayer * tLayer = AddLayer( tLayerHdr->Type, tLayerHdr->Flags, std::string( tLayerHdr->Name ) );

					if ( NULL != tLayer ) {
						tLayer->Offset( eeVector2f( (eeFloat)tLayerHdr->OffsetX, (eeFloat)tLayerHdr->OffsetY ) );

						sPropertyHdr * tProps = eeNewArray( sPropertyHdr, tLayerHdr->PropertyCount );

						IOS.Read( (char*)&tProps[0], sizeof(sPropertyHdr) * tLayerHdr->PropertyCount );

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
				cTileLayer * tTLayer;
				cGameObject * tGO;

				if ( NULL != mForcedHeaders ) {
					mSize = eeSize( MapHdr.SizeX, MapHdr.SizeY );
				}

				if ( ThereIsTiled ) {
					//! First we read the tiled layers.
					for ( y = 0; y < mSize.y; y++ ) {
						for ( x = 0; x < mSize.x; x++ ) {

							//! Read the current tile flags
							IOS.Read( (char*)&tReadFlag, sizeof(Uint32) );

							//! Read every game object header corresponding to this tile
							for ( i = 0; i < mLayerCount; i++ ) {
								if ( tReadFlag & ( 1 << i ) ) {
									tTLayer = reinterpret_cast<cTileLayer*> ( mLayers[i] );

									sMapTileGOHdr tTGOHdr;

									IOS.Read( (char*)&tTGOHdr, sizeof(sMapTileGOHdr) );

									tGO = CreateGameObject( tTGOHdr.Type, tTGOHdr.Flags, mLayers[i], tTGOHdr.Id );

									tTLayer->AddGameObject( tGO, eeVector2i( x, y ) );
								}
							}
						}
					}
				}

				if ( NULL != mForcedHeaders ) {
					mSize = mForcedHeaders->MapSize;
				}

				//! Load the game objects from the object layers
				cObjectLayer * tOLayer;

				for ( i = 0; i < mLayerCount; i++ ) {
					if ( NULL != mLayers[i] && mLayers[i]->Type() == MAP_LAYER_OBJECT ) {
						tLayerHdr	= &( tLayersHdr[i] );
						tOLayer		= reinterpret_cast<cObjectLayer*> ( mLayers[i] );

						for ( Uint32 objCount = 0; objCount < tLayerHdr->ObjectCount; objCount++ ) {
							sMapObjGOHdr tOGOHdr;

							IOS.Read( (char*)&tOGOHdr, sizeof(sMapObjGOHdr) );

							//! For the polygon objects wee need to read the polygon points, the Name, the TypeName and the Properties.
							if (	tOGOHdr.Type == GAMEOBJECT_TYPE_OBJECT		||
									tOGOHdr.Type == GAMEOBJECT_TYPE_POLYGON		||
									tOGOHdr.Type == GAMEOBJECT_TYPE_POLYLINE )
							{
								cGameObjectPolyData tObjData;

								//! First we read the poly obj header
								sMapObjObjHdr tObjObjHdr;

								IOS.Read( (char*)&tObjObjHdr, sizeof(sMapObjObjHdr) );

								tObjData.Name = std::string( tObjObjHdr.Name );
								tObjData.Type = std::string( tObjObjHdr.Type );

								//! Reads the properties
								for ( Uint32 iProp = 0; iProp < tObjObjHdr.PropertyCount; iProp++ ) {
									sPropertyHdr tObjProp;

									IOS.Read( (char*)&tObjProp, sizeof(sPropertyHdr) );

									tObjData.Properties[ std::string( tObjProp.Name ) ] = std::string( tObjProp.Value );
								}

								//! Reads the polygon points
								for ( Uint32 iPoint = 0; iPoint < tObjObjHdr.PointCount; iPoint++ ) {
									eeVector2if p;

									IOS.Read( (char*)&p, sizeof(eeVector2if) );

									tObjData.Poly.PushBack( eeVector2f( p.x, p.y ) );
								}

								mPolyObjs[ tOGOHdr.Id ] = tObjData;

								//! Recover the last max id
								mLastObjId	= eemax( mLastObjId, tOGOHdr.Id );
							}

							tGO = CreateGameObject( tOGOHdr.Type, tOGOHdr.Flags, mLayers[i], tOGOHdr.Id );

							tGO->Pos( eeVector2f( tOGOHdr.PosX, tOGOHdr.PosY ) );

							tOLayer->AddGameObject( tGO );
						}
					}
				}

				//! Load the lights
				if ( MapHdr.LightsCount ) {
					CreateLightManager();

					sMapLightHdr * tLighsHdr = eeNewArray( sMapLightHdr, MapHdr.LightsCount );
					sMapLightHdr * tLightHdr;

					IOS.Read( (char*)tLighsHdr, sizeof(sMapLightHdr) * MapHdr.LightsCount );

					for ( i = 0; i < MapHdr.LightsCount; i++ ) {
						tLightHdr = &( tLighsHdr[ i ] );

						mLightManager->AddLight(
							eeNew( cLight, ( tLightHdr->Radius, tLightHdr->PosX, tLightHdr->PosY, eeColorA( tLightHdr->Color ).ToColor(), (LIGHT_TYPE)tLightHdr->Type ) )
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

const std::string& cMap::Path() const {
	return mPath;
}

bool cMap::Load( const std::string& path ) {
	if ( FileSystem::FileExists( path ) ) {
		mPath = path;

		cIOStreamFile IOS( mPath, std::ios::in | std::ios::binary );

		return LoadFromStream( IOS );
	} else if ( cPackManager::instance()->FallbackToPacks() ) {
		std::string tPath( path );
		cPack * tPack = cPackManager::instance()->Exists( tPath ) ;

		if ( NULL != tPack ) {
			mPath = tPath;
			return LoadFromPack( tPack, tPath );
		}
	}

	return false;
}

bool cMap::LoadFromPack( cPack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		return LoadFromMemory( reinterpret_cast<const char*> ( PData.Data ), PData.DataSize );
	}

	return false;
}

bool cMap::LoadFromMemory( const char * Data, const Uint32& DataSize ) {
	cIOStreamMemory IOS( Data, DataSize );

	return LoadFromStream( IOS );
}

void cMap::SaveToStream( cIOStream& IOS ) {
	Uint32 i;
	sMapHdr MapHdr;
	cLayer * tLayer;

	std::vector<std::string> TextureAtlases = GetTextureAtlases();

	MapHdr.Magic					= EE_MAP_MAGIC;
	MapHdr.Flags					= mFlags;
	MapHdr.MaxLayers				= mMaxLayers;
	MapHdr.SizeX					= mSize.Width();
	MapHdr.SizeY					= mSize.Height();
	MapHdr.TileSizeX				= mTileSize.Width();
	MapHdr.TileSizeY				= mTileSize.Height();
	MapHdr.LayerCount				= mLayerCount;
	MapHdr.PropertyCount			= mProperties.size();
	MapHdr.TextureAtlasCount		= TextureAtlases.size();
	MapHdr.VirtualObjectTypesCount	= mObjTypes.size();	//! This is only useful for the Map Editor, to auto add on the load the virtual object types that where used to create the map.
	MapHdr.BaseColor				= mBaseColor.GetValue();

	if ( LightsEnabled() && NULL != mLightManager )
		MapHdr.LightsCount = mLightManager->Count();
	else
		MapHdr.LightsCount = 0;

	if ( IOS.IsOpen() ) {
		//! Writes the map header
		IOS.Write( (const char*)&MapHdr, sizeof(sMapHdr) );

		//! Writes the properties of the map
		for ( cMap::PropertiesMap::iterator it = mProperties.begin(); it != mProperties.end(); it++ ) {
			sPropertyHdr tProp;

			memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
			memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

			String::StrCopy( tProp.Name, it->first.c_str(), MAP_PROPERTY_SIZE );
			String::StrCopy( tProp.Value, it->second.c_str(), MAP_PROPERTY_SIZE );

			IOS.Write( (const char*)&tProp, sizeof(sPropertyHdr) );
		}

		//! Writes the texture atlases that the map will need and load
		for ( i = 0; i < TextureAtlases.size(); i++ ) {
			sMapTextureAtlas tSG;

			memset( tSG.Path, 0, MAP_TEXTUREATLAS_PATH_SIZE );

			String::StrCopy( tSG.Path, TextureAtlases[i].c_str(), MAP_TEXTUREATLAS_PATH_SIZE );

			IOS.Write( (const char*)&tSG, sizeof(sMapTextureAtlas) );
		}

		//! Writes the names of the virtual object types created in the map editor
		for ( GOTypesList::iterator votit = mObjTypes.begin(); votit != mObjTypes.end(); votit++ ) {
			sVirtualObj tVObjH;

			memset( tVObjH.Name, 0, MAP_PROPERTY_SIZE );

			String::StrCopy( tVObjH.Name, (*votit).c_str(), MAP_PROPERTY_SIZE );

			IOS.Write( (const char*)&tVObjH, sizeof(sVirtualObj) );
		}

		//! Writes every layer header
		for ( i = 0; i < mLayerCount; i++ ) {
			tLayer = mLayers[i];
			sLayerHdr tLayerH;

			memset( tLayerH.Name, 0, LAYER_NAME_SIZE );

			String::StrCopy( tLayerH.Name, tLayer->Name().c_str(), LAYER_NAME_SIZE );

			tLayerH.Type			= tLayer->Type();
			tLayerH.Flags			= tLayer->Flags();
			tLayerH.OffsetX			= tLayer->Offset().x;
			tLayerH.OffsetY			= tLayer->Offset().y;

			if ( MAP_LAYER_OBJECT == tLayerH.Type )
				tLayerH.ObjectCount = reinterpret_cast<cObjectLayer*> ( tLayer )->GetObjectCount();
			else
				tLayerH.ObjectCount		= 0;

			cLayer::PropertiesMap& tLayerProp = tLayer->GetProperties();

			tLayerH.PropertyCount	= tLayerProp.size();

			//! Writes the layer header
			IOS.Write( (const char*)&tLayerH, sizeof(sLayerHdr) );

			//! Writes the properties of the current layer
			for ( cLayer::PropertiesMap::iterator lit = tLayerProp.begin(); lit != tLayerProp.end(); lit++ ) {
				sPropertyHdr tProp;

				memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
				memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

				String::StrCopy( tProp.Name, (*lit).first.c_str(), MAP_PROPERTY_SIZE );
				String::StrCopy( tProp.Value, (*lit).second.c_str(), MAP_PROPERTY_SIZE );

				IOS.Write( (const char*)&tProp, sizeof(sPropertyHdr) );
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
		cTileLayer * tTLayer;
		cGameObject * tObj;

		std::vector<cGameObject*> tObjects( mLayerCount );

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
							tTLayer = reinterpret_cast<cTileLayer*> ( tLayer );

							tObj = tTLayer->GetGameObject( eeVector2i( x, y ) );

							if ( NULL != tObj ) {
								tReadFlag |= 1 << i;

								tObjects[i] = tObj;
							}
						}
					}

					//! Writes the current tile flags
					IOS.Write( (const char*)&tReadFlag, sizeof(Uint32) );

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
								cGameObjectVirtual * tObjV = reinterpret_cast<cGameObjectVirtual*> ( tObj );

								tTGOHdr.Type	= tObjV->RealType();
							}

							tTGOHdr.Flags	= tObj->Flags();

							IOS.Write( (const char*)&tTGOHdr, sizeof(sMapTileGOHdr) );
						}
					}
				}
			}
		}

		//! Then we save the Object layers.
		cObjectLayer * tOLayer;

		for ( i = 0; i < mLayerCount; i++ ) {
			tLayer = mLayers[i];

			if ( NULL != tLayer && tLayer->Type() == MAP_LAYER_OBJECT ) {
				tOLayer = reinterpret_cast<cObjectLayer*> ( tLayer );

				cObjectLayer::ObjList ObjList = tOLayer->GetObjectList();

				for ( cObjectLayer::ObjList::iterator MapObjIt = ObjList.begin(); MapObjIt != ObjList.end(); MapObjIt++ ) {
					tObj = (*MapObjIt);

					sMapObjGOHdr tOGOHdr;

					//! The DataId should be the SubTexture hash name ( at least in the cases of type SubTexture, SubTextureEx and Sprite.
					//! And for the Poly Obj should be an arbitrary value assigned by the map on the moment of creation
					tOGOHdr.Id		= tObj->DataId();

					//! If the object type is virtual, means that the real type is stored elsewhere.
					if ( tObj->Type() != GAMEOBJECT_TYPE_VIRTUAL ) {
						tOGOHdr.Type	= tObj->Type();
					} else {
						cGameObjectVirtual * tObjV = reinterpret_cast<cGameObjectVirtual*> ( tObj );

						tOGOHdr.Type	= tObjV->RealType();
					}

					tOGOHdr.Flags	= tObj->Flags();

					tOGOHdr.PosX	= (Int32)tObj->Pos().x;

					tOGOHdr.PosY	= (Int32)tObj->Pos().y;

					IOS.Write( (const char*)&tOGOHdr, sizeof(sMapObjGOHdr) );

					//! For the polygon objects wee need to write the polygon points, the Name, the TypeName and the Properties.
					if (	tObj->Type() == GAMEOBJECT_TYPE_OBJECT		||
							tObj->Type() == GAMEOBJECT_TYPE_POLYGON		||
							tObj->Type() == GAMEOBJECT_TYPE_POLYLINE )
					{
						cGameObjectObject * tObjObj						= reinterpret_cast<cGameObjectObject*>( tObj );
						eePolygon2f tPoly								= tObjObj->GetPolygon();
						cGameObjectObject::PropertiesMap tObjObjProp	= tObjObj->GetProperties();
						sMapObjObjHdr tObjObjHdr;

						memset( tObjObjHdr.Name, 0, MAP_PROPERTY_SIZE );
						memset( tObjObjHdr.Type, 0, MAP_PROPERTY_SIZE );

						String::StrCopy( tObjObjHdr.Name, tObjObj->Name().c_str(), MAP_PROPERTY_SIZE );
						String::StrCopy( tObjObjHdr.Type, tObjObj->TypeName().c_str(), MAP_PROPERTY_SIZE );

						tObjObjHdr.PointCount		= tPoly.Size();
						tObjObjHdr.PropertyCount	= tObjObjProp.size();

						//! Writes the ObjObj header
						IOS.Write( (const char*)&tObjObjHdr, sizeof(sMapObjObjHdr) );

						//! Writes the properties of the current polygon object
						for ( cGameObjectObject::PropertiesMap::iterator ooit = tObjObjProp.begin(); ooit != tObjObjProp.end(); ooit++ ) {
							sPropertyHdr tProp;

							memset( tProp.Name, 0, MAP_PROPERTY_SIZE );
							memset( tProp.Value, 0, MAP_PROPERTY_SIZE );

							String::StrCopy( tProp.Name, ooit->first.c_str(), MAP_PROPERTY_SIZE );
							String::StrCopy( tProp.Value, ooit->second.c_str(), MAP_PROPERTY_SIZE );

							IOS.Write( (const char*)&tProp, sizeof(sPropertyHdr) );
						}

						//! Writes the polygon points
						for ( Uint32 tPoint = 0; tPoint < tPoly.Size(); tPoint++ ) {
							eeVector2f pf( tPoly.GetAt( tPoint ) );
							eeVector2if p( pf.x, pf.y );	//! Convert it to Int32

							IOS.Write( (const char*)&p, sizeof(eeVector2if) );
						}
					}
				}
			}
		}

		//! Saves the lights
		if ( MapHdr.LightsCount && NULL != mLightManager ) {
			cLightManager::LightsList& Lights = mLightManager->GetLights();

			for ( cLightManager::LightsList::iterator LightsIt = Lights.begin(); LightsIt != Lights.end(); LightsIt++ ) {
				cLight * Light = (*LightsIt);

				sMapLightHdr tLightHdr;

				tLightHdr.Radius	= Light->Radius();
				tLightHdr.PosX		= (Int32)Light->Position().x;
				tLightHdr.PosY		= (Int32)Light->Position().y;
				tLightHdr.Color		= eeColorA( Light->Color() ).GetValue();
				tLightHdr.Type		= Light->Type();

				IOS.Write( (const char*)&tLightHdr, sizeof(sMapLightHdr) );
			}
		}
	}
}

void cMap::Save( const std::string& path ) {
	if ( !FileSystem::IsDirectory( path ) ) {
		cIOStreamFile IOS( path, std::ios::out | std::ios::binary );

		SaveToStream( IOS );

		mPath = path;
	}
}

std::vector<std::string> cMap::GetTextureAtlases() {
	cTextureAtlasManager * SGM = cTextureAtlasManager::instance();
	std::list<cTextureAtlas*>& Res = SGM->GetResources();

	std::vector<std::string> items;

	//! Ugly ugly ugly, but i don't see another way
	Uint32 Restricted1 = String::Hash( std::string( "global" ) );
	Uint32 Restricted2 = String::Hash( UI::cUIThemeManager::instance()->DefaultTheme()->TextureAtlas()->Name() );

	for ( std::list<cTextureAtlas*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
		if ( (*it)->Id() != Restricted1 && (*it)->Id() != Restricted2 )
			items.push_back( (*it)->Path() );
	}

	return items;
}

void cMap::SetDrawCallback( MapDrawCb Cb ) {
	mDrawCb = Cb;
}

void cMap::SetUpdateCallback( MapUpdateCb Cb ) {
	mUpdateCb = Cb;
}

cTexture * cMap::GetBlankTileTexture() {
	return mTileTex;
}

bool cMap::IsTileBlocked( const eeVector2i& TilePos ) {
	cTileLayer * TLayer;
	cGameObject * TObj;

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Type() == MAP_LAYER_TILED ) {
			TLayer	= static_cast<cTileLayer*>( mLayers[i] );
			TObj	= TLayer->GetGameObject( TilePos );

			if ( NULL != TObj && TObj->Blocked() ) {
				return true;
			}
		}
	}

	return false;
}

void cMap::Data( void * value ) {
	mData = value;
}

void * cMap::Data() const {
	return mData;
}

void cMap::OnMapLoaded() {
}

cGameObject * cMap::IsTypeInTilePos( const Uint32& Type, const eeVector2i& TilePos ) {
	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		if ( mLayers[i]->Type() == MAP_LAYER_TILED ) {
			cTileLayer * tLayer = reinterpret_cast<cTileLayer*> ( mLayers[i] );
			cGameObject * tObj = NULL;

			if ( ( tObj = tLayer->GetGameObject( TilePos ) ) ) {
				if ( tObj->IsType( Type ) ) {
					return tObj;
				}
			}
		}
	}

	return NULL;
}

const Uint8& cMap::BackAlpha() const {
	return mBackAlpha;
}

void cMap::BackAlpha( const Uint8& alpha ) {
	mBackAlpha = alpha;
}

const eeColorA& cMap::BackColor() const {
	return mBackColor;
}

void cMap::BackColor( const eeColorA& col ) {
	mBackColor = col;
}

Uint32 cMap::GetNewObjectId() {
	return ++mLastObjId;
}

void cMap::GridLinesColor( const eeColorA& Col ) {
	mGridLinesColor = Col;
}

const eeColorA& cMap::GridLinesColor() const {
	return mGridLinesColor;
}

}}
