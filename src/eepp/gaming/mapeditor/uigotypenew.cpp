#include <eepp/gaming/mapeditor/uigotypenew.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace Gaming { namespace Private {

UIGOTypeNew::UIGOTypeNew( cb::Callback2<void, std::string, Uint32> Cb ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mUIInput( NULL ),
	mCb( Cb )
{
	mUITheme		= UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, Sizei( 278, 114 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 278, 114 ) );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIGOTypeNew::WindowClose ) );
	mUIWindow->Title( "Add GameObject Type" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mUITheme->CreateTextBox( "GameObject Type Name", mUIWindow->Container(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIInput = mUITheme->CreateTextInput( mUIWindow->Container(), Sizei( 120, 22 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );

	UIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "add" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIGOTypeNew::OKClick ) );
	mUIInput->AddEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UIGOTypeNew::OKClick ) );

	OKButton->Text( "Add" );

	UIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIGOTypeNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->Center();
	mUIWindow->Show();

	mUIInput->SetFocus();
}

UIGOTypeNew::~UIGOTypeNew() {

}

void UIGOTypeNew::OKClick( const UIEvent * Event ) {
	if ( mUIInput->Text().size() ) {
		if ( mCb.IsSet() )
			mCb( mUIInput->Text().ToUtf8(), String::Hash( mUIInput->Text().ToUtf8() ) );
	}

	mUIWindow->CloseWindow();
}

void UIGOTypeNew::CancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void UIGOTypeNew::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

}}}
