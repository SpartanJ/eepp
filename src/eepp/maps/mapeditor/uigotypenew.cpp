#include <eepp/maps/mapeditor/uigotypenew.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace Maps { namespace Private {

UIGOTypeNew::UIGOTypeNew( std::function<void( std::string, Uint32 )> Cb ) :
	mUITheme( NULL ), mUIWindow( NULL ), mUIInput( NULL ), mCb( Cb ) {
	if ( SceneManager::instance()->getUISceneNode() == NULL )
		return;

	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	mUITheme = sceneNode->getUIThemeManager()->getDefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow = UIWindow::New();
	mUIWindow->setSizeWithDecoration( 278, 114 )
		->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL )
		->setMinWindowSize( 278, 114 );

	mUIWindow->addEventListener( Event::OnWindowClose,
								 cb::Make1( this, &UIGOTypeNew::onWindowClose ) );
	mUIWindow->setTitle( "Add GameObject Type" );

	Int32 InitialY = 16;
	Int32 DistFromTitle = 18;

	UITextView* Txt = UITextView::New();
	Txt->setFontStyle( Text::Shadow )
		->setFlags( UI_AUTO_SIZE )
		->setParent( mUIWindow->getContainer() )
		->setPosition( 16, InitialY );
	Txt->setText( "GameObject Type Name" );

	mUIInput = UITextInput::New()->setMaxLength( 64 );
	mUIInput->setSize( 120, 0 )
		->setParent( mUIWindow->getContainer() )
		->setPosition( Txt->getPosition().x + DistFromTitle, Txt->getPosition().y + DistFromTitle );

	UIPushButton* OKButton = UIPushButton::New();
	OKButton->setSize( 80, 0 )->setParent( mUIWindow->getContainer() );
	OKButton->setIcon( sceneNode->findIcon( "add" ) );
	OKButton->setPosition(
		mUIWindow->getContainer()->getSize().getWidth() - OKButton->getSize().getWidth() - 4,
		mUIWindow->getContainer()->getSize().getHeight() - OKButton->getSize().getHeight() - 4 );
	OKButton->addEventListener( Event::MouseClick, cb::Make1( this, &UIGOTypeNew::onOKClick ) );
	mUIInput->addEventListener( Event::OnPressEnter, cb::Make1( this, &UIGOTypeNew::onOKClick ) );

	OKButton->setText( "Add" );

	UIPushButton* CancelButton = UIPushButton::New();
	CancelButton->setParent( mUIWindow->getContainer() )
		->setSize( OKButton->getSize() )
		->setPosition( OKButton->getPosition().x - OKButton->getSize().getWidth() - 4,
					   OKButton->getPosition().y );
	CancelButton->setIcon( sceneNode->findIcon( "cancel" ) );
	CancelButton->addEventListener( Event::MouseClick,
									cb::Make1( this, &UIGOTypeNew::onCancelClick ) );
	CancelButton->setText( "Cancel" );

	mUIWindow->center();
	mUIWindow->show();

	mUIInput->setFocus();
}

UIGOTypeNew::~UIGOTypeNew() {}

void UIGOTypeNew::onOKClick( const Event* ) {
	if ( mUIInput->getText().size() ) {
		if ( mCb )
			mCb( mUIInput->getText().toUtf8(), String::hash( mUIInput->getText().toUtf8() ) );
	}

	mUIWindow->closeWindow();
}

void UIGOTypeNew::onCancelClick( const Event* ) {
	mUIWindow->closeWindow();
}

void UIGOTypeNew::onWindowClose( const Event* ) {
	eeDelete( this );
}

}}} // namespace EE::Maps::Private
