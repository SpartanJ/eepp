#include "cgameobjectvirtual.hpp"

#include "cmap.hpp"
#include "clayer.hpp"
#include "ctilelayer.hpp"
#include "../graphics/cprimitives.hpp"
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cGameObjectVirtual::cGameObjectVirtual( Uint32 DataId, cLayer * Layer, const Uint32& Flags, Uint32 Type, const eeVector2f& Pos ) :
	cGameObject( Flags, Layer ),
	mType( Type ),
	mDataId( DataId ),
	mPos( Pos ),
	mLayer( NULL ),
	mShape( NULL )
{
}

cGameObjectVirtual::cGameObjectVirtual( cShape * Shape, cLayer * Layer, const Uint32& Flags, Uint32 Type, const eeVector2f& Pos ) :
	cGameObject( Flags, Layer ),
	mType( Type ),
	mDataId( 0 ),
	mPos( Pos ),
	mLayer( Layer ),
	mShape( Shape )
{
	if ( NULL != Shape )
		mDataId = Shape->Id();
}

cGameObjectVirtual::~cGameObjectVirtual() {
}

Uint32 cGameObjectVirtual::Type() const {
	return GAMEOBJECT_TYPE_VIRTUAL;
}

bool cGameObjectVirtual::IsType( const Uint32& type ) {
	return ( Type() == type ) ? true : cGameObject::IsType( type );
}

Uint32 cGameObjectVirtual::RealType() const {
	return mType;
}

eeSize cGameObjectVirtual::Size() {
	if ( NULL != mShape )
		return mShape->RealSize();

	if ( NULL != mLayer )
		return mLayer->Map()->TileSize();

	return eeSize( 32, 32 );
}

void cGameObjectVirtual::Draw() {
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
	} else {
		cPrimitives P;

		eeColorA C( mDataId );
		C.Alpha = 255;

		P.SetColor( C );

		if ( NULL != mLayer ) {
			eeSize ts = mLayer->Map()->TileSize();
			P.DrawRectangle( mPos.x, mPos.y, ts.x ,ts.y, 0, 1 );
		} else {
			P.DrawRectangle( mPos.x, mPos.y, 32 ,32, 0, 1 );
		}
	}
}

eeVector2f cGameObjectVirtual::Pos() const {
	return mPos;
}

void cGameObjectVirtual::Pos( eeVector2f pos ) {
	mPos = pos;
}

Uint32 cGameObjectVirtual::DataId() {
	return mDataId;
}

void cGameObjectVirtual::DataId( Uint32 Id ) {
	mDataId = Id;
}

void cGameObjectVirtual::SetLayer( cLayer * Layer ) {
	mLayer = Layer;
}

}}
