#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMessageBox::UIMessageBox( const UIMessageBox::CreateParams& Params ) :
	UIWindow( Params ),
	mMsgBoxType( Params.Type ),
	mCloseWithKey( Params.CloseWithKey )
{
	UITheme * Theme = UIThemeManager::instance()->defaultTheme();

	if ( NULL == Theme )
	{
		UIPushButton::CreateParams ButtonParams;
		ButtonParams.Parent( getContainer() );
		ButtonParams.SizeSet( 90, 22 );
		ButtonParams.PosSet( getContainer()->size().width() - 96, getContainer()->size().height() - ButtonParams.Size.height() - 8 );
		ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
		mButtonOK = eeNew( UIPushButton, ( ButtonParams ) );
		mButtonOK->visible( true );
		mButtonOK->enabled( true );

		ButtonParams.Pos.x = mButtonOK->position().x - mButtonOK->size().width() - 8;
		mButtonCancel = eeNew( UIPushButton, ( ButtonParams ) );
		mButtonCancel->visible( true );
		mButtonCancel->enabled( true );
	}
	else
	{
		mButtonOK = Theme->createPushButton( getContainer(),
								 Sizei( 90, 22 ),
								 Vector2i( getContainer()->size().width() - 96, getContainer()->size().height() - 22 - 8 ),
								 UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE );

		mButtonCancel = Theme->createPushButton( getContainer(),
								 mButtonOK->size(),
								 Vector2i( mButtonOK->position().x - mButtonOK->size().width() - 8, mButtonOK->position().y ),
								 mButtonOK->flags() );
	}

	UITextBox::CreateParams TxtParams;
	TxtParams.Parent( getContainer() );
	TxtParams.Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM;
	TxtParams.SizeSet( getContainer()->size().width(), mButtonOK->position().y  );

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->visible( true );
	mTextBox->enabled( true );
	mTextBox->text( Params.Message );

	switch ( mMsgBoxType ) {
		case MSGBOX_OKCANCEL:
		{
			mButtonOK->text( "OK" );
			mButtonCancel->text( "Cancel" );
			break;
		}
		case MSGBOX_YESNO:
		{
			mButtonOK->text( "Yes" );
			mButtonCancel->text( "No" );
			break;
		}
		case MSGBOX_RETRYCANCEL:
		{
			mButtonOK->text( "Retry" );
			mButtonCancel->text( "Cancel" );
			break;
		}
		case MSGBOX_OK:
		{
			mButtonOK->text( "OK" );
			mButtonCancel->visible( false );
			mButtonCancel->enabled( false );
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

	if ( "Retry" != mButtonOK->text() ) {
		SubTexture * OKIcon = Theme->getIconByName( "ok" );
		SubTexture * CancelIcon = Theme->getIconByName( "cancel" );

		if ( NULL != OKIcon ) {
			mButtonOK->icon( OKIcon );
		}

		if ( NULL != CancelIcon ) {
			mButtonCancel->icon( CancelIcon );
		}
	}

	mButtonOK->position( mButtonOK->position().x, getContainer()->size().height() - mButtonOK->size().height() - 8 );
	mButtonCancel->position( mButtonCancel->position().x, mButtonOK->position().y );
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

					CloseWindow();
				} else if ( Msg->getSender() == mButtonCancel ) {
					CloseWindow();
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
	Sizei nSize( mTextBox->getTextWidth() + 48, mTextBox->getTextHeight() + mButtonOK->size().height() + mDecoSize.height() + 8 );

	if ( !( nSize.width() > getContainer()->size().width() ) ) {
		nSize.x = getContainer()->size().width();
	}

	if ( !( nSize.height() > getContainer()->size().height() ) ) {
		nSize.y = getContainer()->size().height();
	}

	if ( nSize.x != getContainer()->size().width() || nSize.y != getContainer()->size().height() ) {
		size( nSize );

		mMinWindowSize = nSize;
	}
}

Uint32 UIMessageBox::onKeyUp( const UIEventKey & Event ) {
	if ( mCloseWithKey && Event.getKeyCode() == mCloseWithKey ) {
		CloseWindow();
	}

	return 1;
}

bool UIMessageBox::show() {
	bool b = UIWindow::show();

	mButtonOK->setFocus();

	return b;
}

}}
