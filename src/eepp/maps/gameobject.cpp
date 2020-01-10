#include <eepp/maps/gameobject.hpp>
#include <eepp/maps/tilemaplayer.hpp>

namespace EE { namespace Maps {

GameObject::GameObject( const Uint32& Flags, MapLayer* Layer ) : mFlags( Flags ), mLayer( Layer ) {}

GameObject::~GameObject() {}

Uint32 GameObject::getType() const {
	return GAMEOBJECT_TYPE_BASE;
}

bool GameObject::isType( const Uint32& type ) {
	return type == GameObject::getType();
}

const Uint32& GameObject::getFlags() const {
	return mFlags;
}

Uint32 GameObject::getFlag( const Uint32& Flag ) {
	return mFlags & Flag;
}

void GameObject::setFlag( const Uint32& Flag ) {
	if ( !( mFlags & Flag ) ) {
		mFlags |= Flag;
	}
}

void GameObject::clearFlag( const Uint32& Flag ) {
	if ( mFlags & Flag ) {
		mFlags &= ~Flag;
	}
}

bool GameObject::isBlocked() const {
	return 0 != ( mFlags & GObjFlags::GAMEOBJECT_BLOCKED );
}

void GameObject::setBlocked( bool blocked ) {
	blocked ? setFlag( GObjFlags::GAMEOBJECT_BLOCKED ) : clearFlag( GObjFlags::GAMEOBJECT_BLOCKED );
}

bool GameObject::isRotated() const {
	return 0 != ( mFlags & GObjFlags::GAMEOBJECT_ROTATE_90DEG );
}

void GameObject::setRotated( bool rotated ) {
	rotated ? setFlag( GObjFlags::GAMEOBJECT_ROTATE_90DEG )
			: clearFlag( GObjFlags::GAMEOBJECT_ROTATE_90DEG );
}

bool GameObject::isMirrored() const {
	return 0 != ( mFlags & GObjFlags::GAMEOBJECT_MIRRORED );
}

void GameObject::setMirrored( bool mirrored ) {
	mirrored ? setFlag( GObjFlags::GAMEOBJECT_MIRRORED )
			 : clearFlag( GObjFlags::GAMEOBJECT_MIRRORED );
}

bool GameObject::isFliped() const {
	return 0 != ( mFlags & GObjFlags::GAMEOBJECT_FLIPED );
}

void GameObject::setFliped( bool fliped ) {
	fliped ? setFlag( GObjFlags::GAMEOBJECT_FLIPED ) : clearFlag( GObjFlags::GAMEOBJECT_FLIPED );
}

bool GameObject::isBlendAdd() const {
	return 0 != ( mFlags & GObjFlags::GAMEOBJECT_BLEND_ADD );
}

void GameObject::setBlendAdd( bool blendAdd ) {
	blendAdd ? setFlag( GObjFlags::GAMEOBJECT_BLEND_ADD )
			 : clearFlag( GObjFlags::GAMEOBJECT_BLEND_ADD );
}

void GameObject::draw() {}

void GameObject::update( const Time& dt ) {}

Vector2f GameObject::getPosition() const {
	return Vector2f();
}

void GameObject::setPosition( Vector2f pos ) {
	autoFixTilePos();
}

Vector2i GameObject::getTilePosition() const {
	return Vector2i();
}

void GameObject::setTilePosition( Vector2i pos ) {}

Sizei GameObject::getSize() {
	return Sizei();
}

Uint32 GameObject::getDataId() {
	return 0;
}

void GameObject::setDataId( Uint32 Id ) {}

RenderMode GameObject::getRenderModeFromFlags() {
	RenderMode Render = RENDER_NORMAL;

	if ( ( mFlags & GObjFlags::GAMEOBJECT_MIRRORED ) &&
		 ( mFlags & GObjFlags::GAMEOBJECT_FLIPED ) ) {
		Render = RENDER_FLIPPED_MIRRORED;
	} else if ( mFlags & GObjFlags::GAMEOBJECT_MIRRORED ) {
		Render = RENDER_MIRROR;
	} else if ( mFlags & GObjFlags::GAMEOBJECT_FLIPED ) {
		Render = RENDER_FLIPPED;
	}

	return Render;
}

BlendMode GameObject::getBlendModeFromFlags() {
	return isBlendAdd() ? BlendMode( BlendMode::DstColor, BlendMode::One ) : BlendAlpha;
}

MapLayer* GameObject::getLayer() const {
	return mLayer;
}

void GameObject::autoFixTilePos() {
	if ( ( mFlags & GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS ) && NULL != mLayer &&
		 mLayer->getType() == MAP_LAYER_TILED ) {
		Vector2i CurPos = getTilePosition();

		assignTilePos();

		Vector2i NewPos = getTilePosition();

		if ( CurPos != NewPos ) {
			TileMapLayer* TLayer = static_cast<TileMapLayer*>( mLayer );

			if ( TLayer->getGameObject( CurPos ) == this ) {
				TLayer->moveTileObject( CurPos, NewPos );
			}
		}
	}
}

void GameObject::assignTilePos() {
	TileMapLayer* TLayer = static_cast<TileMapLayer*>( mLayer );

	setTilePosition( TLayer->getTilePosFromPos( getPosition() ) );
}

Float GameObject::getRotation() {
	return isRotated() ? 90 : 0;
}

}} // namespace EE::Maps
