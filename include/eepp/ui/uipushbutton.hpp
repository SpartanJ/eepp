#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uigfx.hpp>

namespace EE { namespace UI {

class EE_API UIPushButton : public UIComplexControl {
	public:
		static UIPushButton * New();

		UIPushButton();

		virtual ~UIPushButton();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual UIPushButton * setIcon( SubTexture * icon );

		virtual UIGfx * getIcon() const;

		virtual void setText( const String& text );

		virtual const String& getText();

		void setPadding( const Recti& padding );

		const Recti& getPadding() const;

		void setIconHorizontalMargin( Int32 margin );

		const Int32& getIconHorizontalMargin() const;

		UITextBox * getTextBox() const;

		void setFont( Font * font );

		Font * getFont();

		const ColorA& getFontColor() const;

		void setFontColor( const ColorA& color );

		const ColorA& getFontOverColor() const;

		void setFontOverColor( const ColorA& color );

		const ColorA& getFontShadowColor() const;

		void setFontShadowColor( const ColorA& color );

		TooltipStyleConfig getStyleConfig() const;

		void setStyleConfig(const PushButtonStyleConfig & styleConfig);
	protected:
		PushButtonStyleConfig mStyleConfig;
		UIGfx * 		mIcon;
		UITextBox * 	mTextBox;

		virtual void onSizeChange();

		void autoPadding();

		virtual void onAlphaChange();

		virtual void onStateChange();

		virtual void onAlignChange();

		virtual void onThemeLoaded();

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onKeyUp( const UIEventKey& Event );
};

}}

#endif

