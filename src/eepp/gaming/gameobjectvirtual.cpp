#include <eepp/gaming/gameobjectvirtual.hpp>

#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/tilemaplayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

GameObjectVirtual::GameObjectVirtual( Uint32 DataId, MapLayer * Layer, const Uint32& Flags, Uint32 Type, const Vector2f& Pos ) :
	GameObject( Flags, Layer ),
	mType( Type ),
	mDataId( DataId ),
	mPos( Pos ),
	mLayer( NULL ),
	mSubTexture( NULL )
{
}

GameObjectVirtual::GameObjectVirtual( SubTexture * SubTexture, MapLayer * Layer, const Uint32& Flags, Uint32 Type, const Vector2f& Pos ) :
	GameObject( Flags, Layer ),
	mType( Type ),
	mDataId( 0 ),
	mPos( Pos ),
	mLayer( Layer ),
	mSubTexture( SubTexture )
{
	if ( NULL != SubTexture )
		mDataId = SubTexture->Id();
}

GameObjectVirtual::~GameObjectVirtual() {
}

Uint32 GameObjectVirtual::Type() const {
	return GAMEOBJECT_TYPE_VIRTUAL;
}

bool GameObjectVirtual::IsType( const Uint32& type ) {
	return ( GameObjectVirtual::Type() == type ) ? true : GameObject::IsType( type );
}

Uint32 GameObjectVirtual::RealType() const {
	return mType;
}

Sizei GameObjectVirtual::Size() {
	if ( NULL != mSubTexture )
		return mSubTexture->RealSize();

	if ( NULL != mLayer )
		return mLayer->Map()->TileSize();

	return Sizei( 32, 32 );
}

void GameObjectVirtual::Draw() {
	if ( NULL != mSubTexture ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			MapLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						Vector2f::One,
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 ),
						ALPHA_NORMAL,
						RenderModeFromFlags()
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						Vector2f::One,
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y ) ),
						ALPHA_NORMAL,
						RenderModeFromFlags()
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
				}
			}
		} else {
			mSubTexture->Draw( mPos.x, mPos.y, ColorA(), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
		}
	} else {
		Primitives P;

		ColorA C( mDataId );
		C.Alpha = 255;

		P.SetColor( C );

		if ( NULL != mLayer ) {
			Sizei ts = mLayer->Map()->TileSize();
			P.DrawRectangle( Rectf( Vector2f( mPos.x, mPos.y ), Sizef( ts.x ,ts.y ) ), 0, Vector2f::One );
		} else {
			P.DrawRectangle( Rectf( Vector2f( mPos.x, mPos.y ), Sizef( 32 ,32 ) ), 0, Vector2f::One );
		}
	}
}

Vector2f GameObjectVirtual::Pos() const {
	return mPos;
}

void GameObjectVirtual::Pos( Vector2f pos ) {
	mPos = pos;
}

Uint32 GameObjectVirtual::DataId() {
	return mDataId;
}

void GameObjectVirtual::DataId( Uint32 Id ) {
	mDataId = Id;
}

void GameObjectVirtual::SetLayer( MapLayer * Layer ) {
	mLayer = Layer;
}

}}
