#ifndef EE_UICUITEXTINPUT_H
#define EE_UICUITEXTINPUT_H

#include "cuicontrolanim.hpp"
#include "cuitextbox.hpp"

namespace EE { namespace UI {

class EE_API cUITextInput : public cUITextBox {
	public:
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() :
					cUITextBox::CreateParams(),
					SupportFreeEditing( true ),
					MaxLenght( 256 )
				{
				}

				inline ~CreateParams() {}

				bool SupportFreeEditing;
				Uint32 MaxLenght;
		};

		cUITextInput( const cUITextInput::CreateParams& Params );

		~cUITextInput();

		virtual void Update();

		virtual void Draw();

		virtual Uint32 OnFocus();

		virtual Uint32 OnFocusLoss();

		virtual Uint32 OnPressEnter();

		virtual void Text( const std::wstring& text );

		virtual void Text( const std::string& text );

		virtual const std::wstring& Text();

		void PushIgnoredChar( const Uint32& ch );

		virtual void SetTheme( cUITheme * Theme );

		cInputTextBuffer * GetInputTextBuffer();
	protected:
		cInputTextBuffer mTextBuffer;
		bool mShowingWait;
		eeFloat mWaitCursorTime;
		eeVector2f mCurPos;
		eeInt mCursorPos;

		void ResetWaitCursor();
		void AlignFix();
		void PrivOnPressEnter();
};

}}

#endif
