#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uigfx.hpp>

namespace EE { namespace UI {

class EE_API UIPushButton : public UIComplexControl {
	public:
		class CreateParams : public UITextBox::CreateParams {
			public:
				inline CreateParams() :
					UITextBox::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					Icon( NULL ),
					IconHorizontalMargin( 0 ),
					IconAutoMargin( true ),
					IconMinSize( 0, 0 )
				{
					UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->getFont();
						FontColor		= Theme->getFontColor();
						FontShadowColor	= Theme->getFontShadowColor();
						FontOverColor	= Theme->getFontOverColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->getDefaultFont();
				}

				inline ~CreateParams() {}

				inline void setIcon( SubTexture * icon ) {
					Icon = icon;

					if ( !IconHorizontalMargin )
						IconHorizontalMargin = 4;
				}

				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				ColorA				FontOverColor;
				SubTexture *		Icon;
				Int32				IconHorizontalMargin;
				bool				IconAutoMargin;
				Sizei				IconMinSize;
		};

		UIPushButton( const UIPushButton::CreateParams& Params );

		UIPushButton();

		virtual ~UIPushButton();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void setIcon( SubTexture * icon );

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
	protected:
		ColorA			mFontColor;
		ColorA			mFontOverColor;
		UIGfx * 		mIcon;
		UITextBox * 	mTextBox;
		Int32			mIconSpace;

		virtual void onSizeChange();

		void autoPadding();

		virtual void onAlphaChange();

		virtual void onStateChange();

		virtual void doAfterSetTheme();

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onKeyUp( const UIEventKey& Event );
};

}}

#endif

