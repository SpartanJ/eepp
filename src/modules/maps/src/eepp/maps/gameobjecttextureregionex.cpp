#include <eepp/maps/gameobjecttextureregionex.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/tilemaplayer.hpp>

namespace EE { namespace Maps {

GameObjectTextureRegionEx::GameObjectTextureRegionEx( const Uint32& Flags, MapLayer* Layer,
													  Graphics::TextureRegion* TextureRegion,
													  const Vector2f& Pos, BlendMode Blend,
													  RenderMode Render, Float Angle,
													  Vector2f Scale, Color Color ) :
	GameObjectTextureRegion( Flags, Layer, TextureRegion, Pos ),
	mBlend( Blend ),
	mRender( Render ),
	mAngle( Angle ),
	mScale( Scale ),
	mColor( Color ),
	mVertexColors( NULL ) {
	mRender = getRenderModeFromFlags();
	mBlend = getBlendModeFromFlags();

	if ( 0 == mAngle )
		mAngle = getRotation();
}

GameObjectTextureRegionEx::~GameObjectTextureRegionEx() {}

Uint32 GameObjectTextureRegionEx::getType() const {
	return GAMEOBJECT_TYPE_TEXTUREREGIONEX;
}

bool GameObjectTextureRegionEx::isType( const Uint32& type ) {
	return ( GameObjectTextureRegionEx::getType() == type )
			   ? true
			   : GameObjectTextureRegion::isType( type );
}

void GameObjectTextureRegionEx::draw() {
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
						mPos.x, mPos.y, mAngle, mScale, *LM->getTileColor( Tile, 0 ),
						*LM->getTileColor( Tile, 1 ), *LM->getTileColor( Tile, 2 ),
						*LM->getTileColor( Tile, 3 ), mBlend, mRender );
				} else {
					mTextureRegion->draw( mPos.x, mPos.y, *LM->getTileColor( Tile ), mAngle, mScale,
										  mBlend, mRender );
				}
			} else {
				if ( LM->isByVertex() ) {
					mTextureRegion->draw(
						mPos.x, mPos.y, mAngle, mScale,
						LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->getColorFromPos(
							Vector2f( mPos.x, mPos.y + mTextureRegion->getDestSize().y ) ),
						LM->getColorFromPos( Vector2f( mPos.x + mTextureRegion->getDestSize().x,
													   mPos.y + mTextureRegion->getDestSize().y ) ),
						LM->getColorFromPos(
							Vector2f( mPos.x + mTextureRegion->getDestSize().x, mPos.y ) ),
						mBlend, mRender );
				} else {
					mTextureRegion->draw( mPos.x, mPos.y,
										  LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ), mAngle,
										  mScale, mBlend, mRender );
				}
			}
		} else {
			if ( NULL != mVertexColors ) {
				mTextureRegion->draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0],
									  mVertexColors[1], mVertexColors[2], mVertexColors[4], mBlend,
									  mRender );
			} else {
				mTextureRegion->draw( mPos.x, mPos.y, mColor, mAngle, mScale, mBlend, mRender );
			}
		}

		mTextureRegion->setDestSize( destSizeO );
	}
}

void GameObjectTextureRegionEx::setFlag( const Uint32& Flag ) {
	mRender = getRenderModeFromFlags();
	mBlend = getBlendModeFromFlags();
	GameObject::setFlag( Flag );
}

}} // namespace EE::Maps
