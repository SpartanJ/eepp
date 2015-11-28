#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/tilemap.hpp>

namespace EE { namespace Gaming {

MapLayer::MapLayer( TileMap * map, Uint32 type, Uint32 flags, std::string name, Vector2f offset ) :
	mMap( map ),
	mType( type ),
	mFlags( flags ),
	mOffset( offset ),
	mNameHash( String::Hash( name ) ),
	mName( name )
{
}

MapLayer::~MapLayer() {

}

const Uint32& MapLayer::Flags() const {
	return mFlags;
}

Uint32 MapLayer::FlagGet( const Uint32& Flag ) {
	return mFlags & Flag;
}

void MapLayer::FlagSet( const Uint32& Flag ) {
	if ( !( mFlags & Flag ) ) {
		mFlags |= Flag;
	}
}

void MapLayer::FlagClear( const Uint32& Flag ) {
	if ( mFlags & Flag ) {
		mFlags &= ~Flag;
	}
}

const Uint32& MapLayer::Type() const {
	return mType;
}

TileMap * MapLayer::Map() const {
	return mMap;
}

const Vector2f& MapLayer::Offset() const {
	return mOffset;
}

void MapLayer::Offset( const Vector2f& offset ) {
	mOffset = offset;
}

void MapLayer::Name( const std::string& name ) {
	mName		= name;
	mNameHash	= String::Hash( mName );
}

const std::string& MapLayer::Name() const {
	return mName;
}

const Uint32& MapLayer::Id() const {
	return mNameHash;
}

void MapLayer::ClearProperties() {
	mProperties.clear();
}

void MapLayer::AddProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void MapLayer::EditProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void MapLayer::RemoveProperty( std::string Text ) {
	mProperties.erase( Text );
}

MapLayer::PropertiesMap& MapLayer::GetProperties() {
	return mProperties;
}

void MapLayer::Visible( const bool& visible ) {
	visible ? FlagSet( LAYER_FLAG_VISIBLE ) : FlagClear( LAYER_FLAG_VISIBLE );
}

bool MapLayer::Visible() {
	return 0 != FlagGet( LAYER_FLAG_VISIBLE );
}

bool MapLayer::LightsEnabled() {
	return 0 != FlagGet( LAYER_FLAG_LIGHTS_ENABLED );
}

void MapLayer::LightsEnabled( const bool& enabled ) {
	enabled ? FlagSet( LAYER_FLAG_LIGHTS_ENABLED ) : FlagClear( LAYER_FLAG_LIGHTS_ENABLED );
}

}}

