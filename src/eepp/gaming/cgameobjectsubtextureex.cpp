#include <eepp/gaming/cgameobjectsubtextureex.hpp>
#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/ctilelayer.hpp>

namespace EE { namespace Gaming {

cGameObjectSubTextureEx::cGameObjectSubTextureEx( const Uint32& Flags, cLayer * Layer, cSubTexture * SubTexture, const Vector2f& Pos, EE_BLEND_MODE Blend, EE_RENDER_MODE Render, Float Angle, Vector2f Scale, ColorA Color ) :
	cGameObjectSubTexture( Flags, Layer, SubTexture, Pos ),
	mBlend( Blend ),
	mRender( Render ),
	mAngle( Angle ),
	mScale( Scale ),
	mColor( Color ),
	mVertexColors( NULL )
{
	mRender = RenderModeFromFlags();

	if ( 0 == mAngle )
		mAngle = GetAngle();
}

cGameObjectSubTextureEx::~cGameObjectSubTextureEx()
{
}

Uint32 cGameObjectSubTextureEx::Type() const {
	return GAMEOBJECT_TYPE_SUBTEXTUREEX;
}

bool cGameObjectSubTextureEx::IsType( const Uint32& type ) {
	return ( cGameObjectSubTextureEx::Type() == type ) ? true : cGameObjectSubTexture::IsType( type );
}

void cGameObjectSubTextureEx::Draw() {
	if ( NULL != mSubTexture ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			cLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				Vector2i Tile = reinterpret_cast<cTileLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						*LM->GetTileColor( Tile, 0 ),
						*LM->GetTileColor( Tile, 1 ),
						*LM->GetTileColor( Tile, 2 ),
						*LM->GetTileColor( Tile, 3 ),
						mBlend,
						mRender
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), mAngle, mScale, mBlend, mRender );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						mAngle,
						mScale,
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y + mSubTexture->DestSize().y ) ),
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().x, mPos.y ) ),
						mBlend,
						mRender
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, LM->GetColorFromPos( Vector2f( mPos.x, mPos.y ) ), mAngle, mScale, mBlend, mRender );
				}
			}
		} else {
			if ( NULL != mVertexColors ) {
				mSubTexture->Draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[4], mBlend, mRender );
			} else {
				mSubTexture->Draw( mPos.x, mPos.y, mColor, mAngle, mScale, mBlend, mRender );
			}
		}
	}
}

void cGameObjectSubTextureEx::FlagSet( const Uint32& Flag ) {
	mRender = RenderModeFromFlags();

	cGameObject::FlagSet( Flag );
}

}}
