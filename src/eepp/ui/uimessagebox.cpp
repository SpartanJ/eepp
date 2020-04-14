#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

UIMessageBox* UIMessageBox::New( const Type& type, const String& message,
								 const Uint32& windowFlags ) {
	return eeNew( UIMessageBox, ( type, message, windowFlags ) );
}

UIMessageBox::UIMessageBox( const Type& type, const String& message, const Uint32& windowFlags ) :
	UIWindow(), mMsgBoxType( type ), mCloseWithKey( KEY_UNKNOWN ) {
	mStyleConfig.WinFlags = windowFlags;

	updateWinFlags();

	mLayoutCont = UILinearLayout::New();
	mLayoutCont->setLayoutSizeRules( LayoutSizeRule::WrapContent, LayoutSizeRule::WrapContent )
		->setParent( mContainer );

	UILinearLayout* vlay = UILinearLayout::NewVertical();
	vlay->setLayoutSizeRules( LayoutSizeRule::WrapContent, LayoutSizeRule::WrapContent )
		->setLayoutMargin( Rect( 8, 8, 8, 8 ) )
		->setParent( mLayoutCont );

	mTextBox = UITextView::New();
	mTextBox->setText( message )
		->setLayoutSizeRules( LayoutSizeRule::WrapContent, LayoutSizeRule::WrapContent )
		->setParent( vlay );

	UILinearLayout* hlay = UILinearLayout::NewHorizontal();
	hlay->setLayoutMargin( Rect( 0, 8, 0, 0 ) )
		->setLayoutSizeRules( LayoutSizeRule::WrapContent, LayoutSizeRule::WrapContent )
		->setLayoutGravity( UI_HALIGN_RIGHT | UI_VALIGN_CENTER )
		->setParent( vlay );

	mButtonOK = UIPushButton::New();
	mButtonOK->setSize( 90, 0 )->setParent( hlay );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setLayoutMargin( Rect( 8, 0, 0, 0 ) )->setSize( 90, 0 )->setParent( hlay );

	switch ( mMsgBoxType ) {
		case UIMessageBox::OK_CANCEL: {
			mButtonOK->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_ok", "Ok" ) );
			mButtonCancel->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_cancel", "Cancel" ) );
			break;
		}
		case UIMessageBox::YES_NO: {
			mButtonOK->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_yes", "Yes" ) );
			mButtonCancel->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_no", "No" ) );
			break;
		}
		case UIMessageBox::RETRY_CANCEL: {
			mButtonOK->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_retry", "Retry" ) );
			mButtonCancel->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_cancel", "Cancel" ) );
			break;
		}
		case UIMessageBox::OK: {
			mButtonOK->setText(
				getUISceneNode()->getTranslatorString( "@string/msg_box_ok", "Ok" ) );
			mButtonCancel->setVisible( false );
			mButtonCancel->setEnabled( false );
			break;
		}
	}

	applyDefaultTheme();
}

UIMessageBox::~UIMessageBox() {}

void UIMessageBox::setTheme( UITheme* Theme ) {
	UIWindow::setTheme( Theme );

	mTextBox->setTheme( Theme );
	mButtonOK->setTheme( Theme );
	mButtonCancel->setTheme( Theme );

	if ( "Retry" != mButtonOK->getText() ) {
		Drawable* OKIcon = Theme->getIconByName( "ok" );
		Drawable* CancelIcon = Theme->getIconByName( "cancel" );

		if ( NULL != OKIcon ) {
			mButtonOK->setIcon( OKIcon );
		}

		if ( NULL != CancelIcon ) {
			mButtonCancel->setIcon( CancelIcon );
		}
	}

	onThemeLoaded();
}

Uint32 UIMessageBox::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click: {
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

UITextView* UIMessageBox::getTextBox() const {
	return mTextBox;
}

UIPushButton* UIMessageBox::getButtonOK() const {
	return mButtonOK;
}

UIPushButton* UIMessageBox::getButtonCancel() const {
	return mButtonCancel;
}

Uint32 UIMessageBox::onKeyUp( const KeyEvent& Event ) {
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

void UIMessageBox::setCloseWithKey( const Uint32& closeWithKey ) {
	mCloseWithKey = closeWithKey;
}

void UIMessageBox::onWindowReady() {
	UIWindow::onWindowReady();
	setMinWindowSize( mLayoutCont->getSize() );
	center();
}

}} // namespace EE::UI
