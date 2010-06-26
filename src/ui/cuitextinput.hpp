#ifndef EE_UICUITEXTINPUT_H
#define EE_UICUITEXTINPUT_H

#include "cuicontrolanim.hpp"
#include "cuitextbox.hpp"

namespace EE { namespace UI {

class EE_API cUITextInput : public cUITextBox {
	public:
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() : cUITextBox::CreateParams() {
					SupportNewLine = true;
					SupportFreeEditing = true;
					MaxLenght = 256;
				}
				
				inline ~CreateParams() {}
				
				bool SupportNewLine;
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
		
		void PushIgnoredChar( const Uint32& ch );
	protected:
		cInputTextBuffer mTextBuffer;
		bool mShowingWait;
		eeFloat mWaitCursorTime;
		eeVector2f mCurPos;
		eeInt mCursorPos;
		
		void ResetWaitCursor();
		void AlignFix();
};

}}

#endif
