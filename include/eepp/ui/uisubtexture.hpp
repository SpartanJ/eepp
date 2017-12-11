#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace Graphics {
class SubTexture;
}}

namespace EE { namespace UI {

class EE_API UISubTexture : public UIWidget {
	public:
		static UISubTexture * New();

		UISubTexture();

		virtual ~UISubTexture();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::SubTexture * getSubTexture() const;

		UISubTexture * setSubTexture( Graphics::SubTexture * subTexture );

		const Color& getColor() const;

		void setColor( const Color& col );

		const RenderMode& getRenderMode() const;

		void setRenderMode( const RenderMode& render );

		const Vector2i& getAlignOffset() const;

		virtual void loadFromXmlNode( const pugi::xml_node& node );

		Uint32 getScaleType() const;

		UISubTexture * setScaleType(const Uint32 & scaleType);
	protected:
		Uint32					mScaleType;
		Graphics::SubTexture * 	mSubTexture;
		Color					mColor;
		RenderMode			mRender;
		Vector2i				mAlignOffset;

		virtual void onSizeChange();

		virtual void onAlignChange();

		void onAutoSize();

		void autoAlign();

		void drawSubTexture();
};

}}

#endif
