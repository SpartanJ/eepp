#include "clayer.hpp"
#include "cmap.hpp"

namespace EE { namespace Gaming {

cLayer::cLayer( cMap * map, Uint32 type, Uint32 flags, std::string name, eeVector2f offset ) :
	mMap( map ),
	mType( type ),
	mFlags( flags ),
	mOffset( offset ),
	mNameHash( MakeHash( name ) ),
	mName( name )
{
}

cLayer::~cLayer() {

}

const Uint32& cLayer::Flags() const {
	return mFlags;
}

Uint32 cLayer::FlagGet( const Uint32& Flag ) {
	return mFlags & Flag;
}

void cLayer::FlagSet( const Uint32& Flag ) {
	if ( !( mFlags & Flag ) ) {
		mFlags |= Flag;
	}
}

void cLayer::FlagClear( const Uint32& Flag ) {
	if ( mFlags & Flag ) {
		mFlags &= ~Flag;
	}
}

const Uint32& cLayer::Type() const {
	return mType;
}

cMap * cLayer::Map() const {
	return mMap;
}

const eeVector2f& cLayer::Offset() const {
	return mOffset;
}

void cLayer::Offset( const eeVector2f& offset ) {
	mOffset = offset;
}

const std::string& cLayer::Name() const {
	return mName;
}

const Uint32& cLayer::Id() const {
	return mNameHash;
}

void cLayer::ClearProperties() {
	mProperties.clear();
}

void cLayer::AddProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void cLayer::EditProperty( std::string Text, std::string Value ) {
	mProperties[ Text ] = Value;
}

void cLayer::RemoveProperty( std::string Text ) {
	mProperties.erase( Text );
}

cLayer::PropertiesMap& cLayer::GetProperties() {
	return mProperties;
}

void cLayer::Visible( const bool& visible ) {
	visible ? FlagSet( LAYER_FLAG_VISIBLE ) : FlagClear( LAYER_FLAG_VISIBLE );
}

bool cLayer::Visible() {
	return 0 != FlagGet( LAYER_FLAG_VISIBLE );
}

bool cLayer::LightsEnabled() {
	return 0 != FlagGet( LAYER_FLAG_LIGHTS_ENABLED );
}

void cLayer::LightsEnabled( const bool& enabled ) {
	enabled ? FlagSet( LAYER_FLAG_LIGHTS_ENABLED ) : FlagClear( LAYER_FLAG_LIGHTS_ENABLED );
}

}}

