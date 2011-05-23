#ifndef CGAMEOBJECTSHAPEEX_HPP
#define CGAMEOBJECTSHAPEEX_HPP

#include "base.hpp"
#include "cgameobjectshape.hpp"

namespace EE { namespace Gaming {

class cGameObjectShapeEx : public cGameObjectShape {
	public:
		cGameObjectShapeEx( const Uint32& Flags = GObjFlags::GAMEOBJECT_STATIC, cShape * Shape = NULL, const eeVector2f& Pos = eeVector2f(), EE_PRE_BLEND_FUNC Blend = ALPHA_NORMAL, EE_RENDERTYPE Render = RN_NORMAL, eeFloat Angle = 0.f, eeFloat Scale = 1.f, eeColorA Color = eeColorA() );

		virtual ~cGameObjectShapeEx();

		virtual void Draw();

		virtual Uint32 Type() const;
	protected:
		EE_PRE_BLEND_FUNC	mBlend;
		EE_RENDERTYPE		mRender;
		eeFloat				mAngle;
		eeFloat				mScale;
		eeColorA			mColor;
		eeColorA *			mVertexColors;
};

}}

#endif
