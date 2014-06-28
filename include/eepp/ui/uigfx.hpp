#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include <eepp/ui/uicomplexcontrol.hpp>

namespace EE { namespace UI {

class EE_API UIGfx : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
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

		UIGfx( const UIGfx::CreateParams& Params );

		virtual ~UIGfx();

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
