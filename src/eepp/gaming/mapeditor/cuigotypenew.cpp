#include <eepp/gaming/mapeditor/cuigotypenew.hpp>
#include <eepp/ui/cuitextinput.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cUIGOTypeNew::cUIGOTypeNew( cb::Callback2<void, std::string, Uint32> Cb ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mUIInput( NULL ),
	mCb( Cb )
{
	mUITheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, eeSize( 278, 114 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, eeSize( 278, 114 ) );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cUIGOTypeNew::WindowClose ) );
	mUIWindow->Title( "Add GameObject Type" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mUITheme->CreateTextBox( "GameObject Type Name", mUIWindow->Container(), eeSize(), eeVector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIInput = mUITheme->CreateTextInput( mUIWindow->Container(), eeSize( 120, 22 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );

	cUIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize( 80, 22 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "add" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIGOTypeNew::OKClick ) );
	mUIInput->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cUIGOTypeNew::OKClick ) );

	OKButton->Text( "Add" );

	cUIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), eeVector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIGOTypeNew::CancelClick ) );
	CancelButton->Text( "Cancel" );

	mUIWindow->Center();
	mUIWindow->Show();

	mUIInput->SetFocus();
}

cUIGOTypeNew::~cUIGOTypeNew() {

}

void cUIGOTypeNew::OKClick( const cUIEvent * Event ) {
	if ( mUIInput->Text().size() ) {
		if ( mCb.IsSet() )
			mCb( mUIInput->Text().ToUtf8(), String::Hash( mUIInput->Text().ToUtf8() ) );
	}

	mUIWindow->CloseWindow();
}

void cUIGOTypeNew::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cUIGOTypeNew::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

}}}
