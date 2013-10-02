#ifndef EE_UICUITEXTINPUT_H
#define EE_UICUITEXTINPUT_H

#include <eepp/ui/cuicontrolanim.hpp>
#include <eepp/ui/cuitextbox.hpp>
#include <eepp/window/cinputtextbuffer.hpp>

namespace EE { namespace UI {

class EE_API cUITextInput : public cUITextBox {
	public:
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() :
					cUITextBox::CreateParams(),
					SupportFreeEditing( true ),
					MaxLength( 256 ),
					PassInput( false )
				{
				}

				inline ~CreateParams() {}

				bool SupportFreeEditing;
				Uint32 MaxLength;
				bool PassInput;
		};

		cUITextInput( const cUITextInput::CreateParams& Params );

		virtual ~cUITextInput();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Update();

		virtual void Draw();

		void PushIgnoredChar( const Uint32& ch );

		virtual void SetTheme( cUITheme * Theme );

		cInputTextBuffer * GetInputTextBuffer();

		void AllowEditing( const bool& allow );

		const bool& AllowEditing() const;

		virtual const String& Text();

		virtual void Text( const String& text );

		virtual void ShrinkText( const Uint32& MaxWidth );

		const bool& PasswordInput() const;

		void PasswordInput( bool pass );
	protected:
		cInputTextBuffer	mTextBuffer;
		eeFloat				mWaitCursorTime;
		eeVector2f			mCurPos;
		eeInt				mCursorPos;
		bool				mAllowEditing;
		bool				mShowingWait;
		bool				mPassInput;

		void ResetWaitCursor();

		void AlignFix();

		virtual void AutoSize();

		void PrivOnPressEnter();

		void AutoPadding();

		virtual Uint32 OnFocus();

		virtual Uint32 OnFocusLoss();

		virtual Uint32 OnPressEnter();

		virtual void OnCursorPosChange();
};

}}

#endif
