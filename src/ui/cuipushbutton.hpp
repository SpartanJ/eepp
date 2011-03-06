#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include "cuicomplexcontrol.hpp"
#include "cuitextbox.hpp"
#include "cuigfx.hpp"

namespace EE { namespace UI {

class EE_API cUIPushButton : public cUIComplexControl {
	public:
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() :
					cUITextBox::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					Icon( NULL ),
					IconHorizontalMargin( 0 ),
					IconAutoMargin( true )
				{
					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->Font();
						FontColor		= Theme->FontColor();
						FontShadowColor	= Theme->FontShadowColor();
						FontOverColor	= Theme->FontOverColor();
					}

					if ( NULL == Font )
						Font = cUIThemeManager::instance()->DefaultFont();
				}

				inline ~CreateParams() {}

				inline void SetIcon( cShape * icon ) {
					Icon = icon;

					if ( !IconHorizontalMargin )
						IconHorizontalMargin = 4;
				}

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA	FontShadowColor;
				eeColorA 	FontOverColor;
				cShape * 	Icon;
				Int32 		IconHorizontalMargin;
				bool 		IconAutoMargin;
		};

		cUIPushButton( const cUIPushButton::CreateParams& Params );

		~cUIPushButton();

		virtual void SetTheme( cUITheme * Theme );

		void Icon( cShape * Icon );

		cUIGfx * Icon() const;

		void Text( const String& text );

		const String& Text();

		void Padding( const eeRecti& padding );

		const eeRecti& Padding() const;

		void IconHorizontalMargin( Int32 margin );

		const Int32& IconHorizontalMargin() const;

		cUITextBox * TextBox() const;
	protected:
		eeColorA		mFontColor;
		eeColorA		mFontOverColor;
		cUIGfx * 		mIcon;
		cUITextBox * 	mTextBox;
		Int32			mIconSpace;

		virtual void OnSizeChange();

		void AutoPadding();

		virtual void OnAlphaChange();

		virtual void OnStateChange();

		void DoAfterSetTheme();
};

}}

#endif

