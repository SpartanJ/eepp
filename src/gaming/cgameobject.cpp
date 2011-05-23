#include "cgameobject.hpp"

namespace EE { namespace Gaming {

cGameObject::cGameObject(  const Uint32& Flags ) :
	mFlags( Flags )
{
}

cGameObject::~cGameObject()
{
}

bool cGameObject::IsType( const Uint32& type ) {
	return type == Type();
}

bool cGameObject::InheritsFrom( const Uint32& Type ) {
	return false;
}

bool cGameObject::IsTypeOrInheritsFrom( const Uint32& Type ) {
	return IsType( Type ) || InheritsFrom( Type );
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

Uint32 cGameObject::IsBlocked() const {
	return mFlags & GObjFlags::GAMEOBJECT_BLOCKED;
}

void cGameObject::Draw() {
}

void cGameObject::Update() {
}

eeVector2f cGameObject::Pos() const {
	return eeVector2f();
}

void cGameObject::Pos( eeVector2f pos ) {
}

Uint32 cGameObject::Type() const {
	return GAMEOBJECT_TYPE_BASE;
}

}}
