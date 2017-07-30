#include <eepp/maps/gameobjectsubtexture.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/tilemaplayer.hpp>
#include <eepp/maps/maplightmanager.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace Maps {

GameObjectSubTexture::GameObjectSubTexture( const Uint32& Flags, MapLayer * Layer, Graphics::SubTexture * SubTexture, const Vector2f& Pos ) :
	GameObject( Flags, Layer ),
	mSubTexture( SubTexture ),
	mPos( Pos )
{
	assignTilePos();
}

GameObjectSubTexture::~GameObjectSubTexture() {
}

Uint32 GameObjectSubTexture::getType() const {
	return GAMEOBJECT_TYPE_SUBTEXTURE;
}

bool GameObjectSubTexture::isType( const Uint32& type ) {
	return ( GameObjectSubTexture::getType() == type ) ? true : GameObject::isType( type );
}

void GameObjectSubTexture::draw() {
	if ( NULL != mSubTexture ) {
		Sizef destSizeO = mSubTexture->getDestSize();
		Sizei realSize = mSubTexture->getRealSize();
		mSubTexture->setDestSize( Sizef( (Float)realSize.getWidth(), (Float)realSize.getHeight() ) );

		if ( mLayer->getMap()->getLightsEnabled() && mLayer->getLightsEnabled() ) {
			MapLightManager * LM = mLayer->getMap()->getLightManager();

			if ( MAP_LAYER_TILED == mLayer->getType() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*> ( mLayer )->getCurrentTile();

				if ( LM->isByVertex() ) {
					mSubTexture->draw(
						mPos.x,
						mPos.y,
						getRotation(),
						Vector2f::One,
						*LM->getTileColor( Tile, 0 ),
						*LM->getTileColor( Tile, 1 ),
						*LM->getTileColor( Tile, 2 ),
						*LM->getTileColor( Tile, 3 ),
						ALPHA_NORMAL,
						getRenderModeFromFlags()
					);
				} else {
					mSubTexture->draw( mPos.x, mPos.y, *LM->getTileColor( Tile ), getRotation(), Vector2f::One, ALPHA_NORMAL, getRenderModeFromFlags() );
				}
			} else {
				if ( LM->isByVertex() ) {
					mSubTexture->draw(
						mPos.x,
						mPos.y,
						getRotation(),
						Vector2f::One,
						LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->getColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->getDestSize().y ) ),
						LM->getColorFromPos( Vector2f( mPos.x + mSubTexture->getDestSize().x, mPos.y + mSubTexture->getDestSize().y ) ),
						LM->getColorFromPos( Vector2f( mPos.x + mSubTexture->getDestSize().y, mPos.y ) ),
						ALPHA_NORMAL,
						getRenderModeFromFlags()
					);
				} else {
					mSubTexture->draw( mPos.x, mPos.y, LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ), getRotation(), Vector2f::One, ALPHA_NORMAL, getRenderModeFromFlags() );
				}
			}
		} else {
			mSubTexture->draw( mPos.x, mPos.y, Color::White, getRotation(), Vector2f::One, ALPHA_NORMAL, getRenderModeFromFlags() );
		}

		mSubTexture->setDestSize( destSizeO );
	}
}

Vector2f GameObjectSubTexture::getPosition() const {
	return mPos;
}

void GameObjectSubTexture::setPosition( Vector2f pos ) {
	mPos = pos;
	GameObject::setPosition( pos );
}

Vector2i GameObjectSubTexture::getTilePosition() const {
	return mTilePos;
}

void GameObjectSubTexture::setTilePosition( Vector2i pos ) {
	mTilePos = pos;
}

Sizei GameObjectSubTexture::getSize() {
	if ( NULL != mSubTexture )
		return mSubTexture->getRealSize();

	return Sizei();
}

Graphics::SubTexture * GameObjectSubTexture::getSubTexture() const {
	return mSubTexture;
}

void GameObjectSubTexture::setSubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;
}

Uint32 GameObjectSubTexture::getDataId() {
	return mSubTexture->getId();
}

void GameObjectSubTexture::setDataId( Uint32 Id ) {
	setSubTexture( TextureAtlasManager::instance()->getSubTextureById( Id ) );
}

}}
