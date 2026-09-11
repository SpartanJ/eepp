#include <eepp/ee.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>

#include <cstdlib>
#include <iostream>

EE_MAIN_FUNC int main( int argc, char** argv ) {
	bool multiWindow = false;
	bool closePrimary = false;
	bool benchmark = false;
	bool benchmarkInactive = false;
	size_t benchmarkItems = 1000;
	size_t benchmarkIterations = 100000;
	for ( int i = 1; i < argc; ++i ) {
		multiWindow |= std::string_view( argv[i] ) == "--multi-window";
		closePrimary |= std::string_view( argv[i] ) == "--close-primary";
		benchmark |= std::string_view( argv[i] ) == "--benchmark";
		benchmarkInactive |= std::string_view( argv[i] ) == "--benchmark-inactive";
		if ( std::string_view( argv[i] ) == "--benchmark-items" && i + 1 < argc )
			benchmarkItems = std::max<size_t>( 1, std::strtoull( argv[++i], nullptr, 10 ) );
		if ( std::string_view( argv[i] ) == "--benchmark-iterations" && i + 1 < argc )
			benchmarkIterations = std::max<size_t>( 1, std::strtoull( argv[++i], nullptr, 10 ) );
	}
	UIApplication::Settings settings;
	settings.threadPool = ThreadPool::createShared( 1 );
	UIApplication app( { 1280, 720, "eepp - Accessibility",
						 static_cast<Uint32>( benchmark || benchmarkInactive
												  ? WindowStyle::Default | WindowStyle::Hidden
												  : WindowStyle::Default ) },
					   settings );
	multiWindow |= closePrimary;
	if ( closePrimary )
		app.setQuitPolicy( UIApplication::QuitPolicy::OnLastWindowClosed );
	auto scene = app.getUI();
	auto content = scene->loadLayoutFromString( R"xml(
		<vbox lw="mp" lh="mp" padding="16dp" padding="8dp">
			<TextView text="Accessibility settings" font-size="22dp" />
			<TextInput id="project-name" aria-label="Project name" text="eepp" lw="mp" />
			<TextEdit id="description" aria-label="Description" aria-description="Multiline editor. Press Mod Tab to move to the next control." text="Accessible UI" lw="mp" lh="80dp" />
			<CheckBox id="autosave" aria-label="Enable autosave" text="Enable autosave" />
			<RadioButton id="light-theme" aria-label="Light Theme" text="Light theme" />
			<RadioButton id="dark-theme" aria-label="Dark Theme" text="Dark theme" />
			<ComboBox id="language" aria-label="Language" lw="mp" />
			<Slider id="volume" aria-label="Volume" value="40" min-value="0" max-value="100" lw="mp" orientation="horizontal" />
			<SpinBox id="retries" aria-label="Retry count" value="3" min-value="0" max-value="10" lw="mp" />
			<ProgressBar id="progress" aria-label="Operation progress" progress="35" lw="mp" />
			<TabWidget id="sections" lw="mp" lh="40dp">
				<vbox id="general-panel" lw="mp" lh="mp">
					<TextView text="General settings panel" />
				</vbox>
				<vbox id="advanced-panel" lw="mp" lh="mp">
					<TextView text="Advanced settings panel" />
				</vbox>
				<Tab text="General" owns="general-panel" />
				<Tab text="Advanced" owns="advanced-panel" />
			</TabWidget>
			<hbox lw="mp" lh="40dp" margin-top="8dp">
				<ListView id="items" aria-label="Colors" lw="50%" lw="0" lw8="0.5" lh="80dp" />
				<TableView id="projects" aria-label="Projects" lw="0" lw8="0.5" lh="80dp" />
			</hbox>
			<hbox lw="mp" lh="wc" margin-top="8dp">
				<PushButton id="save" text="Save settings" />
				<PushButton id="inspect" text="Open inspector" />
				<PushButton id="open-window" text="Open accessibility window" />
			</hbox>
			<TextView id="status" text="Ready" />
		</vbox>
	)xml" );
	content->setAccessibilityLabel( "Accessibility settings" );

	auto comboBox = content->find<UIComboBox>( "language" );
	comboBox->getListBox()->addListBoxItems( { "English", "Spanish", "German" } );
	comboBox->setText( "English" );
	auto list = content->find<UIListView>( "items" );
	if ( benchmark ) {
		std::vector<std::string> items;
		items.reserve( benchmarkItems );
		for ( size_t i = 0; i < benchmarkItems; ++i )
			items.emplace_back( "Benchmark item " + std::to_string( i ) );
		list->setAccessibilityLabel( "Benchmark items" );
		list->setModel( Models::ItemListOwnerModel<std::string>::create( std::move( items ) ) );
	} else {
		list->setModel(
			Models::ItemListOwnerModel<std::string>::create( { "Red", "Green", "Blue" } ) );
	}
	auto table = content->find<UITableView>( "projects" );
	auto tableModel = Models::ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "eepp", "Active" }, { "ecode", "Ready" } } );
	tableModel->setColumnName( 0, "Project" );
	tableModel->setColumnName( 1, "Status" );
	table->setModel( tableModel );

	auto status = content->find<UITextView>( "status" );
	if ( benchmarkInactive ) {
		Clock clock;
		scene->getAccessibilityManager()->update();
		const auto initializationUs = clock.getElapsedTime().asMicroseconds();
		clock.restart();
		for ( size_t i = 0; i < benchmarkIterations; ++i )
			status->setText( i & 1 ? "Ready" : "Idle" );
		const auto notificationsUs = clock.getElapsedTime().asMicroseconds();
		std::cout << "{\"initialization_us\":" << initializationUs
				  << ",\"iterations\":" << benchmarkIterations
				  << ",\"notifications_us\":" << notificationsUs << "}\n";
		return EXIT_SUCCESS;
	}
	content->find<UIPushButton>( "save" )->onClick(
		[status]( auto ) { status->setText( "Settings saved" ); } );
	content->find<UIPushButton>( "inspect" )->onClick( [scene]( auto ) {
		EE::UI::Tools::UIWidgetInspector::create( scene );
	} );
	content->find<UIPushButton>( "open-window" )->onClick( [&app]( auto ) {
		auto* extra = app.createWindow( { 400, 180, "eepp - Accessibility Dynamic" } );
		if ( !extra )
			return;
		auto* extraContent = extra->loadLayoutFromString( R"xml(
			<vbox lw="mp" lh="mp" padding="16dp">
				<TextView text="Dynamic accessibility window" />
				<PushButton id="close-dynamic" text="Close dynamic window" />
			</vbox>
		)xml" );
		extraContent->find<UIPushButton>( "close-dynamic" )->onClick( [&app, extra]( auto ) {
			app.closeWindow( extra->getWindow() );
		} );
	} );
	if ( benchmark ) {
		auto benchmarkRoot = UIWidget::New();
		benchmarkRoot->setAccessibilityRole( AccessibilityRole::Group );
		benchmarkRoot->setAccessibilityLabel( "Benchmark depth 0" );
		benchmarkRoot->setParent( content );
		auto parent = benchmarkRoot;
		for ( size_t i = 1; i < 32; ++i ) {
			auto group = UIWidget::New();
			group->setAccessibilityRole( AccessibilityRole::Group );
			group->setAccessibilityLabel( "Benchmark depth " + std::to_string( i ) );
			group->setParent( parent );
			parent = group;
		}
		auto leaf = UIPushButton::New();
		leaf->setText( "Benchmark depth leaf" );
		leaf->setParent( parent );

		auto mutate = UIPushButton::New();
		mutate->setText( "Replace benchmark model" );
		mutate->setParent( content );
		mutate->onClick( [list, benchmarkItems]( auto ) {
			for ( size_t generation = 0; generation < 16; ++generation ) {
				std::vector<std::string> items;
				items.reserve( benchmarkItems );
				for ( size_t i = 0; i < benchmarkItems; ++i )
					items.emplace_back( "Updated benchmark item " + std::to_string( i ) );
				list->setModel(
					Models::ItemListOwnerModel<std::string>::create( std::move( items ) ) );
			}
		} );
	}
	if ( multiWindow ) {
		auto* primaryWindow = app.getWindow();
		auto* secondary = app.createWindow( { 480, 240, "eepp - Accessibility Secondary" } );
		if ( secondary ) {
			auto* secondaryContent = secondary->loadLayoutFromString( R"xml(
				<vbox lw="mp" lh="mp" padding="16dp">
					<TextView text="Secondary accessibility window" />
					<PushButton id="close-secondary" text="Close secondary window" />
					<PushButton id="close-primary" text="Close primary window" />
				</vbox>
			)xml" );
			secondaryContent->find<UIPushButton>( "close-secondary" )
				->onClick(
					[&app, secondary]( auto ) { app.closeWindow( secondary->getWindow() ); } );
			secondaryContent->find<UIPushButton>( "close-primary" )
				->onClick( [&app, primaryWindow]( auto ) { app.closeWindow( primaryWindow ); } );
		}
	}
	content->find( "project-name" )->setFocus();
	return app.run();
}
