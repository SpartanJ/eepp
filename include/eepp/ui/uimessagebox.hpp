#ifndef EE_UICUIMESSAGEBOX_HPP
#define EE_UICUIMESSAGEBOX_HPP

#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIMessageBox : public UIWindow {
	public:
		static UIMessageBox * New( UI_MSGBOX_TYPE type, String message );

		UIMessageBox( UI_MSGBOX_TYPE type, String message );

		virtual ~UIMessageBox();

		virtual Uint32		onMessage( const NodeMessage * Msg );

		virtual void		setTheme( UITheme * Theme );

		UITextView *			getTextBox() const;

		UIPushButton *		getButtonOK() const;

		UIPushButton *		getButtonCancel() const;

		virtual bool		show();

		Uint32 getCloseWithKey() const;

		void setCloseWithKey(const Uint32 & closeWithKey);
	protected:
		UI_MSGBOX_TYPE		mMsgBoxType;
		UITextView *		mTextBox;
		UIPushButton *		mButtonOK;
		UIPushButton *		mButtonCancel;
		Uint32				mCloseWithKey;

		virtual Uint32 onKeyUp( const KeyEvent& Event );
};

}}

#endif
