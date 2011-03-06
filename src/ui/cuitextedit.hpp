#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include "cuicontrolanim.hpp"
#include "cuitextinput.hpp"
#include "cuiscrollbar.hpp"

namespace EE { namespace UI {

class EE_API cUITextEdit : public cUIControlAnim {
	public:			
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() : cUITextBox::CreateParams(),
					HScrollBar( UI_SCROLLBAR_AUTO ),
					VScrollBar( UI_SCROLLBAR_AUTO ),
					WordWrap( true )
				{
				}

				inline ~CreateParams() {}

				UI_SCROLLBAR_MODE	HScrollBar;
				UI_SCROLLBAR_MODE	VScrollBar;
				bool				WordWrap;
		};

		cUITextEdit( cUITextEdit::CreateParams& Params );

		~cUITextEdit();

		virtual void SetTheme( cUITheme * Theme );

		const String& Text() const;

		void Text( const String& Txt );

		cUITextInput * TextInput() const;

		cUIScrollBar * HScrollBar() const;

		cUIScrollBar * VScrollBar() const;

		virtual void Update();

		void AllowEditing( const bool& allow );

		const bool& AllowEditing() const;
	protected:
		cUITextInput *		mTextInput;
		cUIScrollBar *		mHScrollBar;
		cUIScrollBar *		mVScrollBar;
		UI_SCROLLBAR_MODE	mHScrollBarMode;
		UI_SCROLLBAR_MODE	mVScrollBarMode;
		eeRecti				mPadding;
		String		mText;
		bool				mSkipValueChange;

		virtual void OnSizeChange();

		void OnVScrollValueChange( const cUIEvent * Event );

		void OnHScrollValueChange( const cUIEvent * Event );

		void OnInputSizeChange( const cUIEvent * Event = NULL );

		void OnCursorPosChange( const cUIEvent * Event );

		void AutoPadding();

		void ScrollbarsSet();

		void FixScroll();

		void FixScrollToCursor();

		void ShrinkText( const Uint32& Width );
};

}}

#endif
