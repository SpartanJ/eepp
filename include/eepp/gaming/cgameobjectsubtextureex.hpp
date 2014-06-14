#ifndef CGAMEOBJECTSUBTEXTUREEX_HPP
#define CGAMEOBJECTSUBTEXTUREEX_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/cgameobjectsubtexture.hpp>

namespace EE { namespace Gaming {

class EE_API cGameObjectSubTextureEx : public cGameObjectSubTexture {
	public:
		cGameObjectSubTextureEx( const Uint32& Flags, cLayer * Layer, cSubTexture * SubTexture = NULL, const eeVector2f& Pos = eeVector2f(), EE_BLEND_MODE Blend = ALPHA_NORMAL, EE_RENDER_MODE Render = RN_NORMAL, Float Angle = 0.f, eeVector2f Scale = eeVector2f::One, eeColorA Color = eeColorA() );

		virtual ~cGameObjectSubTextureEx();

		virtual void Draw();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type );

		virtual void FlagSet( const Uint32& Flag );
	protected:
		EE_BLEND_MODE	mBlend;
		EE_RENDER_MODE		mRender;
		Float				mAngle;
		eeVector2f			mScale;
		eeColorA			mColor;
		eeColorA *			mVertexColors;
};

}}

#endif
