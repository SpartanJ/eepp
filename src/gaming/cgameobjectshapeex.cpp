#include "cgameobjectshapeex.hpp"

namespace EE { namespace Gaming {

cGameObjectShapeEx::cGameObjectShapeEx( const Uint32& Flags, cShape * Shape, const eeVector2f& Pos, EE_PRE_BLEND_FUNC Blend, EE_RENDERTYPE Render, eeFloat Angle, eeFloat Scale, eeColorA Color) :
	cGameObjectShape( Flags, Shape, Pos ),
	mBlend( Blend ),
	mRender( Render ),
	mAngle( Angle ),
	mScale( Scale ),
	mColor( Color ),
	mVertexColors( NULL )
{
	mRender = RenderTypeFromFlags();
}

cGameObjectShapeEx::~cGameObjectShapeEx()
{
}

Uint32 cGameObjectShapeEx::Type() const {
	return GAMEOBJECT_TYPE_SHAPEEX;
}

void cGameObjectShapeEx::Draw() {
	if ( NULL != mShape ) {
		if ( NULL != mVertexColors )
			mShape->Draw( mPos.x, mPos.y, mAngle, mScale, mVertexColors[0], mVertexColors[1], mVertexColors[2], mVertexColors[4], mBlend, mRender );
		else
			mShape->Draw( mPos.x, mPos.y, mColor, mAngle, mScale, mBlend, mRender );
	}
}

void cGameObjectShapeEx::FlagSet( const Uint32& Flag ) {
	mRender = RenderTypeFromFlags();

	cGameObject::FlagSet( Flag );
}

}}
