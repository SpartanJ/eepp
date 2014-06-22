#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include <eepp/ui/cuicomplexcontrol.hpp>

namespace EE { namespace UI {

class EE_API cUIGfx : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					SubTexture( NULL ),
					SubTextureColor(),
					SubTextureRender( RN_NORMAL )
				{
				}

				inline ~CreateParams() {}

				Graphics::SubTexture * 	SubTexture;
				ColorA					SubTextureColor;
				EE_RENDER_MODE			SubTextureRender;
		};

		cUIGfx( const cUIGfx::CreateParams& Params );

		virtual ~cUIGfx();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Draw();

		virtual void Alpha( const Float& alpha );

		Graphics::SubTexture * SubTexture() const;

		void SubTexture( Graphics::SubTexture * subTexture );

		const ColorA& Color() const;

		void Color( const ColorA& color );

		const EE_RENDER_MODE& RenderMode() const;

		void RenderMode( const EE_RENDER_MODE& render );

		const Vector2i& AlignOffset() const;
	protected:
		Graphics::SubTexture * 	mSubTexture;
		ColorA					mColor;
		EE_RENDER_MODE			mRender;
		Vector2i				mAlignOffset;

		virtual void OnSizeChange();

		void AutoSize();

		void AutoAlign();

		void DrawSubTexture();
};

}}

#endif
