#include <eepp/gaming/cgameobjectvirtual.hpp>

#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/clayer.hpp>
#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/graphics/primitives.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

cGameObjectVirtual::cGameObjectVirtual( Uint32 DataId, cLayer * Layer, const Uint32& Flags, Uint32 Type, const Vector2f& Pos ) :
	cGameObject( Flags, Layer ),
	mType( Type ),
	mDataId( DataId ),
	mPos( Pos ),
	mLayer( NULL ),
	mSubTexture( NULL )
{
}

cGameObjectVirtual::cGameObjectVirtual( SubTexture * SubTexture, cLayer * Layer, const Uint32& Flags, Uint32 Type, const Vector2f& Pos ) :
	cGameObject( Flags, Layer ),
	mType( Type ),
	mDataId( 0 ),
	mPos( Pos ),
	mLayer( Layer ),
	mSubTexture( SubTexture )
{
	if ( NULL != SubTexture )
		mDataId = SubTexture->Id();
}

cGameObjectVirtual::~cGameObjectVirtual() {
}

Uint32 cGameObjectVirtual::Type() const {
	return GAMEOBJECT_TYPE_VIRTUAL;
}

bool cGameObjectVirtual::IsType( const Uint32& type ) {
	return ( cGameObjectVirtual::Type() == type ) ? true : cGameObject::IsType( type );
}

Uint32 cGameObjectVirtual::RealType() const {
	return mType;
}

Sizei cGameObjectVirtual::Size() {
	if ( NULL != mSubTexture )
		return mSubTexture->RealSize();

	if ( NULL != mLayer )
		return mLayer->Map()->TileSize();

	return Sizei( 32, 32 );
}

void cGameObjectVirtual::Draw() {
	if ( NULL != mSubTexture ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			cLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				Vector2i Tile = reinterpret_cast<cTileLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						Vector2f::One,
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 ),
						ALPHA_NORMAL,
						RenderModeFromFlags()
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						Vector2f::One,
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y ) ),
						ALPHA_NORMAL,
						RenderModeFromFlags()
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
				}
			}
		} else {
			mSubTexture->Draw( mPos.x, mPos.y, ColorA(), GetAngle(), Vector2f::One, ALPHA_NORMAL, RenderModeFromFlags() );
		}
	} else {
		Primitives P;

		ColorA C( mDataId );
		C.Alpha = 255;

		P.SetColor( C );

		if ( NULL != mLayer ) {
			Sizei ts = mLayer->Map()->TileSize();
			P.DrawRectangle( Rectf( Vector2f( mPos.x, mPos.y ), Sizef( ts.x ,ts.y ) ), 0, Vector2f::One );
		} else {
			P.DrawRectangle( Rectf( Vector2f( mPos.x, mPos.y ), Sizef( 32 ,32 ) ), 0, Vector2f::One );
		}
	}
}

Vector2f cGameObjectVirtual::Pos() const {
	return mPos;
}

void cGameObjectVirtual::Pos( Vector2f pos ) {
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
