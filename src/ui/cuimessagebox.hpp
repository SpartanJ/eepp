#ifndef EE_UICUIMESSAGEBOX_HPP
#define EE_UICUIMESSAGEBOX_HPP

#include "cuiwindow.hpp"
#include "cuitextbox.hpp"
#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class cUIMessageBox : public cUIWindow {
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
	protected:
		UI_MSGBOX_TYPE		mType;
		cUITextBox *		mTextBox;
		cUIPushButton *		mButtonOK;
		cUIPushButton *		mButtonCancel;

		void				AutoSize();
};

}}

#endif
