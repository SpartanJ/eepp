#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include "cuitextbox.hpp"
#include "cuigfx.hpp"

namespace EE { namespace UI {

class EE_API cUIPushButton : public cUIControlAnim {
	public:
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() :
					cUITextBox::CreateParams(),
					Icon( NULL ),
					IconHorizontalMargin( 0 )
				{
				}

				inline ~CreateParams() {}

				cShape * Icon;

				inline void SetIcon( cShape * icon ) {
					Icon = icon;

					if ( !IconHorizontalMargin )
						IconHorizontalMargin = 4;
				}

				Int32 IconHorizontalMargin;
		};

		cUIPushButton( const cUIPushButton::CreateParams& Params );

		~cUIPushButton();

		virtual void SetTheme( cUITheme * Theme );

		void Icon( cShape * Icon );

		cUIGfx * Icon() const;

		void Text( const std::wstring& text );

		void Text( const std::string& text );

		void Padding( const eeRectf& padding );

		const eeRectf& Padding() const;

		void IconHorizontalMargin( Int32 margin );

		const Int32& IconHorizontalMargin() const;

		cUITextBox * TextBox() const;
	protected:
		cUIGfx * 		mIcon;
		cUITextBox * 	mTextBox;
		Int32			mIconSpace;

		virtual void OnSizeChange();
};

}}

#endif

