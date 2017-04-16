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

		virtual UIPushButton * setIcon( Drawable * icon );

		virtual UIImage * getIcon() const;

		virtual UIPushButton * setText( const String& text );

		virtual const String& getText();

		void setPadding( const Rect& padding );

		const Rect& getPadding() const;

		void setIconHorizontalMargin( Int32 margin );

		const Int32& getIconHorizontalMargin() const;

		UITextView * getTextBox() const;

		void setFont( Font * font );

		Font * getFont();

		const Color& getFontColor() const;

		void setFontColor( const Color& color );

		const Color& getFontOverColor() const;

		void setFontOverColor( const Color& color );

		const Color& getFontShadowColor() const;

		void setFontShadowColor( const Color& color );

		Uint32 getCharacterSize();

		void setCharacterSize( const Uint32& characterSize );

		const Uint32& getFontStyle() const;

		UIPushButton * setFontStyle( const Uint32& fontStyle );

		const Float & getOutlineThickness() const;

		UIPushButton * setOutlineThickness( const Float& outlineThickness );

		const Color& getOutlineColor() const;

		UIPushButton * setOutlineColor( const Color& outlineColor );

		UITooltipStyleConfig getStyleConfig() const;

		void setStyleConfig(const UIPushButtonStyleConfig & styleConfig);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		UIPushButtonStyleConfig mStyleConfig;
		UIImage * 	mIcon;
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

