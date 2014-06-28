#include <eepp/gaming/gameobjectsubtextureex.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/tilemaplayer.hpp>

namespace EE { namespace Gaming {

GameObjectSubTextureEx::GameObjectSubTextureEx( const Uint32& Flags, MapLayer * Layer, Graphics::SubTexture * SubTexture, const Vector2f& Pos, EE_BLEND_MODE Blend, EE_RENDER_MODE Render, Float Angle, Vector2f Scale, ColorA Color ) :
	GameObjectSubTexture( Flags, Layer, SubTexture, Pos ),
	mBlend( Blend ),
	mRender( Render ),
	mAngle( Angle ),
	mScale( Scale ),
	mColor( Color ),
	mVertexColors( NULL )
{
	mRender = RenderModeFromFlags();

	if ( 0 == mAngle )
		mAngle = GetAngle();
}

GameObjectSubTextureEx::~GameObjectSubTextureEx()
{
}

Uint32 GameObjectSubTextureEx::Type() const {
	return GAMEOBJECT_TYPE_SUBTEXTUREEX;
}

bool GameObjectSubTextureEx::IsType( const Uint32& type ) {
	return ( GameObjectSubTextureEx::Type() == type ) ? true : GameObjectSubTexture::IsType( type );
}

void GameObjectSubTextureEx::Draw() {
	if ( NULL != mSubTexture ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			MapLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 ),
						mBlend,
						mRender
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), mAngle, mScale, mBlend, mRender );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y ) ),
						mBlend,
						mRender
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ), mAngle, mScale, mBlend, mRender );
				}
			}
		} else {
			if ( NULL != mVertexColors ) {
				mSubTexture->Draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[4], mBlend, mRender );
			} else {
				mSubTexture->Draw( mPos.x, mPos.y, mColor, mAngle, mScale, mBlend, mRender );
			}
		}
	}
}

void GameObjectSubTextureEx::FlagSet( const Uint32& Flag ) {
	mRender = RenderModeFromFlags();

	GameObject::FlagSet( Flag );
}

}}
