#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include <eepp/ui/uicomplexcontrol.hpp>

namespace EE { namespace UI {

class EE_API UIGfx : public UIComplexControl {
	public:
		static UIGfx * New();

		UIGfx();

		virtual ~UIGfx();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::SubTexture * getSubTexture() const;

		void setSubTexture( Graphics::SubTexture * subTexture );

		const ColorA& getColor() const;

		void setColor( const ColorA& col );

		const EE_RENDER_MODE& getRenderMode() const;

		void setRenderMode( const EE_RENDER_MODE& render );

		const Vector2i& getAlignOffset() const;
	protected:
		Graphics::SubTexture * 	mSubTexture;
		ColorA					mColor;
		EE_RENDER_MODE			mRender;
		Vector2i				mAlignOffset;

		virtual void onSizeChange();

		virtual void onAlignChange();

		void onAutoSize();

		void autoAlign();

		void drawSubTexture();
};

}}

#endif
