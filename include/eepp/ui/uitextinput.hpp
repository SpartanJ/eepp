#ifndef EE_UICUITEXTINPUT_H
#define EE_UICUITEXTINPUT_H

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/window/inputtextbuffer.hpp>

namespace EE { namespace UI {

class EE_API UITextInput : public UITextBox {
	public:
		class CreateParams : public UITextBox::CreateParams {
			public:
				inline CreateParams() :
					UITextBox::CreateParams(),
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

		UITextInput( const UITextInput::CreateParams& Params );

		virtual ~UITextInput();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void update();

		virtual void draw();

		void pushIgnoredChar( const Uint32& ch );

		virtual void setTheme( UITheme * Theme );

		InputTextBuffer * getInputTextBuffer();

		void allowEditing( const bool& allow );

		const bool& allowEditing() const;

		virtual const String& text();

		virtual void text( const String& text );

		virtual void shrinkText( const Uint32& MaxWidth );
	protected:
		InputTextBuffer	mTextBuffer;
		Float				mWaitCursorTime;
		Vector2f			mCurPos;
		int				mCursorPos;
		bool				mAllowEditing;
		bool				mShowingWait;

		void resetWaitCursor();

		virtual void alignFix();

		virtual void autoSize();

		void privOnPressEnter();

		void autoPadding();

		virtual Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onFocus();

		virtual Uint32 onFocusLoss();

		virtual Uint32 onPressEnter();

		virtual void onCursorPosChange();

		void drawWaitingCursor();

		virtual void updateText();

		virtual void selCurInit( const Int32& init );

		virtual void selCurEnd( const Int32& end );

		virtual Int32 selCurInit();

		virtual Int32 selCurEnd();
};

}}

#endif
