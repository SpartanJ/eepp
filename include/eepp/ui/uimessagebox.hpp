#ifndef EE_UICUIMESSAGEBOX_HPP
#define EE_UICUIMESSAGEBOX_HPP

#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIMessageBox : public UIWindow {
	public:
		class CreateParams : public UIWindow::CreateParams {
			public:
				inline CreateParams() :
					UIWindow::CreateParams(),
					Type( MSGBOX_OKCANCEL ),
					CloseWithKey( KEY_UNKNOWN )
				{
				}

				inline ~CreateParams() {}

				UI_MSGBOX_TYPE	Type;
				String			Message;
				Uint32			CloseWithKey;
		};

		UIMessageBox( const UIMessageBox::CreateParams& Params );

		virtual ~UIMessageBox();

		virtual Uint32		OnMessage( const UIMessage * Msg );

		virtual void		SetTheme( UITheme * Theme );

		UITextBox *		TextBox() const;

		UIPushButton *		ButtonOK() const;

		UIPushButton *		ButtonCancel() const;

		virtual bool		Show();
	protected:
		UI_MSGBOX_TYPE		mMsgBoxType;
		UITextBox *		mTextBox;
		UIPushButton *		mButtonOK;
		UIPushButton *		mButtonCancel;
		Uint32				mCloseWithKey;

		void				AutoSize();

		virtual Uint32 OnKeyUp( const UIEventKey& Event );
};

}}

#endif
