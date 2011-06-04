#include "cmap.hpp"
#include "cgameobjectshape.hpp"
#include "cgameobjectshapeex.hpp"
#include "cgameobjectsprite.hpp"
#include "ctilelayer.hpp"
#include "cobjectlayer.hpp"

#include "../graphics/cprimitives.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cMap::cMap() :
	mWindow( NULL ),
	mLayers( NULL ),
	mFlags( 0 ),
	mMaxLayers( 0 ),
	mLayerCount( 0 ),
	mViewSize( 800, 600 )
{
	ViewSize( mViewSize );
}

cMap::~cMap() {
	DeleteLayers();
}

void cMap::Reset() {
	DeleteLayers();

	mWindow = NULL;
	mLayers = NULL;
	mFlags	= 0;
	mMaxLayers	= 0;
	mViewSize = eeSize( 800, 600 );
}

void cMap::DeleteLayers() {
	for ( Uint32 i = 0; i < mLayerCount; i++ )
		eeSAFE_DELETE( mLayers[i] );

	eeSAFE_DELETE( mLayers );

	mLayerCount = 0;
}

void cMap::Create( eeSize Size, Uint32 MaxLayers, eeSize TileSize, Uint32 Flags, eeSize viewSize, cWindow * Window ) {
	Reset();

	mWindow		= Window;

	if ( NULL == mWindow )
		mWindow	= cEngine::instance()->GetCurrentWindow();

	mFlags		= Flags;
	mMaxLayers	= MaxLayers;
	mSize		= Size;
	mTileSize	= TileSize;
	mLayers		= eeNewArray( cLayer*, mMaxLayers );

	for ( Uint32 i = 0; i < mMaxLayers; i++ )
		mLayers[i] = NULL;

	ViewSize( viewSize );
}

void cMap::AddLayer( Uint32 Type, Uint32 flags, std::string name ) {
	eeASSERT( NULL != mLayers );

	switch ( Type ) {
		case MAP_LAYER_TILED:
			mLayers[ mLayerCount ] = eeNew( cTileLayer, ( this, mSize, flags, name ) );
			break;
		case MAP_LAYER_OBJECT:
			mLayers[ mLayerCount ] = eeNew( cObjectLayer, ( this, flags, name ) );
			break;
		default:
			return;
	}

	mLayerCount++;
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

cLayer* cMap::GetLayer( const std::string& name ) {
	return GetLayerByHash( MakeHash( name ) );
}

void cMap::Draw() {
	cGlobalBatchRenderer::instance()->Draw();

	if ( ClipedArea() ) {
		mWindow->ClipEnable( mScreenPos.x, mScreenPos.y, mViewSize.x, mViewSize.y );
	}

	eeVector2f offsetFixed = eeVector2f( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y ) + FixOffset();

	GetMouseOverTile();

	GridDraw();

	for ( Uint32 i = 0; i < mLayerCount; i++ ) {
		mLayers[i]->Draw( offsetFixed );
	}

	if ( ClipedArea() ) {
		mWindow->ClipDisable();
	}
}

void cMap::GridDraw() {
	if ( !DrawGrid() )
		return;

	cPrimitives P;

	P.SetColor( eeColorA( 0, 0, 0, 50 ) );
	P.DrawRectangle( mScreenPos.x, mScreenPos.y, mViewSize.x, mViewSize.y, 0.f, 1.f );
	P.SetColor( eeColorA( 255, 255, 255, 255 ) );

	if ( 0 == mSize.x || 0 == mSize.y )
		return;

	cGlobalBatchRenderer::instance()->Draw();
	eeVector2f offsetFixed = eeVector2f( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y ) + FixOffset();

	GLi->LoadIdentity();
	GLi->PushMatrix();
	GLi->Translatef( offsetFixed.x, offsetFixed.y, 0.0f );

	eeVector2i start = StartTile();
	eeVector2i end = EndTile();

	for ( Int32 x = start.x; x < end.x; x++ ) {
		for ( Int32 y = start.y; y < end.y; y++ ) {
			P.DrawRectangle( x * mTileSize.x, y * mTileSize.y, mTileSize.x, mTileSize.y, 0.f, 1.f, EE_DRAW_LINE );
		}
	}

	GLi->PopMatrix();
}

void cMap::GetMouseOverTile() {
	eeVector2i mouse = mWindow->GetInput()->GetMousePos();

	mMouseOverTile.x = ( mouse.x - mScreenPos.x - mOffset.x ) / mTileSize.Width();
	mMouseOverTile.y = ( mouse.y - mScreenPos.y - mOffset.y ) / mTileSize.Height();

	if ( mMouseOverTile.x < 0 )
		mMouseOverTile.x = 0;

	if ( mMouseOverTile.y < 0 )
		mMouseOverTile.y = 0;

	if ( mMouseOverTile.x >= mSize.Width() )
		mMouseOverTile.x = mSize.Width() - 1;

	if ( mMouseOverTile.y >= mSize.Height() )
		mMouseOverTile.y = mSize.Height() - 1;
}

void cMap::Update() {
	for ( Uint32 i = 0; i < mLayerCount; i++ )
		mLayers[i]->Update();
}

const eeSize& cMap::ViewSize() const {
	return mViewSize;
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

void cMap::Offset( const eeVector2f& offset ) {
	mOffset = offset;

	Clamp();

	CalcTilesClip();
}

void cMap::CalcTilesClip() {
	if ( mTileSize.x > 0 && mTileSize.y > 0 ) {
		eeVector2f ffoff( FixOffset() );
		eeVector2i foff( (Int32)ffoff.x, (Int32)ffoff.y );

		mStartTile.x	= -foff.x / mTileSize.x;
		mStartTile.y	= -foff.y / mTileSize.y;
		mEndTile.x		= mStartTile.x + eeRound( (eeFloat)mViewSize.x / (eeFloat)mTileSize.x ) + 1;
		mEndTile.y		= mStartTile.y + eeRound( (eeFloat)mViewSize.y / (eeFloat)mTileSize.y ) + 1;

		if ( mStartTile.x < 0 )
			mStartTile.x = 0;

		if ( mStartTile.y < 0 )
			mStartTile.y = 0;

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

	eeSize totSize( mTileSize * mSize );

	if ( -mOffset.x + mViewSize.x > totSize.x )
		mOffset.x = -( totSize.x - mViewSize.x );

	if ( -mOffset.y + mViewSize.y > totSize.y )
		mOffset.y = -( totSize.y - mViewSize.y );

	if ( totSize.x < mViewSize.x )
		mOffset.x = 0;

	if ( totSize.y < mViewSize.y )
		mOffset.y = 0;
}

void cMap::DrawGrid( const bool& draw ) {
	SetFlagValue( &mFlags, MAP_FLAG_DRAW_GRID, draw ? 1 : 0 );
}

Uint32 cMap::DrawGrid() const {
	return mFlags & MAP_FLAG_DRAW_GRID;
}

Uint32 cMap::ClipedArea() const {
	return mFlags & MAP_FLAG_CLIP_AREA;
}

Uint32 cMap::ClampBorders() const {
	return mFlags & MAP_FLAG_CLAMP_BODERS;
}

eeVector2f cMap::FixOffset() {
	return eeVector2f( (eeFloat)static_cast<Int32>( mOffset.x ), (eeFloat)static_cast<Int32>( mOffset.y ) );
}

void cMap::Move( const eeVector2f& offset )  {
	Move( offset.x, offset.y );
}

void cMap::Move( const eeFloat& offsetx, const eeFloat& offsety ) {
	Offset( eeVector2f( mOffset.x + offsetx, mOffset.y + offsety ) );
}

cGameObject * cMap::CreateGameObject( const Uint32& Type, const Uint32& Flags ) {
	switch ( Type ) {
		case GAMEOBJECT_TYPE_SHAPE:
			return eeNew( cGameObjectShape, ( Flags ) );
		case GAMEOBJECT_TYPE_SHAPEEX:
			return eeNew( cGameObjectShapeEx, ( Flags ) );
		case GAMEOBJECT_TYPE_SPRITE:
			return eeNew( cGameObjectSprite, ( Flags ) );
	}

	return NULL;
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

void cMap::Load( const std::string& path ) {

}

void cMap::Save( const std::string& path ) {

}

}}
