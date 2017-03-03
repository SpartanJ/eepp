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

	mUIWindow	= UIWindow::New();
	mUIWindow->setSizeWithDecoration( 278, 114 )->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )->setMinWindowSize( 278, 114 );

	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &UIGOTypeNew::onWindowClose ) );
	mUIWindow->setTitle( "Add GameObject Type" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = UITextBox::New();
	Txt->setFlags( UI_DRAW_SHADOW | UI_AUTO_SIZE )->setParent( mUIWindow->getContainer() )->setPosition( 16, InitialY );
	Txt->setText( "GameObject Type Name" );

	mUIInput = UITextInput::New()->setMaxLength( 64 );
	mUIInput->setParent( mUIWindow->getContainer() )->setSize( 120, 0 )->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );

	UIPushButton * OKButton = UIPushButton::New();
	OKButton->setParent(  mUIWindow->getContainer() )->setSize( 80, 0 );
	OKButton->setIcon( mUITheme->getIconByName( "add" ) );
	OKButton->setPosition( mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4, mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIGOTypeNew::onOKClick ) );
	mUIInput->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &UIGOTypeNew::onOKClick ) );

	OKButton->setText( "Add" );

	UIPushButton * CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )->setSize( OKButton->getSize() )->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4, OKButton->getPosition().y );
	CancelButton->setIcon( mUITheme->getIconByName( "cancel" ) );
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

	mUIWindow->closeWindow();
}

void UIGOTypeNew::onCancelClick( const UIEvent * Event ) {
	mUIWindow->closeWindow();
}

void UIGOTypeNew::onWindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

}}}
