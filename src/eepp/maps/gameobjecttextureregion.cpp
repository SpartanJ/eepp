#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/maps/gameobjecttextureregion.hpp>
#include <eepp/maps/maplightmanager.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/tilemaplayer.hpp>

namespace EE { namespace Maps {

GameObjectTextureRegion::GameObjectTextureRegion( const Uint32& Flags, MapLayer* Layer,
												  Graphics::TextureRegion* TextureRegion,
												  const Vector2f& Pos ) :
	GameObject( Flags, Layer ), mTextureRegion( TextureRegion ), mPos( Pos ) {
	assignTilePos();
}

GameObjectTextureRegion::~GameObjectTextureRegion() {}

Uint32 GameObjectTextureRegion::getType() const {
	return GAMEOBJECT_TYPE_TEXTUREREGION;
}

bool GameObjectTextureRegion::isType( const Uint32& type ) {
	return ( GameObjectTextureRegion::getType() == type ) ? true : GameObject::isType( type );
}

void GameObjectTextureRegion::draw() {
	if ( NULL != mTextureRegion ) {
		Sizef destSizeO = mTextureRegion->getDestSize();
		Sizei realSize = mTextureRegion->getRealSize();
		mTextureRegion->setDestSize(
			Sizef( (Float)realSize.getWidth(), (Float)realSize.getHeight() ) );

		if ( mLayer->getMap()->getLightsEnabled() && mLayer->getLightsEnabled() ) {
			MapLightManager* LM = mLayer->getMap()->getLightManager();

			if ( MAP_LAYER_TILED == mLayer->getType() ) {
				Vector2i Tile = reinterpret_cast<TileMapLayer*>( mLayer )->getCurrentTile();

				if ( LM->isByVertex() ) {
					mTextureRegion->draw(
						mPos.x, mPos.y, getRotation(), Vector2f::One, *LM->getTileColor( Tile, 0 ),
						*LM->getTileColor( Tile, 1 ), *LM->getTileColor( Tile, 2 ),
						*LM->getTileColor( Tile, 3 ), getBlendModeFromFlags(),
						getRenderModeFromFlags() );
				} else {
					mTextureRegion->draw( mPos.x, mPos.y, *LM->getTileColor( Tile ), getRotation(),
										  Vector2f::One, BlendAlpha, getRenderModeFromFlags() );
				}
			} else {
				if ( LM->isByVertex() ) {
					mTextureRegion->draw(
						mPos.x, mPos.y, getRotation(), Vector2f::One,
						LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->getColorFromPos(
							Vector2f( mPos.x, mPos.y + mTextureRegion->getDestSize().y ) ),
						LM->getColorFromPos( Vector2f( mPos.x + mTextureRegion->getDestSize().x,
													   mPos.y + mTextureRegion->getDestSize().y ) ),
						LM->getColorFromPos(
							Vector2f( mPos.x + mTextureRegion->getDestSize().y, mPos.y ) ),
						getBlendModeFromFlags(), getRenderModeFromFlags() );
				} else {
					mTextureRegion->draw( mPos.x, mPos.y,
										  LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ),
										  getRotation(), Vector2f::One, getBlendModeFromFlags(),
										  getRenderModeFromFlags() );
				}
			}
		} else {
			mTextureRegion->draw( mPos.x, mPos.y, Color::White, getRotation(), Vector2f::One,
								  getBlendModeFromFlags(), getRenderModeFromFlags() );
		}

		mTextureRegion->setDestSize( destSizeO );
	}
}

Vector2f GameObjectTextureRegion::getPosition() const {
	return mPos;
}

void GameObjectTextureRegion::setPosition( Vector2f pos ) {
	mPos = pos;
	GameObject::setPosition( pos );
}

Vector2i GameObjectTextureRegion::getTilePosition() const {
	return mTilePos;
}

void GameObjectTextureRegion::setTilePosition( Vector2i pos ) {
	mTilePos = pos;
}

Sizei GameObjectTextureRegion::getSize() {
	if ( NULL != mTextureRegion )
		return mTextureRegion->getRealSize();

	return Sizei();
}

Graphics::TextureRegion* GameObjectTextureRegion::getTextureRegion() const {
	return mTextureRegion;
}

void GameObjectTextureRegion::setTextureRegion( Graphics::TextureRegion* TextureRegion ) {
	mTextureRegion = TextureRegion;
}

Uint32 GameObjectTextureRegion::getDataId() {
	return mTextureRegion->getId();
}

void GameObjectTextureRegion::setDataId( Uint32 Id ) {
	setTextureRegion( TextureAtlasManager::instance()->getTextureRegionById( Id ) );
}

}} // namespace EE::Maps
