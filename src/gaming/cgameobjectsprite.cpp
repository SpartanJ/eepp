#include "cgameobjectsprite.hpp"
#include "../graphics/cshapegroupmanager.hpp"
#include "cmap.hpp"
#include "ctilelayer.hpp"

namespace EE { namespace Gaming {

cGameObjectSprite::cGameObjectSprite( const Uint32& Flags, cLayer * Layer, cSprite * Sprite ) :
	cGameObject( Flags, Layer ),
	mSprite( Sprite )
{
	if ( NULL != mSprite )
		mSprite->RenderType( RenderTypeFromFlags() );

	AssignTilePos();
}

cGameObjectSprite::~cGameObjectSprite() {
	eeSAFE_DELETE( mSprite );
}

Uint32 cGameObjectSprite::Type() const {
	return GAMEOBJECT_TYPE_SPRITE;
}

bool cGameObjectSprite::IsType( const Uint32& type ) {
	return ( cGameObjectSprite::Type() == type ) ? true : cGameObject::IsType( type );
}

void cGameObjectSprite::Draw() {
	if ( NULL != mSprite ) {
		mSprite->Angle( GetAngle() );

		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			cLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				eeVector2i Tile = reinterpret_cast<cTileLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSprite->UpdateVertexColors(
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 )
					);
				} else {
					mSprite->Color( *LM->GetTileColor( Tile ) );
				}
			} else {
				if ( LM->IsByVertex() ) {
					eeQuad2f Q = mSprite->GetQuad();

					mSprite->UpdateVertexColors(
						LM->GetColorFromPos( Q.V[0] ),
						LM->GetColorFromPos( Q.V[1] ),
						LM->GetColorFromPos( Q.V[2] ),
						LM->GetColorFromPos( Q.V[3] )
					);
				} else {
					mSprite->Color( LM->GetColorFromPos( mSprite->Position() ) );
				}
			}
		}

		mSprite->Draw();
	}
}

eeVector2f cGameObjectSprite::Pos() const {
	if ( NULL != mSprite )
		return mSprite->Position();

	return eeVector2f();
}

void cGameObjectSprite::Pos( eeVector2f pos ) {
	if ( NULL != mSprite ) {
		mSprite->Position( pos );
		cGameObject::Pos( pos );
	}
}

eeVector2i cGameObjectSprite::TilePos() const {
	return mTilePos;
}

void cGameObjectSprite::TilePos( eeVector2i pos ) {
	mTilePos = pos;
}

eeSize cGameObjectSprite::Size() {
	if ( NULL != mSprite )
		return mSprite->GetShape(0)->RealSize();

	return eeSize();
}

cSprite * cGameObjectSprite::Sprite() const {
	return mSprite;
}

void cGameObjectSprite::Sprite( cSprite * sprite ) {
	eeSAFE_DELETE( mSprite );
	mSprite = sprite;
	mSprite->RenderType( RenderTypeFromFlags() );
}

void cGameObjectSprite::FlagSet( const Uint32& Flag ) {
	if ( NULL != mSprite )
		mSprite->RenderType( RenderTypeFromFlags() );

	cGameObject::FlagSet( Flag );
}

Uint32 cGameObjectSprite::DataId() {
	return mSprite->GetShape(0)->Id();
}

void cGameObjectSprite::DataId( Uint32 Id ) {
	cSprite * tSprite = NULL;

	if ( mFlags & GObjFlags::GAMEOBJECT_ANIMATED ) {
		std::vector<cShape*> tShapeVec = cShapeGroupManager::instance()->GetShapesByPatternId( Id );

		if ( tShapeVec.size() ) {
			tSprite = eeNew( cSprite, () );
			tSprite->CreateAnimation();
			tSprite->AddFrames( tShapeVec );

			Sprite( tSprite );
		}
	} else {
		cShape * tShape = cShapeGroupManager::instance()->GetShapeById( Id );

		if ( NULL != tShape ) {
			Sprite( eeNew( cSprite, ( tShape ) ) );
		}
	}
}

}}
