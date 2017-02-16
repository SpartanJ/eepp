#include <eepp/gaming/gameobjectsprite.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/tilemaplayer.hpp>

namespace EE { namespace Gaming {

GameObjectSprite::GameObjectSprite( const Uint32& Flags, MapLayer * Layer, Graphics::Sprite * Sprite ) :
	GameObject( Flags, Layer ),
	mSprite( Sprite )
{
	if ( NULL != mSprite )
		mSprite->renderMode( RenderModeFromFlags() );

	AssignTilePos();
}

GameObjectSprite::~GameObjectSprite() {
	eeSAFE_DELETE( mSprite );
}

Uint32 GameObjectSprite::Type() const {
	return GAMEOBJECT_TYPE_SPRITE;
}

bool GameObjectSprite::IsType( const Uint32& type ) {
	return ( GameObjectSprite::Type() == type ) ? true : GameObject::IsType( type );
}

void GameObjectSprite::Draw() {
	if ( NULL != mSprite ) {
		mSprite->angle( GetAngle() );

		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			MapLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSprite->updateVertexColors(
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 )
					);
				} else {
					mSprite->color( *LM->GetTileColor( Tile ) );
				}
			} else {
				if ( LM->IsByVertex() ) {
					Quad2f Q = mSprite->getQuad();

					mSprite->updateVertexColors(
						LM->GetColorFromPos( Q.V[0] ),
						LM->GetColorFromPos( Q.V[1] ),
						LM->GetColorFromPos( Q.V[2] ),
						LM->GetColorFromPos( Q.V[3] )
					);
				} else {
					mSprite->color( LM->GetColorFromPos( mSprite->position() ) );
				}
			}
		}

		mSprite->draw();
	}
}

Vector2f GameObjectSprite::Pos() const {
	if ( NULL != mSprite )
		return mSprite->position();

	return Vector2f();
}

void GameObjectSprite::Pos( Vector2f pos ) {
	if ( NULL != mSprite ) {
		mSprite->position( pos );
		GameObject::Pos( pos );
	}
}

Vector2i GameObjectSprite::TilePos() const {
	return mTilePos;
}

void GameObjectSprite::TilePos( Vector2i pos ) {
	mTilePos = pos;
}

Sizei GameObjectSprite::Size() {
	if ( NULL != mSprite )
		return mSprite->getSubTexture(0)->realSize();

	return Sizei();
}

Graphics::Sprite * GameObjectSprite::Sprite() const {
	return mSprite;
}

void GameObjectSprite::Sprite( Graphics::Sprite * sprite ) {
	eeSAFE_DELETE( mSprite );
	mSprite = sprite;
	mSprite->renderMode( RenderModeFromFlags() );
}

void GameObjectSprite::FlagSet( const Uint32& Flag ) {
	if ( NULL != mSprite )
		mSprite->renderMode( RenderModeFromFlags() );

	GameObject::FlagSet( Flag );
}

Uint32 GameObjectSprite::DataId() {
	return mSprite->getSubTexture(0)->getId();
}

void GameObjectSprite::DataId( Uint32 Id ) {
	Graphics::Sprite * tSprite = NULL;

	if ( mFlags & GObjFlags::GAMEOBJECT_ANIMATED ) {
		std::vector<SubTexture*> tSubTextureVec = TextureAtlasManager::instance()->getSubTexturesByPatternId( Id );

		if ( tSubTextureVec.size() ) {
			tSprite = eeNew( Graphics::Sprite, () );
			tSprite->createAnimation();
			tSprite->addFrames( tSubTextureVec );

			Sprite( tSprite );
		}
	} else {
		Graphics::SubTexture * tSubTexture = TextureAtlasManager::instance()->getSubTextureById( Id );

		if ( NULL != tSubTexture ) {
			Sprite( eeNew( Graphics::Sprite, ( tSubTexture ) ) );
		}
	}
}

}}
