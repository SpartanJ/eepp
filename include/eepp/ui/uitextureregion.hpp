#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace Graphics {
class TextureRegion;
}}

namespace EE { namespace UI {

class EE_API UITextureRegion : public UIWidget {
	public:
		static UITextureRegion * New();

		UITextureRegion();

		virtual ~UITextureRegion();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::TextureRegion * getTextureRegion() const;

		UITextureRegion * setTextureRegion( Graphics::TextureRegion * TextureRegion );

		const Color& getColor() const;

		void setColor( const Color& col );

		const RenderMode& getRenderMode() const;

		void setRenderMode( const RenderMode& render );

		const Vector2f& getAlignOffset() const;

		Uint32 getScaleType() const;

		UITextureRegion * setScaleType(const Uint32 & scaleType);

		virtual bool applyProperty( const StyleSheetProperty& attribute, const Uint32& state = UIState::StateFlagNormal );
	protected:
		Uint32					mScaleType;
		Graphics::TextureRegion * 	mTextureRegion;
		Color					mColor;
		RenderMode				mRender;
		Vector2f				mAlignOffset;

		virtual void onSizeChange();

		virtual void onAlignChange();

		void onAutoSize();

		void autoAlign();

		void drawTextureRegion();
};

}}

#endif
