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
					UITheme * Theme = UIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->Font();
						FontColor		= Theme->FontColor();
						FontShadowColor	= Theme->FontShadowColor();
						FontOverColor	= Theme->FontOverColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->DefaultFont();
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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		virtual void Icon( SubTexture * Icon );

		virtual UIGfx * Icon() const;

		virtual void Text( const String& text );

		virtual const String& Text();

		void Padding( const Recti& padding );

		const Recti& Padding() const;

		void IconHorizontalMargin( Int32 margin );

		const Int32& IconHorizontalMargin() const;

		UITextBox * TextBox() const;

		const ColorA& FontColor() const;

		void FontColor( const ColorA& color );

		const ColorA& FontOverColor() const;

		void FontOverColor( const ColorA& color );
	protected:
		ColorA		mFontColor;
		ColorA		mFontOverColor;
		UIGfx * 		mIcon;
		UITextBox * 	mTextBox;
		Int32			mIconSpace;

		virtual void OnSizeChange();

		void AutoPadding();

		virtual void OnAlphaChange();

		virtual void OnStateChange();

		virtual void DoAfterSetTheme();

		virtual Uint32 OnKeyDown( const UIEventKey& Event );

		virtual Uint32 OnKeyUp( const UIEventKey& Event );
};

}}

#endif

