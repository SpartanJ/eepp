#include <eepp/maps/maplayer.hpp>
#include <eepp/maps/tilemap.hpp>

namespace EE { namespace Maps {

MapLayer::MapLayer( TileMap* map, Uint32 type, Uint32 flags, std::string name, Vector2f offset ) :
	mMap( map ),
	mType( type ),
	mFlags( flags ),
	mOffset( offset ),
	mNameHash( String::hash( name ) ),
	mName( name ) {}

MapLayer::~MapLayer() {}

const Uint32& MapLayer::getFlags() const {
	return mFlags;
}

Uint32 MapLayer::getFlag( const Uint32& Flag ) {
	return mFlags & Flag;
}

void MapLayer::setFlag( const Uint32& Flag ) {
	if ( !( mFlags & Flag ) ) {
		mFlags |= Flag;
	}
}

void MapLayer::clearFlag( const Uint32& Flag ) {
	if ( mFlags & Flag ) {
		mFlags &= ~Flag;
	}
}

const Uint32& MapLayer::getType() const {
	return mType;
}

TileMap* MapLayer::getMap() const {
	return mMap;
}

const Vector2f& MapLayer::getOffset() const {
	return mOffset;
}

void MapLayer::setOffset( const Vector2f& offset ) {
	mOffset = offset;
}

void MapLayer::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
}

const std::string& MapLayer::getName() const {
	return mName;
}

const Uint32& MapLayer::getId() const {
	return mNameHash;
}

void MapLayer::clearProperties() {
	mProperties.clear();
}

void MapLayer::addProperty( std::string Text, std::string Value ) {
	mProperties[Text] = Value;
}

void MapLayer::editProperty( std::string Text, std::string Value ) {
	mProperties[Text] = Value;
}

void MapLayer::removeProperty( std::string Text ) {
	mProperties.erase( Text );
}

MapLayer::PropertiesMap& MapLayer::getProperties() {
	return mProperties;
}

void MapLayer::setVisible( const bool& visible ) {
	visible ? setFlag( LAYER_FLAG_VISIBLE ) : clearFlag( LAYER_FLAG_VISIBLE );
}

bool MapLayer::isVisible() {
	return 0 != getFlag( LAYER_FLAG_VISIBLE );
}

bool MapLayer::getLightsEnabled() {
	return 0 != getFlag( LAYER_FLAG_LIGHTS_ENABLED );
}

void MapLayer::setLightsEnabled( const bool& enabled ) {
	enabled ? setFlag( LAYER_FLAG_LIGHTS_ENABLED ) : clearFlag( LAYER_FLAG_LIGHTS_ENABLED );
}

}} // namespace EE::Maps
