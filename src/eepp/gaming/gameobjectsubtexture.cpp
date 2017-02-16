#include <eepp/gaming/gameobjectsubtexture.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/tilemaplayer.hpp>
#include <eepp/gaming/maplightmanager.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace Gaming {

GameObjectSubTexture::GameObjectSubTexture( const Uint32& Flags, MapLayer * Layer, Graphics::SubTexture * SubTexture, const Vector2f& Pos ) :
	GameObject( Flags, Layer ),
	mSubTexture( SubTexture ),
	mPos( Pos )
{
	AssignTilePos();
}

GameObjectSubTexture::~GameObjectSubTexture() {
}

Uint32 GameObjectSubTexture::Type() const {
	return GAMEOBJECT_TYPE_SUBTEXTURE;
}

bool GameObjectSubTexture::IsType( const Uint32& type ) {
	return ( GameObjectSubTexture::Type() == type ) ? true : GameObject::IsType( type );
}

void GameObjectSubTexture::Draw() {
	if ( NULL != mSubTexture ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			MapLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSubTexture->draw(
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
					mSubTexture->draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mSubTexture->draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						Vector2f::One,
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->destSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->destSize().x, mPos.y + mSubTexture->destSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->destSize().y, mPos.y ) ),
						ALPHA_NORMAL,
						RenderModeFromFlags()
					);
				} else {
					mSubTexture->draw( mPos.x, mPos.y, LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
				}
			}
		} else {
			mSubTexture->draw( mPos.x, mPos.y, ColorA(), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
		}
	}
}

Vector2f GameObjectSubTexture::Pos() const {
	return mPos;
}

void GameObjectSubTexture::Pos( Vector2f pos ) {
	mPos = pos;
	GameObject::Pos( pos );
}

Vector2i GameObjectSubTexture::TilePos() const {
	return mTilePos;
}

void GameObjectSubTexture::TilePos( Vector2i pos ) {
	mTilePos = pos;
}

Sizei GameObjectSubTexture::Size() {
	if ( NULL != mSubTexture )
		return mSubTexture->realSize();

	return Sizei();
}

Graphics::SubTexture * GameObjectSubTexture::SubTexture() const {
	return mSubTexture;
}

void GameObjectSubTexture::SubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;
}

Uint32 GameObjectSubTexture::DataId() {
	return mSubTexture->getId();
}

void GameObjectSubTexture::DataId( Uint32 Id ) {
	SubTexture( TextureAtlasManager::instance()->getSubTextureById( Id ) );
}

}}
