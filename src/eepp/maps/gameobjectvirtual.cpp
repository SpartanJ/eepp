#include <eepp/maps/gameobjectvirtual.hpp>

#include <eepp/maps/tilemap.hpp>
#include <eepp/maps/maplayer.hpp>
#include <eepp/maps/tilemaplayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Maps {

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
		mDataId = SubTexture->getId();
}

GameObjectVirtual::~GameObjectVirtual() {
}

Uint32 GameObjectVirtual::getType() const {
	return GAMEOBJECT_TYPE_VIRTUAL;
}

bool GameObjectVirtual::isType( const Uint32& type ) {
	return ( GameObjectVirtual::getType() == type ) ? true : GameObject::isType( type );
}

Uint32 GameObjectVirtual::getRealType() const {
	return mType;
}

Sizei GameObjectVirtual::getSize() {
	if ( NULL != mSubTexture )
		return Sizei( (Int32)mSubTexture->getDestSize().x, (Int32)mSubTexture->getDestSize().y );

	if ( NULL != mLayer )
		return mLayer->getMap()->getTileSize();

	return Sizei( 32, 32 );
}

void GameObjectVirtual::draw() {
	if ( NULL != mSubTexture ) {
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
						LM->getColorFromPos( Vector2f( mPos.x + mSubTexture->getDestSize().x, mPos.y ) ),
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
	} else {
		Primitives P;

		Color C( mDataId );
		C.a = 255;

		P.setColor( C );

		if ( NULL != mLayer ) {
			Sizei ts = mLayer->getMap()->getTileSize();
			P.drawRectangle( Rectf( Vector2f( mPos.x, mPos.y ), Sizef( ts.x ,ts.y ) ), 0, Vector2f::One );
		} else {
			P.drawRectangle( Rectf( Vector2f( mPos.x, mPos.y ), Sizef( 32 ,32 ) ), 0, Vector2f::One );
		}
	}
}

Vector2f GameObjectVirtual::getPosition() const {
	return mPos;
}

void GameObjectVirtual::setPosition( Vector2f pos ) {
	mPos = pos;
}

Uint32 GameObjectVirtual::getDataId() {
	return mDataId;
}

void GameObjectVirtual::setDataId( Uint32 Id ) {
	mDataId = Id;
}

void GameObjectVirtual::setLayer( MapLayer * Layer ) {
	mLayer = Layer;
}

}}
