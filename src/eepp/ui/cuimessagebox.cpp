#include <eepp/ui/cuimessagebox.hpp>

namespace EE { namespace UI {

cUIMessageBox::cUIMessageBox( const cUIMessageBox::CreateParams& Params ) :
	cUIWindow( Params ),
	mMsgBoxType( Params.Type ),
	mCloseWithKey( Params.CloseWithKey )
{
	cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == Theme )
	{
		cUIPushButton::CreateParams ButtonParams;
		ButtonParams.Parent( Container() );
		ButtonParams.SizeSet( 90, 22 );
		ButtonParams.PosSet( Container()->Size().Width() - 96, Container()->Size().Height() - ButtonParams.Size.Height() - 8 );
		ButtonParams.Flags = UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE;
		mButtonOK = eeNew( cUIPushButton, ( ButtonParams ) );
		mButtonOK->Visible( true );
		mButtonOK->Enabled( true );

		ButtonParams.Pos.x = mButtonOK->Pos().x - mButtonOK->Size().Width() - 8;
		mButtonCancel = eeNew( cUIPushButton, ( ButtonParams ) );
		mButtonCancel->Visible( true );
		mButtonCancel->Enabled( true );
	}
	else
	{
		mButtonOK = Theme->CreatePushButton( Container(),
								 eeSize( 90, 22 ),
								 eeVector2i( Container()->Size().Width() - 96, Container()->Size().Height() - 22 - 8 ),
								 UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_VALIGN_CENTER | UI_AUTO_SIZE );

		mButtonCancel = Theme->CreatePushButton( Container(),
								 mButtonOK->Size(),
								 eeVector2i( mButtonOK->Pos().x - mButtonOK->Size().Width() - 8, mButtonOK->Pos().y ),
								 mButtonOK->Flags() );
	}

	cUITextBox::CreateParams TxtParams;
	TxtParams.Parent( Container() );
	TxtParams.Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM;
	TxtParams.SizeSet( Container()->Size().Width(), mButtonOK->Pos().y  );

	mTextBox = eeNew( cUITextBox, ( TxtParams ) );
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

cUIMessageBox::~cUIMessageBox() {
}

void cUIMessageBox::SetTheme( cUITheme * Theme ) {
	cUIWindow::SetTheme( Theme );

	if ( "Retry" != mButtonOK->Text() ) {
		cSubTexture * OKIcon = Theme->GetIconByName( "ok" );
		cSubTexture * CancelIcon = Theme->GetIconByName( "cancel" );

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

Uint32 cUIMessageBox::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender() == mButtonOK ) {
					SendCommonEvent( cUIEvent::EventMsgBoxConfirmClick );

					CloseWindow();
				} else if ( Msg->Sender() == mButtonCancel ) {
					CloseWindow();
				}
			}

			break;
		}
	}

	return cUIWindow::OnMessage( Msg );
}

cUITextBox * cUIMessageBox::TextBox() const {
	return mTextBox;
}

cUIPushButton *	cUIMessageBox::ButtonOK() const {
	return mButtonOK;
}

cUIPushButton * cUIMessageBox::ButtonCancel() const {
	return mButtonCancel;
}

void cUIMessageBox::AutoSize() {
	eeSize nSize( mTextBox->GetTextWidth() + 48, mTextBox->GetTextHeight() + mButtonOK->Size().Height() + mDecoSize.Height() + 8 );

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

Uint32 cUIMessageBox::OnKeyUp( const cUIEventKey & Event ) {
	if ( mCloseWithKey && Event.KeyCode() == mCloseWithKey ) {
		CloseWindow();
	}

	return 1;
}

bool cUIMessageBox::Show() {
	bool b = cUIWindow::Show();

	mButtonOK->SetFocus();

	return b;
}

}}
