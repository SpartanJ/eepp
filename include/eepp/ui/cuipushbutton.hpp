#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/cuigfx.hpp>

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
					IconAutoMargin( true ),
					IconMinSize( 0, 0 )
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

				inline void SetIcon( cSubTexture * icon ) {
					Icon = icon;

					if ( !IconHorizontalMargin )
						IconHorizontalMargin = 4;
				}

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA	FontShadowColor;
				eeColorA 	FontOverColor;
				cSubTexture * 	Icon;
				Int32 		IconHorizontalMargin;
				bool 		IconAutoMargin;
				eeSize		IconMinSize;
		};

		cUIPushButton( const cUIPushButton::CreateParams& Params );

		virtual ~cUIPushButton();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		virtual void Icon( cSubTexture * Icon );

		virtual cUIGfx * Icon() const;

		virtual void Text( const String& text );

		virtual const String& Text();

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

		virtual void DoAfterSetTheme();

		virtual Uint32 OnKeyDown( const cUIEventKey& Event );

		virtual Uint32 OnKeyUp( const cUIEventKey& Event );
};

}}

#endif

