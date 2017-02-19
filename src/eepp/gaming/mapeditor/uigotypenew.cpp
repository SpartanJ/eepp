#include <eepp/gaming/mapeditor/uigotypenew.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace Gaming { namespace Private {

UIGOTypeNew::UIGOTypeNew( cb::Callback2<void, std::string, Uint32> Cb ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mUIInput( NULL ),
	mCb( Cb )
{
	mUITheme		= UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->createWindow( NULL, Sizei( 278, 114 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 278, 114 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIGOTypeNew::onWindowClose ) );
	mUIWindow->setTitle( "Add GameObject Type" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mUITheme->createTextBox( "GameObject Type Name", mUIWindow->getContainer(), Sizei(), Vector2i( 16, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );

	mUIInput = mUITheme->createTextInput( mUIWindow->getContainer(), Sizei( 120, 22 ), Vector2i( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );

	UIPushButton * OKButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "add" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIGOTypeNew::onOKClick ) );
	mUIInput->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UIGOTypeNew::onOKClick ) );

	OKButton->setText( "Add" );

	UIPushButton * CancelButton = mUITheme->createPushButton( mUIWindow->getContainer(), OKButton->getSize(), Vector2i( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIGOTypeNew::onCancelClick ) );
	CancelButton->setText( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();

	mUIInput->setFocus();
}

UIGOTypeNew::~UIGOTypeNew() {

}

void UIGOTypeNew::onOKClick( const UIEvent * Event ) {
	if ( mUIInput->getText().size() ) {
		if ( mCb.IsSet() )
			mCb( mUIInput->getText().toUtf8(), String::hash( mUIInput->getText().toUtf8() ) );
	}

	mUIWindow->CloseWindow();
}

void UIGOTypeNew::onCancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void UIGOTypeNew::onWindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

}}}
