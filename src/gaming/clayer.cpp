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

}}

