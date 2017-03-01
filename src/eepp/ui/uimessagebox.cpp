#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMessageBox::UIMessageBox( const UIMessageBox::CreateParams& Params ) :
	UIWindow( Params ),
	mMsgBoxType( Params.Type ),
	mCloseWithKey( Params.CloseWithKey )
{
	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == Theme )
	{
		UIPushButton::CreateParams ButtonParams;
		ButtonParams.setParent( getContainer() );
		ButtonParams.setSize( 90, 22 );
		ButtonParams.setPosition( getContainer()->getSize().getWidth() - 96, getContainer()->getSize().getHeight() - ButtonParams.Size.getHeight() - 8 );
		ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
		mButtonOK = eeNew( UIPushButton, ( ButtonParams ) );
		mButtonOK->setVisible( true );
		mButtonOK->setEnabled( true );

		ButtonParams.Pos.x = mButtonOK->getPosition().x - mButtonOK->getSize().getWidth() - 8;
		mButtonCancel = eeNew( UIPushButton, ( ButtonParams ) );
		mButtonCancel->setVisible( true );
		mButtonCancel->setEnabled( true );
	}
	else
	{
		mButtonOK = Theme->createPushButton( getContainer(),
								 Sizei( 90, 22 ),
								 Vector2i( getContainer()->getSize().getWidth() - 96, getContainer()->getSize().getHeight() - 22 - 8 ),
								 UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE );

		mButtonCancel = Theme->createPushButton( getContainer(),
								 mButtonOK->getSize(),
								 Vector2i( mButtonOK->getPosition().x - mButtonOK->getSize().getWidth() - 8, mButtonOK->getPosition().y ),
								 mButtonOK->getFlags() );
	}

	UITextBox::CreateParams TxtParams;
	TxtParams.setParent( getContainer() );
	TxtParams.Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM;
	TxtParams.setSize( getContainer()->getSize().getWidth(), mButtonOK->getPosition().y  );

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( true );
	mTextBox->setText( Params.Message );

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

	mButtonCancel->toFront();
	mButtonOK->toFront();

	autoSize();

	applyDefaultTheme();
}

UIMessageBox::~UIMessageBox() {
}

void UIMessageBox::setTheme( UITheme * Theme ) {
	UIWindow::setTheme( Theme );

	if ( "Retry" != mButtonOK->getText() ) {
		SubTexture * OKIcon = Theme->getIconByName( "ok" );
		SubTexture * CancelIcon = Theme->getIconByName( "cancel" );

		if ( NULL != OKIcon ) {
			mButtonOK->setIcon( OKIcon );
		}

		if ( NULL != CancelIcon ) {
			mButtonCancel->setIcon( CancelIcon );
		}
	}

	mButtonOK->setPosition( mButtonOK->getPosition().x, getContainer()->getSize().getHeight() - mButtonOK->getSize().getHeight() - 8 );
	mButtonCancel->setPosition( mButtonCancel->getPosition().x, mButtonOK->getPosition().y );
}

Uint32 UIMessageBox::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				Vector2i mousei( UIManager::instance()->getMousePos() );
				Vector2f mouse( mousei.x, mousei.y );

				if ( Msg->getSender() == mButtonOK && mButtonOK->getPolygon().pointInside( mouse ) ) {
					sendCommonEvent( UIEvent::EventMsgBoxConfirmClick );

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

UITextBox * UIMessageBox::getTextBox() const {
	return mTextBox;
}

UIPushButton *	UIMessageBox::getButtonOK() const {
	return mButtonOK;
}

UIPushButton * UIMessageBox::getButtonCancel() const {
	return mButtonCancel;
}

void UIMessageBox::autoSize() {
	Sizei nSize( mTextBox->getTextWidth() + 48, mTextBox->getTextHeight() + mButtonOK->getSize().getHeight() + mStyleConfig.DecorationSize.getHeight() + 8 );

	if ( !( nSize.getWidth() > getContainer()->getSize().getWidth() ) ) {
		nSize.x = getContainer()->getSize().getWidth();
	}

	if ( !( nSize.getHeight() > getContainer()->getSize().getHeight() ) ) {
		nSize.y = getContainer()->getSize().getHeight();
	}

	if ( nSize.x != getContainer()->getSize().getWidth() || nSize.y != getContainer()->getSize().getHeight() ) {
		setSize( nSize );

		 mStyleConfig.MinWindowSize = nSize;
	}
}

Uint32 UIMessageBox::onKeyUp( const UIEventKey & Event ) {
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

}}
