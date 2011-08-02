#include "cgameobjectshapeex.hpp"
#include "cmap.hpp"
#include "ctilelayer.hpp"

namespace EE { namespace Gaming {

cGameObjectShapeEx::cGameObjectShapeEx( const Uint32& Flags, cLayer * Layer, cShape * Shape, const eeVector2f& Pos, EE_PRE_BLEND_FUNC Blend, EE_RENDERTYPE Render, eeFloat Angle, eeFloat Scale, eeColorA Color ) :
	cGameObjectShape( Flags, Layer, Shape, Pos ),
	mBlend( Blend ),
	mRender( Render ),
	mAngle( Angle ),
	mScale( Scale ),
	mColor( Color ),
	mVertexColors( NULL )
{
	mRender = RenderTypeFromFlags();

	if ( 0 == mAngle )
		mAngle = GetAngle();
}

cGameObjectShapeEx::~cGameObjectShapeEx()
{
}

Uint32 cGameObjectShapeEx::Type() const {
	return GAMEOBJECT_TYPE_SHAPEEX;
}

void cGameObjectShapeEx::Draw() {
	if ( NULL != mShape ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			cLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				eeVector2i Tile = reinterpret_cast<cTileLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mShape->Draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						*mLayer->Map()->GetLightManager()->GetTileColor( Tile, 0 ),
						*mLayer->Map()->GetLightManager()->GetTileColor( Tile, 1 ),
						*mLayer->Map()->GetLightManager()->GetTileColor( Tile, 2 ),
						*mLayer->Map()->GetLightManager()->GetTileColor( Tile, 3 ),
						mBlend,
						mRender
					);
				} else {
					mShape->Draw( mPos.x, mPos.y, *mLayer->Map()->GetLightManager()->GetTileColor( Tile ), mAngle, mScale, mBlend, mRender );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mShape->Draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						mLayer->Map()->GetLightManager()->GetColorFromPos( eeVector2f( mPos.x, mPos.y ) ),
						mLayer->Map()->GetLightManager()->GetColorFromPos( eeVector2f( mPos.x, mPos.y + mShape->DestHeight() ) ),
						mLayer->Map()->GetLightManager()->GetColorFromPos( eeVector2f( mPos.x + mShape->DestWidth(), mPos.y + mShape->DestHeight() ) ),
						mLayer->Map()->GetLightManager()->GetColorFromPos( eeVector2f( mPos.x + mShape->DestWidth(), mPos.y ) ),
						mBlend,
						mRender
					);
				} else {
					mShape->Draw( mPos.x, mPos.y, mLayer->Map()->GetLightManager()->GetColorFromPos( eeVector2f( mPos.x, mPos.y ) ), mAngle, mScale, mBlend, mRender );
				}
			}
		} else {
			if ( NULL != mVertexColors ) {
				mShape->Draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[4], mBlend, mRender );
			} else {
				mShape->Draw( mPos.x, mPos.y, mColor, mAngle, mScale, mBlend, mRender );
			}
		}
	}
}

void cGameObjectShapeEx::FlagSet( const Uint32& Flag ) {
	mRender = RenderTypeFromFlags();

	cGameObject::FlagSet( Flag );
}

}}
