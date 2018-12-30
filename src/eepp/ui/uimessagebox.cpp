#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

UIMessageBox * UIMessageBox::New(UI_MSGBOX_TYPE type, String message) {
	return eeNew( UIMessageBox, ( type, message ) );
}

UIMessageBox::UIMessageBox( UI_MSGBOX_TYPE type , String message ) :
	UIWindow(),
	mMsgBoxType( type ),
	mCloseWithKey( KEY_UNKNOWN )
{
	mStyleConfig.WinFlags &= ~UI_WIN_RESIZEABLE;

	updateWinFlags();

	UILinearLayout * rlay = UILinearLayout::New();
	rlay->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT )->setParent( mContainer );

	UILinearLayout * vlay = UILinearLayout::NewVertical();
	vlay->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT )
		->setLayoutMargin( Rect( 8, 8, 8, 8) )->setParent( rlay );

	mTextBox = UITextView::New();
	mTextBox->setText( message )
			->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT )
			->setParent( vlay );

	UILinearLayout * hlay = UILinearLayout::NewHorizontal();
	hlay->setLayoutMargin( Rect( 0, 8, 0, 0 ) )
		->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT )
		->setLayoutGravity( UI_HALIGN_RIGHT | UI_VALIGN_CENTER )
		->setParent( vlay );

	mButtonOK = UIPushButton::New();
	mButtonOK->setSize( 90, 0 )->setParent( hlay );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setLayoutMargin( Rect( 8, 0, 0, 0 ) )->setSize( 90, 0 )->setParent( hlay );

	switch ( mMsgBoxType ) {
		case MSGBOX_OKCANCEL:
		{
			mButtonOK->setText( "OK" );
			mButtonCancel->setText( "Cancel" );
			break;
		}
		case MSGBOX_YESNO:
		{
			mButtonOK->setText( "Yes" );
			mButtonCancel->setText( "No" );
			break;
		}
		case MSGBOX_RETRYCANCEL:
		{
			mButtonOK->setText( "Retry" );
			mButtonCancel->setText( "Cancel" );
			break;
		}
		case MSGBOX_OK:
		{
			mButtonOK->setText( "OK" );
			mButtonCancel->setVisible( false );
			mButtonCancel->setEnabled( false );
			break;
		}
	}

	applyDefaultTheme();

	setMinWindowSize( rlay->getSize() );
}

UIMessageBox::~UIMessageBox() {
}

void UIMessageBox::setTheme( UITheme * Theme ) {
	UIWindow::setTheme( Theme );

	mTextBox->setTheme( Theme );
	mButtonOK->setTheme( Theme );
	mButtonCancel->setTheme( Theme );

	if ( "Retry" != mButtonOK->getText() ) {
		Drawable * OKIcon = Theme->getIconByName( "ok" );
		Drawable * CancelIcon = Theme->getIconByName( "cancel" );

		if ( NULL != OKIcon ) {
			mButtonOK->setIcon( OKIcon );
		}

		if ( NULL != CancelIcon ) {
			mButtonCancel->setIcon( CancelIcon );
		}
	}

	onThemeLoaded();
}

Uint32 UIMessageBox::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mButtonOK ) {
					sendCommonEvent( Event::MsgBoxConfirmClick );

					closeWindow();
				} else if ( Msg->getSender() == mButtonCancel ) {
					closeWindow();
				}
			}

			break;
		}
	}

	return UIWindow::onMessage( Msg );
}

UITextView * UIMessageBox::getTextBox() const {
	return mTextBox;
}

UIPushButton *	UIMessageBox::getButtonOK() const {
	return mButtonOK;
}

UIPushButton * UIMessageBox::getButtonCancel() const {
	return mButtonCancel;
}

Uint32 UIMessageBox::onKeyUp( const KeyEvent & Event ) {
	if ( mCloseWithKey && Event.getKeyCode() == mCloseWithKey ) {
		closeWindow();
	}

	return 1;
}

bool UIMessageBox::show() {
	bool b = UIWindow::show();

	mButtonOK->setFocus();

	return b;
}

Uint32 UIMessageBox::getCloseWithKey() const {
	return mCloseWithKey;
}

void UIMessageBox::setCloseWithKey(const Uint32 & closeWithKey) {
	mCloseWithKey = closeWithKey;
}

}}
