#include "cgameobjectshape.hpp"
#include "../graphics/cshapegroupmanager.hpp"

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
		mShape->Draw( mPos.x, mPos.y, eeColorA(), 0.f, 1.f, ALPHA_NORMAL, RenderTypeFromFlags() );
	}
}

void cGameObjectShape::Update() {
}

eeVector2f cGameObjectShape::Pos() const {
	return mPos;
}

eeSize cGameObjectShape::Size() {
	if ( NULL != mShape )
		return mShape->RealSize();

	return eeSize();
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

Uint32 cGameObjectShape::DataId() {
	return mShape->Id();
}

void cGameObjectShape::DataId( Uint32 Id ) {
	Shape( cShapeGroupManager::instance()->GetShapeById( Id ) );
}

}}
