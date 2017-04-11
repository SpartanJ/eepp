#include <eepp/maps/gameobjectsubtextureex.hpp>
#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/tilemaplayer.hpp>

namespace EE { namespace Maps {

GameObjectSubTextureEx::GameObjectSubTextureEx( const Uint32& Flags, MapLayer * Layer, Graphics::SubTexture * SubTexture, const Vector2f& Pos, EE_BLEND_MODE Blend, EE_RENDER_MODE Render, Float Angle, Vector2f Scale, Color Color ) :
	GameObjectSubTexture( Flags, Layer, SubTexture, Pos ),
	mBlend( Blend ),
	mRender( Render ),
	mAngle( Angle ),
	mScale( Scale ),
	mColor( Color ),
	mVertexColors( NULL )
{
	mRender = getRenderModeFromFlags();

	if ( 0 == mAngle )
		mAngle = getRotation();
}

GameObjectSubTextureEx::~GameObjectSubTextureEx()
{
}

Uint32 GameObjectSubTextureEx::getType() const {
	return GAMEOBJECT_TYPE_SUBTEXTUREEX;
}

bool GameObjectSubTextureEx::isType( const Uint32& type ) {
	return ( GameObjectSubTextureEx::getType() == type ) ? true : GameObjectSubTexture::isType( type );
}

void GameObjectSubTextureEx::draw() {
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
						mAngle,
						mScale,
						*LM->getTileColor( Tile, 0 ),
						*LM->getTileColor( Tile, 1 ),
						*LM->getTileColor( Tile, 2 ),
						*LM->getTileColor( Tile, 3 ),
						mBlend,
						mRender
					);
				} else {
					mSubTexture->draw( mPos.x, mPos.y, *LM->getTileColor( Tile ), mAngle, mScale, mBlend, mRender );
				}
			} else {
				if ( LM->isByVertex() ) {
					mSubTexture->draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->getColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->getDestSize().y ) ),
						LM->getColorFromPos( Vector2f( mPos.x + mSubTexture->getDestSize().x, mPos.y + mSubTexture->getDestSize().y ) ),
						LM->getColorFromPos( Vector2f( mPos.x + mSubTexture->getDestSize().x, mPos.y ) ),
						mBlend,
						mRender
					);
				} else {
					mSubTexture->draw( mPos.x, mPos.y, LM->getColorFromPos( Vector2f( mPos.x, mPos.y ) ), mAngle, mScale, mBlend, mRender );
				}
			}
		} else {
			if ( NULL != mVertexColors ) {
				mSubTexture->draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[4], mBlend, mRender );
			} else {
				mSubTexture->draw( mPos.x, mPos.y, mColor, mAngle, mScale, mBlend, mRender );
			}
		}

		mSubTexture->setDestSize( destSizeO );
	}
}

void GameObjectSubTextureEx::setFlag( const Uint32& Flag ) {
	mRender = getRenderModeFromFlags();

	GameObject::setFlag( Flag );
}

}}
