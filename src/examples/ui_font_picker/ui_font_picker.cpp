#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1280, 720, "eepp - UIFontPickerDialog Example" } );

	std::shared_ptr<ThreadPool> threadPool(
		ThreadPool::createShared( eemax<int>( 4, Sys::getCPUCount() ) ) );

	if ( !app.getWindow()->isOpen() )
		return EXIT_FAILURE;

	app.getUI()->setThreadPool( threadPool );
	app.getUI()->getUIIconThemeManager()->setCurrentTheme(
		IconManager::init( "icons", FontTrueType::New( "icon", "assets/fonts/remixicon.ttf" ),
						   FontTrueType::New( "nonicons", "assets/fonts/nonicons.ttf" ),
						   FontTrueType::New( "codicon", "assets/fonts/codicon.ttf" ) ) );

	UIFontPickerDialog* dialog = UIFontPickerDialog::New();
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->setOnFontPicked( []( const UIFontSelection& selection ) {
		Log::info( "Selected font: %s (%s, face index: %u, size: %u)",
				   selection.font.family.c_str(), selection.font.path.c_str(),
				   selection.font.faceIndex, selection.size );
	} );
	dialog->center();
	dialog->show();

	app.getUI()->on( Event::KeyUp, [&app]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 )
			UIWidgetInspector::create( app.getUI() );
	} );

	return app.run();
}
