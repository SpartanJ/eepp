#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMessageBox * UIMessageBox::New(UI_MSGBOX_TYPE type, String message) {
	return eeNew( UIMessageBox, ( type, message ) );
}

UIMessageBox::UIMessageBox( UI_MSGBOX_TYPE type , String message ) :
	UIWindow(),
	mMsgBoxType( type ),
	mCloseWithKey( KEY_UNKNOWN )
{
	mButtonOK = UIPushButton::New();
	mButtonOK->setParent( getContainer() )
			 ->setFlags( UI_AUTO_SIZE )
			 ->setSize( 90, 0 )
			 ->setPosition( getContainer()->getSize().getWidth() - 96, getContainer()->getSize().getHeight() - mButtonOK->getSize().getHeight() - 8 );
	mButtonOK->setAnchors( UI_ANCHOR_RIGHT );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setParent( getContainer() )
			 ->setFlags( UI_AUTO_SIZE )
			 ->setSize( 90, 0 )
			 ->setPosition( mButtonOK->getPosition().x - mButtonOK->getSize().getWidth() - 8, getContainer()->getSize().getHeight() - mButtonOK->getSize().getHeight() - 8 );
	mButtonCancel->setAnchors( UI_ANCHOR_RIGHT );

	applyDefaultTheme();

	mTextBox = UITextBox::New();
	mTextBox->setParent( getContainer() )
			->setSize( getContainer()->getSize().getWidth(), mButtonOK->getPosition().y )
			->setHorizontalAlign( UI_HALIGN_CENTER );
	mTextBox->setText( message );

	setSize( getContainer()->getSize().getWidth(), mTextBox->getTextHeight() + mButtonOK->getSize().getHeight() + 8 );

	mTextBox->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );

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

	onAutoSize();
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

void UIMessageBox::onAutoSize() {
	Sizei nSize( PixelDensity::pxToDpI( mTextBox->getTextWidth() ) + 48, mTextBox->getTextHeight() + mButtonOK->getSize().getHeight() + mStyleConfig.DecorationSize.getHeight() + 8 );

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

Uint32 UIMessageBox::getCloseWithKey() const
{
	return mCloseWithKey;
}

void UIMessageBox::setCloseWithKey(const Uint32 & closeWithKey)
{
	mCloseWithKey = closeWithKey;
}

}}
