#include <eepp/gaming/cgameobjectsubtexture.hpp>
#include <eepp/gaming/cmap.hpp>
#include <eepp/gaming/ctilelayer.hpp>
#include <eepp/gaming/clightmanager.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace Gaming {

cGameObjectSubTexture::cGameObjectSubTexture( const Uint32& Flags, cLayer * Layer, Graphics::SubTexture * SubTexture, const Vector2f& Pos ) :
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
						LM->GetColorFromPos( Vector2f( mPos.x + mSubTexture->DestSize().y, mPos.y ) ),
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
	}
}

Vector2f cGameObjectSubTexture::Pos() const {
	return mPos;
}

void cGameObjectSubTexture::Pos( Vector2f pos ) {
	mPos = pos;
	cGameObject::Pos( pos );
}

Vector2i cGameObjectSubTexture::TilePos() const {
	return mTilePos;
}

void cGameObjectSubTexture::TilePos( Vector2i pos ) {
	mTilePos = pos;
}

Sizei cGameObjectSubTexture::Size() {
	if ( NULL != mSubTexture )
		return mSubTexture->RealSize();

	return Sizei();
}

Graphics::SubTexture * cGameObjectSubTexture::SubTexture() const {
	return mSubTexture;
}

void cGameObjectSubTexture::SubTexture( Graphics::SubTexture * subTexture ) {
	mSubTexture = subTexture;
}

Uint32 cGameObjectSubTexture::DataId() {
	return mSubTexture->Id();
}

void cGameObjectSubTexture::DataId( Uint32 Id ) {
	SubTexture( TextureAtlasManager::instance()->GetSubTextureById( Id ) );
}

}}
