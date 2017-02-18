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

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::SubTexture * subTexture() const;

		void subTexture( Graphics::SubTexture * subTexture );

		const ColorA& color() const;

		void color( const ColorA& col );

		const EE_RENDER_MODE& renderMode() const;

		void renderMode( const EE_RENDER_MODE& render );

		const Vector2i& alignOffset() const;
	protected:
		Graphics::SubTexture * 	mSubTexture;
		ColorA					mColor;
		EE_RENDER_MODE			mRender;
		Vector2i				mAlignOffset;

		virtual void onSizeChange();

		void autoSize();

		void autoAlign();

		void drawSubTexture();
};

}}

#endif
