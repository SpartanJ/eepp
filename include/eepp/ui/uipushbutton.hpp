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
					UITheme * Theme = UIThemeManager::instance()->defaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->font();
						FontColor		= Theme->fontColor();
						FontShadowColor	= Theme->fontShadowColor();
						FontOverColor	= Theme->fontOverColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->defaultFont();
				}

				inline ~CreateParams() {}

				inline void SetIcon( SubTexture * icon ) {
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

		virtual ~UIPushButton();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void icon( SubTexture * icon );

		virtual UIGfx * icon() const;

		virtual void text( const String& text );

		virtual const String& text();

		void padding( const Recti& padding );

		const Recti& padding() const;

		void iconHorizontalMargin( Int32 margin );

		const Int32& iconHorizontalMargin() const;

		UITextBox * getTextBox() const;

		const ColorA& fontColor() const;

		void fontColor( const ColorA& color );

		const ColorA& fontOverColor() const;

		void fontOverColor( const ColorA& color );
	protected:
		ColorA		mFontColor;
		ColorA		mFontOverColor;
		UIGfx * 		mIcon;
		UITextBox * 	mTextBox;
		Int32			mIconSpace;

		virtual void onSizeChange();

		void autoPadding();

		virtual void onAlphaChange();

		virtual void onStateChange();

		virtual void doAftersetTheme();

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onKeyUp( const UIEventKey& Event );
};

}}

#endif

