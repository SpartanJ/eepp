#include <eepp/gaming/cgameobject.hpp>
#include <eepp/gaming/ctilelayer.hpp>

namespace EE { namespace Gaming {

cGameObject::cGameObject(  const Uint32& Flags, cLayer * Layer ) :
	mFlags( Flags ),
	mLayer( Layer )
{
}

cGameObject::~cGameObject()
{
}

Uint32 cGameObject::Type() const {
	return GAMEOBJECT_TYPE_BASE;
}

bool cGameObject::IsType( const Uint32& type ) {
	return type == cGameObject::Type();
}

const Uint32& cGameObject::Flags() const {
	return mFlags;
}

Uint32 cGameObject::FlagGet( const Uint32& Flag ) {
	return mFlags & Flag;
}

void cGameObject::FlagSet( const Uint32& Flag ) {
	if ( !( mFlags & Flag ) ) {
		mFlags |= Flag;
	}
}

void cGameObject::FlagClear( const Uint32& Flag ) {
	if ( mFlags & Flag ) {
		mFlags &= ~Flag;
	}
}

Uint32 cGameObject::Blocked() const {
	return mFlags & GObjFlags::GAMEOBJECT_BLOCKED;
}

void cGameObject::Blocked( bool blocked ) {
	blocked ? FlagSet( GObjFlags::GAMEOBJECT_BLOCKED ) : FlagClear( GObjFlags::GAMEOBJECT_BLOCKED );
}

Uint32 cGameObject::Rotated() const {
	return mFlags & GObjFlags::GAMEOBJECT_ROTATE_90DEG;
}

void cGameObject::Rotated( bool rotated ) {
	rotated ? FlagSet( GObjFlags::GAMEOBJECT_ROTATE_90DEG ) : FlagClear( GObjFlags::GAMEOBJECT_ROTATE_90DEG );
}

Uint32 cGameObject::Mirrored() const {
	return mFlags & GObjFlags::GAMEOBJECT_MIRRORED;
}

void cGameObject::Mirrored( bool mirrored ) {
	mirrored ? FlagSet( GObjFlags::GAMEOBJECT_MIRRORED ) : FlagClear( GObjFlags::GAMEOBJECT_MIRRORED );
}

Uint32 cGameObject::Fliped() const {
	return mFlags & GObjFlags::GAMEOBJECT_FLIPED;
}

void cGameObject::Fliped( bool fliped ) {
	fliped ? FlagSet( GObjFlags::GAMEOBJECT_FLIPED ) : FlagClear( GObjFlags::GAMEOBJECT_FLIPED );
}

void cGameObject::Draw() {
}

void cGameObject::Update() {
}

Vector2f cGameObject::Pos() const {
	return Vector2f();
}

void cGameObject::Pos( Vector2f pos ) {
	AutoFixTilePos();
}

Vector2i cGameObject::TilePos() const {
	return Vector2i();
}

void cGameObject::TilePos( Vector2i pos ) {
}

Sizei cGameObject::Size() {
	return Sizei();
}

Uint32 cGameObject::DataId() {
	return 0;
}

void cGameObject::DataId( Uint32 Id ){
}

EE_RENDER_MODE cGameObject::RenderModeFromFlags() {
	EE_RENDER_MODE Render = RN_NORMAL;

	if ( ( mFlags & GObjFlags::GAMEOBJECT_MIRRORED ) && ( mFlags & GObjFlags::GAMEOBJECT_FLIPED ) ) {
		Render = RN_FLIPMIRROR;
	} else if ( mFlags & GObjFlags::GAMEOBJECT_MIRRORED ) {
		Render = RN_MIRROR;
	} else if ( mFlags & GObjFlags::GAMEOBJECT_FLIPED ) {
		Render = RN_FLIP;
	}

	return Render;
}

cLayer * cGameObject::Layer() const {
	return mLayer;
}

void cGameObject::AutoFixTilePos() {
	if ( ( mFlags & GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS ) && NULL != mLayer && mLayer->Type() == MAP_LAYER_TILED ) {
		Vector2i CurPos = TilePos();

		AssignTilePos();

		Vector2i NewPos = TilePos();

		if ( CurPos != NewPos ) {
			cTileLayer * TLayer = static_cast<cTileLayer *> ( mLayer );

			if ( TLayer->GetGameObject( CurPos ) == this ) {
				TLayer->MoveTileObject( CurPos, NewPos );
			}
		}
	}
}

void cGameObject::AssignTilePos() {
	cTileLayer * TLayer = static_cast<cTileLayer *> ( mLayer );

	TilePos( TLayer->GetTilePosFromPos( Pos() ) );
}

Float cGameObject::GetAngle() {
	return Rotated() ? 90 : 0;
}

}}
