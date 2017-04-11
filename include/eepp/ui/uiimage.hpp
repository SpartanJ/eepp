#ifndef EE_UI_UIIMAGE_HPP
#define EE_UI_UIIMAGE_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIImage : public UIWidget {
	public:
		static UIImage * New();

		UIImage();

		virtual ~UIImage();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Drawable * getDrawable() const;

		void setDrawable( Drawable * drawable );

		const Color& getColor() const;

		void setColor( const Color& col );

		const EE_RENDER_MODE& getRenderMode() const;

		void setRenderMode( const EE_RENDER_MODE& render );

		const Vector2i& getAlignOffset() const;

		virtual void loadFromXmlNode( const pugi::xml_node& node );

		Uint32 getScaleType() const;

		UIImage * setScaleType(const Uint32 & scaleType);
	protected:
		Uint32			mScaleType;
		Drawable *		mDrawable;
		Color			mColor;
		Vector2i		mAlignOffset;
		Vector2f		mDestSize;

		virtual void onSizeChange();

		virtual void onAlignChange();

		void onAutoSize();

		void calcDestSize();

		void autoAlign();

		void safeDeleteDrawable();
};

}}


#endif
