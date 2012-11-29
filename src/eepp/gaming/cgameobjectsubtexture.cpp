#include <eepp/gaming/cgameobjectsubtexture.hpp>
#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/gaming/clightmanager.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>

namespace EE { namespace Gaming {

cGameObjectSubTexture::cGameObjectSubTexture( const Uint32& Flags, cLayer * Layer, cSubTexture * SubTexture, const eeVector2f& Pos ) :
	cGameObject( Flags, Layer ),
	mSubTexture( SubTexture ),
	mPos( Pos )
{
	AssignTilePos();
}

cGameObjectSubTexture::~cGameObjectSubTexture() {
}

Uint32 cGameObjectSubTexture::Type() const {
	return GAMEOBJECT_TYPE_SUBTEXTURE;
}

bool cGameObjectSubTexture::IsType( const Uint32& type ) {
	return ( cGameObjectSubTexture::Type() == type ) ? true : cGameObject::IsType( type );
}

void cGameObjectSubTexture::Draw() {
	if ( NULL != mSubTexture ) {
		if ( mLayer->Map()->LightsEnabled() && mLayer->LightsEnabled() ) {
			cLightManager * LM = mLayer->Map()->GetLightManager();

			if ( MAP_LAYER_TILED == mLayer->Type() ) {
				eeVector2i Tile = reinterpret_cast<cTileLayer*> ( mLayer )->GetCurrentTile();

				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
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
					mSubTexture->Draw( mPos.x, mPos.y, *LM->GetTileColor( Tile ), GetAngle(), 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
				}
			} else {
				if ( LM->IsByVertex() ) {
					mSubTexture->Draw(
						mPos.x,
						mPos.y,
						GetAngle(),
						1.f,
						LM->GetColorFromPos( eeVector2f( mPos.x, mPos.y ) ),
						LM->GetColorFromPos( eeVector2f( mPos.x, mPos.y + mSubTexture->DestHeight() ) ),
						LM->GetColorFromPos( eeVector2f( mPos.x + mSubTexture->DestWidth(), mPos.y + mSubTexture->DestHeight() ) ),
						LM->GetColorFromPos( eeVector2f( mPos.x + mSubTexture->DestWidth(), mPos.y ) ),
						ALPHA_NORMAL,
						RenderTypeFromFlags()
					);
				} else {
					mSubTexture->Draw( mPos.x, mPos.y, LM->GetColorFromPos( eeVector2f( mPos.x, mPos.y ) ), GetAngle(), 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
				}
			}
		} else {
			mSubTexture->Draw( mPos.x, mPos.y, eeColorA(), GetAngle(), 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
		}
	}
}

eeVector2f cGameObjectSubTexture::Pos() const {
	return mPos;
}

void cGameObjectSubTexture::Pos( eeVector2f pos ) {
	mPos = pos;
	cGameObject::Pos( pos );
}

eeVector2i cGameObjectSubTexture::TilePos() const {
	return mTilePos;
}

void cGameObjectSubTexture::TilePos( eeVector2i pos ) {
	mTilePos = pos;
}

eeSize cGameObjectSubTexture::Size() {
	if ( NULL != mSubTexture )
		return mSubTexture->RealSize();

	return eeSize();
}

cSubTexture * cGameObjectSubTexture::SubTexture() const {
	return mSubTexture;
}

void cGameObjectSubTexture::SubTexture( cSubTexture * subTexture ) {
	mSubTexture = subTexture;
}

Uint32 cGameObjectSubTexture::DataId() {
	return mSubTexture->Id();
}

void cGameObjectSubTexture::DataId( Uint32 Id ) {
	SubTexture( cTextureAtlasManager::instance()->GetSubTextureById( Id ) );
}

}}
