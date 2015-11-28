#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace EE { namespace UI {

class EE_API UITextEdit : public UIComplexControl {
	public:			
		class CreateParams : public UITextBox::CreateParams {
			public:
				inline CreateParams() : UITextBox::CreateParams(),
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

		UITextEdit( UITextEdit::CreateParams& Params );

		virtual ~UITextEdit();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		const String& Text() const;

		void Text( const String& Txt );

		UITextInput * TextInput() const;

		UIScrollBar * HScrollBar() const;

		UIScrollBar * VScrollBar() const;

		virtual void Update();

		void AllowEditing( const bool& allow );

		const bool& AllowEditing() const;
	protected:
		UITextInput *		mTextInput;
		UIScrollBar *		mHScrollBar;
		UIScrollBar *		mVScrollBar;
		UI_SCROLLBAR_MODE	mHScrollBarMode;
		UI_SCROLLBAR_MODE	mVScrollBarMode;
		Recti				mPadding;
		String				mText;
		bool				mSkipValueChange;

		virtual void OnSizeChange();

		virtual void OnAlphaChange();

		virtual void OnParentSizeChange( const Vector2i& SizeChange );

		void OnVScrollValueChange( const UIEvent * Event );

		void OnHScrollValueChange( const UIEvent * Event );

		void OnInputSizeChange( const UIEvent * Event = NULL );

		void OnCursorPosChange( const UIEvent * Event );

		void AutoPadding();

		void ScrollbarsSet();

		void FixScroll();

		void FixScrollToCursor();

		void ShrinkText( const Uint32& Width );
};

}}

#endif
