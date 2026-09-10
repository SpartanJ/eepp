#include <eepp/ee.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uimessagebox.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 640, 480, "eepp - UIApplication Multi-window" } );
	auto* layout = app.getUI()->loadLayoutFromString( R"xml(
		<RelativeLayout layout_width="match_parent" layout_height="match_parent">
			<PushButton id="open_file" layout_width="wrap_content" layout_height="wrap_content"
						layout_gravity="center" text="Open File..." />
		</RelativeLayout>
	)xml" );

	auto* openFile = layout->querySelector<UIPushButton>( "#open_file" );
	openFile->on( Event::MouseClick, [&app]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
			return;

		auto* dialog = UIFileDialog::NewInApplicationWindow(
			app, { 640, 400, "Open File" }, UIFileDialog::DefaultFlags, "*",
			FileSystem::getCurrentWorkingDirectory() );
		if ( nullptr == dialog )
			return;

		dialog->on( Event::OpenFile, [&app]( const Event* event ) {
			const auto path = event->getNode()->asType<UIFileDialog>()->getFullPath();
			UIMessageBox::NewInApplicationWindow( app, { 520, 100, "File Opened" },
												  UIMessageBox::OK,
												  String::format( "File %s opened", path ) );
		} );
	} );

	return app.run();
}
