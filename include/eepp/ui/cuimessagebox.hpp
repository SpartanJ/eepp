#ifndef EE_UICUIMESSAGEBOX_HPP
#define EE_UICUIMESSAGEBOX_HPP

#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/cuipushbutton.hpp>

namespace EE { namespace UI {

class EE_API cUIMessageBox : public cUIWindow {
	public:
		class CreateParams : public cUIWindow::CreateParams {
			public:
				inline CreateParams() :
					cUIWindow::CreateParams(),
					Type( MSGBOX_OKCANCEL )
				{
				}

				inline ~CreateParams() {}

				UI_MSGBOX_TYPE	Type;
				String			Message;
		};

		cUIMessageBox( const cUIMessageBox::CreateParams& Params );

		virtual ~cUIMessageBox();

		virtual Uint32		OnMessage( const cUIMessage * Msg );

		virtual void		SetTheme( cUITheme * Theme );

		cUITextBox *		TextBox() const;

		cUIPushButton *		ButtonOK() const;

		cUIPushButton *		ButtonCancel() const;

		virtual bool		Show();
	protected:
		UI_MSGBOX_TYPE		mMsgBoxType;
		cUITextBox *		mTextBox;
		cUIPushButton *		mButtonOK;
		cUIPushButton *		mButtonCancel;

		void				AutoSize();
};

}}

#endif
