#include "cgameobjectshape.hpp"
#include "cmap.hpp"
#include "ctilelayer.hpp"
#include "clightmanager.hpp"
#include "../graphics/cshapegroupmanager.hpp"

namespace EE { namespace Gaming {

cGameObjectShape::cGameObjectShape( const Uint32& Flags, cLayer * Layer, cShape * Shape, const eeVector2f& Pos ) :
	cGameObject( Flags, Layer ),
	mShape( Shape ),
	mPos( Pos )
{
	AssignTilePos();
}

cGameObjectShape::~cGameObjectShape() {
}

Uint32 cGameObjectShape::Type() const {
	return GAMEOBJECT_TYPE_SHAPE;
}

void cGameObjectShape::Draw() {
	if ( NULL != mShape ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			cLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				eeVector2i Tile = reinterpret_cast<cTileLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mShape->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						1.f,
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 ),
						ALPHA_NORMAL,
						RenderTypeFromFlags()
					);
				} else {
					mShape->Draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), GetAngle(), 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mShape->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						1.f,
						LM->GetColorFromPos( eeVector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( eeVector2f( mPos.x, mPos.y + mShape->DestHeight() ) ),
						LM->GetColorFromPos( eeVector2f( mPos.x + mShape->DestWidth(), mPos.y + mShape->DestHeight() ) ),
						LM->GetColorFromPos( eeVector2f( mPos.x + mShape->DestWidth(), mPos.y ) ),
						ALPHA_NORMAL,
						RenderTypeFromFlags()
					);
				} else {
					mShape->Draw( mPos.x, mPos.y, LM->GetColorFromPos( eeVector2f( mPos.x, mPos.y ) ), GetAngle(), 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
				}
			}
		} else {
			mShape->Draw( mPos.x, mPos.y, eeColorA(), GetAngle(), 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
		}
	}
}

eeVector2f cGameObjectShape::Pos() const {
	return mPos;
}

void cGameObjectShape::Pos( eeVector2f pos ) {
	mPos = pos;
	cGameObject::Pos( pos );
}

eeVector2i cGameObjectShape::TilePos() const {
	return mTilePos;
}

void cGameObjectShape::TilePos( eeVector2i pos ) {
	mTilePos = pos;
}

eeSize cGameObjectShape::Size() {
	if ( NULL != mShape )
		return mShape->RealSize();

	return eeSize();
}

cShape * cGameObjectShape::Shape() const {
	return mShape;
}

void cGameObjectShape::Shape( cShape * shape ) {
	mShape = shape;
}

Uint32 cGameObjectShape::DataId() {
	return mShape->Id();
}

void cGameObjectShape::DataId( Uint32 Id ) {
	Shape( cShapeGroupManager::instance()->GetShapeById( Id ) );
}

}}
