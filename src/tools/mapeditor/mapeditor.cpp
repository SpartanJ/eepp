#include <eepp/ee.hpp>

EE::Window::Window * win = NULL;
UIMessageBox * MsgBox = NULL;
MapEditor * Editor = NULL;

bool onCloseRequestCallback( EE::Window::Window * w ) {
	if ( NULL != Editor ) {
		MsgBox = UIMessageBox::New( UIMessageBox::OK_CANCEL, "Do you really want to close the current map?\nAll changes will be lost." );
		MsgBox->addEventListener( Event::MsgBoxConfirmClick, cb::Make1<void, const Event*>( []( const Event * event ) { win->close(); } ) );
		MsgBox->addEventListener( Event::OnClose, cb::Make1<void, const Event*>( []( const Event * event ) { MsgBox = NULL; } ) );
		MsgBox->setTitle( "Close Map?" );
		MsgBox->center();
		MsgBox->show();
		return false;
	} else {
		return true;
	}
}

void mainLoop() {
	win->getInput()->update();

	if ( win->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox && onCloseRequestCallback( win ) ) {
		win->close();
	}

	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() ) {
		win->clear();

		SceneManager::instance()->draw();

		win->display();
	} else {
		Sys::sleep( Milliseconds(8) );
	}
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	Display * currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(0);
	Float pixelDensity = PixelDensity::toFloat( currentDisplay->getPixelDensity() );

	win = Engine::instance()->createWindow( WindowSettings( 1280, 720, "eepp - Map Editor", WindowStyle::Default, WindowBackend::Default, 32, "assets/icon/ee.png", pixelDensity ), ContextSettings( true, GLv_default, true, 24, 1, 0, false ) );

	if ( win->isOpen() ) {
		PixelDensity::setPixelDensity( eemax( win->getScale(), pixelDensity ) );

		win->setCloseRequestCallback( cb::Make1( onCloseRequestCallback ) );

		UISceneNode * uiSceneNode = UISceneNode::New();

		SceneManager::instance()->add( uiSceneNode );

		{
			std::string pd;
			if ( PixelDensity::getPixelDensity() >= 1.5f ) pd = "1.5x";
			else if ( PixelDensity::getPixelDensity() >= 2.f ) pd = "2x";

			FontTrueType * font = FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

			UITheme * theme = UITheme::load( "uitheme" + pd, "uitheme" + pd, "assets/ui/uitheme" + pd + ".eta", font, "assets/ui/uitheme.css" );

			uiSceneNode->combineStyleSheet( theme->getStyleSheet() );

			uiSceneNode->getUIThemeManager()->setDefaultEffectsEnabled( true )->setDefaultTheme( theme )->setDefaultFont( font )->add( theme );
		}

		Editor = MapEditor::New();

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
