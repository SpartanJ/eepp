#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMessageBox::UIMessageBox( const UIMessageBox::CreateParams& Params ) :
	UIWindow( Params ),
	mMsgBoxType( Params.Type ),
	mCloseWithKey( Params.CloseWithKey )
{
	UITheme * Theme = UIThemeManager::instance()->DefaultTheme();

	if ( NULL == Theme )
	{
		UIPushButton::CreateParams ButtonParams;
		ButtonParams.Parent( Container() );
		ButtonParams.SizeSet( 90, 22 );
		ButtonParams.PosSet( Container()->Size().Width() - 96, Container()->Size().Height() - ButtonParams.Size.Height() - 8 );
		ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
		mButtonOK = eeNew( UIPushButton, ( ButtonParams ) );
		mButtonOK->Visible( true );
		mButtonOK->Enabled( true );

		ButtonParams.Pos.x = mButtonOK->Pos().x - mButtonOK->Size().Width() - 8;
		mButtonCancel = eeNew( UIPushButton, ( ButtonParams ) );
		mButtonCancel->Visible( true );
		mButtonCancel->Enabled( true );
	}
	else
	{
		mButtonOK = Theme->CreatePushButton( Container(),
								 Sizei( 90, 22 ),
								 Vector2i( Container()->Size().Width() - 96, Container()->Size().Height() - 22 - 8 ),
								 UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE );

		mButtonCancel = Theme->CreatePushButton( Container(),
								 mButtonOK->Size(),
								 Vector2i( mButtonOK->Pos().x - mButtonOK->Size().Width() - 8, mButtonOK->Pos().y ),
								 mButtonOK->Flags() );
	}

	UITextBox::CreateParams TxtParams;
	TxtParams.Parent( Container() );
	TxtParams.Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM;
	TxtParams.SizeSet( Container()->Size().Width(), mButtonOK->Pos().y  );

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->Visible( true );
	mTextBox->Enabled( true );
	mTextBox->Text( Params.Message );

	switch ( mMsgBoxType ) {
		case MSGBOX_OKCANCEL:
		{
			mButtonOK->Text( "OK" );
			mButtonCancel->Text( "Cancel" );
			break;
		}
		case MSGBOX_YESNO:
		{
			mButtonOK->Text( "Yes" );
			mButtonCancel->Text( "No" );
			break;
		}
		case MSGBOX_RETRYCANCEL:
		{
			mButtonOK->Text( "Retry" );
			mButtonCancel->Text( "Cancel" );
			break;
		}
		case MSGBOX_OK:
		{
			mButtonOK->Text( "OK" );
			mButtonCancel->Visible( false );
			mButtonCancel->Enabled( false );
			break;
		}
	}

	mButtonCancel->ToFront();
	mButtonOK->ToFront();

	AutoSize();

	ApplyDefaultTheme();
}

UIMessageBox::~UIMessageBox() {
}

void UIMessageBox::SetTheme( UITheme * Theme ) {
	UIWindow::SetTheme( Theme );

	if ( "Retry" != mButtonOK->Text() ) {
		SubTexture * OKIcon = Theme->GetIconByName( "ok" );
		SubTexture * CancelIcon = Theme->GetIconByName( "cancel" );

		if ( NULL != OKIcon ) {
			mButtonOK->Icon( OKIcon );
		}

		if ( NULL != CancelIcon ) {
			mButtonCancel->Icon( CancelIcon );
		}
	}

	mButtonOK->Pos( mButtonOK->Pos().x, Container()->Size().Height() - mButtonOK->Size().Height() - 8 );
	mButtonCancel->Pos( mButtonCancel->Pos().x, mButtonOK->Pos().y );
}

Uint32 UIMessageBox::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				Vector2i mousei( UIManager::instance()->GetMousePos() );
				Vector2f mouse( mousei.x, mousei.y );

				if ( Msg->Sender() == mButtonOK && mButtonOK->GetPolygon().PointInside( mouse ) ) {
					SendCommonEvent( UIEvent::EventMsgBoxConfirmClick );

					CloseWindow();
				} else if ( Msg->Sender() == mButtonCancel ) {
					CloseWindow();
				}
			}

			break;
		}
	}

	return UIWindow::OnMessage( Msg );
}

UITextBox * UIMessageBox::TextBox() const {
	return mTextBox;
}

UIPushButton *	UIMessageBox::ButtonOK() const {
	return mButtonOK;
}

UIPushButton * UIMessageBox::ButtonCancel() const {
	return mButtonCancel;
}

void UIMessageBox::AutoSize() {
	Sizei nSize( mTextBox->GetTextWidth() + 48, mTextBox->GetTextHeight() + mButtonOK->Size().Height() + mDecoSize.Height() + 8 );

	if ( !( nSize.Width() > Container()->Size().Width() ) ) {
		nSize.x = Container()->Size().Width();
	}

	if ( !( nSize.Height() > Container()->Size().Height() ) ) {
		nSize.y = Container()->Size().Height();
	}

	if ( nSize.x != Container()->Size().Width() || nSize.y != Container()->Size().Height() ) {
		Size( nSize );

		mMinWindowSize = nSize;
	}
}

Uint32 UIMessageBox::OnKeyUp( const UIEventKey & Event ) {
	if ( mCloseWithKey && Event.KeyCode() == mCloseWithKey ) {
		CloseWindow();
	}

	return 1;
}

bool UIMessageBox::Show() {
	bool b = UIWindow::Show();

	mButtonOK->SetFocus();

	return b;
}

}}
