#include "cgameobject.hpp"

namespace EE { namespace Gaming {

cGameObject::cGameObject(  const Uint32& Flags, cLayer * Layer ) :
	mFlags( Flags ),
	mLayer( Layer )
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

eeSize cGameObject::Size() {
	return eeSize();
}

Uint32 cGameObject::Type() const {
	return GAMEOBJECT_TYPE_BASE;
}

Uint32 cGameObject::DataId() {
	return 0;
}

void cGameObject::DataId( Uint32 Id ){
}

EE_RENDERTYPE cGameObject::RenderTypeFromFlags() {
	EE_RENDERTYPE Render = RN_NORMAL;

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

}}
