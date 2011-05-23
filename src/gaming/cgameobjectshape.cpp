#include "cgameobjectshape.hpp"

namespace EE { namespace Gaming {

cGameObjectShape::cGameObjectShape( const Uint32& Flags, cShape * Shape, const eeVector2f& Pos ) :
	cGameObject( Flags ),
	mShape( Shape ),
	mPos( Pos )
{
}

cGameObjectShape::~cGameObjectShape() {
}

Uint32 cGameObjectShape::Type() const {
	return GAMEOBJECT_TYPE_SHAPE;
}

void cGameObjectShape::Draw() {
	if ( NULL != mShape ) {
		Uint8 Both = 0;
		EE_RENDERTYPE Render = RN_NORMAL;

		if ( mFlags & GObjFlags::GAMEOBJECT_MIRRORED ) {
			Render = RN_MIRROR;
			Both |= 1 << RN_MIRROR;
		}

		if ( mFlags & GObjFlags::GAMEOBJECT_FLIPED ) {
			Render = RN_FLIP;
			Both |= 1 << RN_FLIP;
		}

		if ( ( Both & RN_MIRROR ) && ( Both & RN_FLIP ) )
			Render = RN_FLIPMIRROR;

		mShape->Draw( mPos.x, mPos.y, eeColorA(), 0.f, 1.f, ALPHA_NORMAL, Render );
	}
}

void cGameObjectShape::Update() {
}

eeVector2f cGameObjectShape::Pos() const {
	return mPos;
}

void cGameObjectShape::Pos( eeVector2f pos ) {
	mPos = pos;
}

cShape * cGameObjectShape::Shape() const {
	return mShape;
}

void cGameObjectShape::Shape( cShape * shape ) {
	mShape = shape;
}

}}
