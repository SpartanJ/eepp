#include <eepp/gaming/gameobject.hpp>
#include <eepp/gaming/tilemaplayer.hpp>

namespace EE { namespace Gaming {

GameObject::GameObject(  const Uint32& Flags, MapLayer * Layer ) :
	mFlags( Flags ),
	mLayer( Layer )
{
}

GameObject::~GameObject()
{
}

Uint32 GameObject::Type() const {
	return GAMEOBJECT_TYPE_BASE;
}

bool GameObject::IsType( const Uint32& type ) {
	return type == GameObject::Type();
}

const Uint32& GameObject::Flags() const {
	return mFlags;
}

Uint32 GameObject::FlagGet( const Uint32& Flag ) {
	return mFlags & Flag;
}

void GameObject::FlagSet( const Uint32& Flag ) {
	if ( !( mFlags & Flag ) ) {
		mFlags |= Flag;
	}
}

void GameObject::FlagClear( const Uint32& Flag ) {
	if ( mFlags & Flag ) {
		mFlags &= ~Flag;
	}
}

Uint32 GameObject::Blocked() const {
	return mFlags & GObjFlags::GAMEOBJECT_BLOCKED;
}

void GameObject::Blocked( bool blocked ) {
	blocked ? FlagSet( GObjFlags::GAMEOBJECT_BLOCKED ) : FlagClear( GObjFlags::GAMEOBJECT_BLOCKED );
}

Uint32 GameObject::Rotated() const {
	return mFlags & GObjFlags::GAMEOBJECT_ROTATE_90DEG;
}

void GameObject::Rotated( bool rotated ) {
	rotated ? FlagSet( GObjFlags::GAMEOBJECT_ROTATE_90DEG ) : FlagClear( GObjFlags::GAMEOBJECT_ROTATE_90DEG );
}

Uint32 GameObject::Mirrored() const {
	return mFlags & GObjFlags::GAMEOBJECT_MIRRORED;
}

void GameObject::Mirrored( bool mirrored ) {
	mirrored ? FlagSet( GObjFlags::GAMEOBJECT_MIRRORED ) : FlagClear( GObjFlags::GAMEOBJECT_MIRRORED );
}

Uint32 GameObject::Fliped() const {
	return mFlags & GObjFlags::GAMEOBJECT_FLIPED;
}

void GameObject::Fliped( bool fliped ) {
	fliped ? FlagSet( GObjFlags::GAMEOBJECT_FLIPED ) : FlagClear( GObjFlags::GAMEOBJECT_FLIPED );
}

void GameObject::Draw() {
}

void GameObject::Update() {
}

Vector2f GameObject::Pos() const {
	return Vector2f();
}

void GameObject::Pos( Vector2f pos ) {
	AutoFixTilePos();
}

Vector2i GameObject::TilePos() const {
	return Vector2i();
}

void GameObject::TilePos( Vector2i pos ) {
}

Sizei GameObject::Size() {
	return Sizei();
}

Uint32 GameObject::DataId() {
	return 0;
}

void GameObject::DataId( Uint32 Id ){
}

EE_RENDER_MODE GameObject::RenderModeFromFlags() {
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

MapLayer * GameObject::Layer() const {
	return mLayer;
}

void GameObject::AutoFixTilePos() {
	if ( ( mFlags & GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS ) && NULL != mLayer && mLayer->Type() == MAP_LAYER_TILED ) {
		Vector2i CurPos = TilePos();

		AssignTilePos();

		Vector2i NewPos = TilePos();

		if ( CurPos != NewPos ) {
			TileMapLayer * TLayer = static_cast<TileMapLayer *> ( mLayer );

			if ( TLayer->GetGameObject( CurPos ) == this ) {
				TLayer->MoveTileObject( CurPos, NewPos );
			}
		}
	}
}

void GameObject::AssignTilePos() {
	TileMapLayer * TLayer = static_cast<TileMapLayer *> ( mLayer );

	TilePos( TLayer->GetTilePosFromPos( Pos() ) );
}

Float GameObject::GetAngle() {
	return Rotated() ? 90 : 0;
}

}}
