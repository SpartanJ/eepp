#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiimage.hpp>

namespace EE { namespace UI {

class EE_API UIPushButton : public UIWidget {
	public:
		static UIPushButton * New();

		UIPushButton();

		virtual ~UIPushButton();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual UIPushButton * setIcon( SubTexture * icon );

		virtual UIImage * getIcon() const;

		virtual UIPushButton * setText( const String& text );

		virtual const String& getText();

		void setPadding( const Recti& padding );

		const Recti& getPadding() const;

		void setIconHorizontalMargin( Int32 margin );

		const Int32& getIconHorizontalMargin() const;

		UITextView * getTextBox() const;

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

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		PushButtonStyleConfig mStyleConfig;
		UIImage * 		mIcon;
		UITextView * 	mTextBox;

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

