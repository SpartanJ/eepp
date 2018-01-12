#include <eepp/maps/gameobjectsprite.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/tilemaplayer.hpp>

namespace EE { namespace Maps {

GameObjectSprite::GameObjectSprite( const Uint32& Flags, MapLayer * Layer, Graphics::Sprite * Sprite ) :
	GameObject( Flags, Layer ),
	mSprite( Sprite )
{
	if ( NULL != mSprite ) {
		mSprite->setRenderMode( getRenderModeFromFlags() );
		mSprite->setBlendMode( getBlendModeFromFlags() );
	}

	assignTilePos();
}

GameObjectSprite::~GameObjectSprite() {
	eeSAFE_DELETE( mSprite );
}

Uint32 GameObjectSprite::getType() const {
	return GAMEOBJECT_TYPE_SPRITE;
}

bool GameObjectSprite::isType( const Uint32& type ) {
	return ( GameObjectSprite::getType() == type ) ? true : GameObject::isType( type );
}

void GameObjectSprite::draw() {
	if ( NULL != mSprite ) {
		TextureRegion * TextureRegion = mSprite->getCurrentTextureRegion();
		Sizef destSizeO = TextureRegion->getDestSize();
		Sizei realSize = TextureRegion->getRealSize();
		TextureRegion->setDestSize( Sizef( (Float)realSize.getWidth(), (Float)realSize.getHeight() ) );

		mSprite->setRotation( getRotation() );

		if ( mLayer->getMap()->getLightsEnabled() && mLayer->getLightsEnabled() ) {
			MapLightManager * LM = mLayer->getMap()->getLightManager();

			if ( MAP_LAYER_TILED == mLayer->getType() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*> ( mLayer )->getCurrentTile();

				if ( LM->isByVertex() ) {
					mSprite->updateVertexColors(
						*LM->getTileColor( Tile, 0 ),
						*LM->getTileColor( Tile, 1 ),
						*LM->getTileColor( Tile, 2 ),
						*LM->getTileColor( Tile, 3 )
					);
				} else {
					mSprite->setColor( *LM->getTileColor( Tile ) );
				}
			} else {
				if ( LM->isByVertex() ) {
					Quad2f Q = mSprite->getQuad();

					mSprite->updateVertexColors(
						LM->getColorFromPos( Q.V[0] ),
						LM->getColorFromPos( Q.V[1] ),
						LM->getColorFromPos( Q.V[2] ),
						LM->getColorFromPos( Q.V[3] )
					);
				} else {
					mSprite->setColor( LM->getColorFromPos( mSprite->getPosition() ) );
				}
			}
		}

		mSprite->draw();

		TextureRegion->setDestSize( destSizeO );
	}
}

void GameObjectSprite::update(const Time & dt) {
	if ( NULL != mSprite )
		mSprite->update( dt );
}

Vector2f GameObjectSprite::getPosition() const {
	if ( NULL != mSprite )
		return mSprite->getPosition();

	return Vector2f();
}

void GameObjectSprite::setPosition( Vector2f pos ) {
	if ( NULL != mSprite ) {
		mSprite->setPosition( pos );
		GameObject::setPosition( pos );
	}
}

Vector2i GameObjectSprite::getTilePosition() const {
	return mTilePos;
}

void GameObjectSprite::setTilePosition( Vector2i pos ) {
	mTilePos = pos;
}

Sizei GameObjectSprite::getSize() {
	if ( NULL != mSprite )
		return mSprite->getTextureRegion(0)->getRealSize();

	return Sizei();
}

Graphics::Sprite * GameObjectSprite::getSprite() const {
	return mSprite;
}

void GameObjectSprite::setSprite( Graphics::Sprite * sprite ) {
	eeSAFE_DELETE( mSprite );
	mSprite = sprite;
	mSprite->setRenderMode( getRenderModeFromFlags() );
	mSprite->setBlendMode( getBlendModeFromFlags() );
	mSprite->setAutoAnimate( false );
}

void GameObjectSprite::setFlag( const Uint32& Flag ) {
	if ( NULL != mSprite ) {
		mSprite->setRenderMode( getRenderModeFromFlags() );
		mSprite->setBlendMode( getBlendModeFromFlags() );
	}
	GameObject::setFlag( Flag );
}

Uint32 GameObjectSprite::getDataId() {
	return mSprite->getTextureRegion(0)->getId();
}

void GameObjectSprite::setDataId( Uint32 Id ) {
	Graphics::Sprite * tSprite = NULL;

	if ( mFlags & GObjFlags::GAMEOBJECT_ANIMATED ) {
		std::vector<TextureRegion*> tTextureRegionVec = TextureAtlasManager::instance()->getTextureRegionsByPatternId( Id );

		if ( tTextureRegionVec.size() ) {
			tSprite = eeNew( Graphics::Sprite, () );
			tSprite->createAnimation();
			tSprite->addFrames( tTextureRegionVec );

			setSprite( tSprite );
		}
	} else {
		Graphics::TextureRegion * tTextureRegion = TextureAtlasManager::instance()->getTextureRegionById( Id );

		if ( NULL != tTextureRegion ) {
			setSprite( eeNew( Graphics::Sprite, ( tTextureRegion ) ) );
		}
	}
}

}}
