#include <eepp/ee.hpp>

// Demonstrates UI direction: the same layout authored with logical properties (start/end,
// margin-inline-*, padding-inline-*) works in both left-to-right and right-to-left. Press the
// "Toggle direction" button (or F2) to switch the whole scene at runtime; the "LTR panel" keeps
// its own direction override. The HTML sample uses a WebView to keep document CSS isolated
// from the native widgets.

static void loadOptionalFallbackFont( const std::string& name, const std::string& path ) {
	if ( FileSystem::fileExists( path ) ) {
		defaultResourceScope().getFontService().addFallbackFont(
			FontTrueType::New( name, path ).get() );
	}
}

EE_MAIN_FUNC int main( int, char** ) {
	Text::TextShaperEnabled = true;
	UIApplication app( { 1024, 768, "eepp - UI Direction (RTL)" } );
	// Hebrew and Arabic glyphs come from fallback fonts shipped with the unit test assets.
	loadOptionalFallbackFont( "NotoSansHebrew-Regular",
							  "unit_tests/assets/fonts/NotoSansHebrew-Regular.ttf" );
	loadOptionalFallbackFont( "NotoNaskhArabic-Regular",
							  "unit_tests/assets/fonts/NotoNaskhArabic-Regular.ttf" );

	UISceneNode* ui = app.getUI();
	ui->loadLayoutFromString( R"xml(
	<vbox id="main" layout_width="match_parent" layout_height="match_parent">
		<MenuBar id="menubar" layout_width="match_parent" layout_height="wrap_content" />
		<hbox layout_width="match_parent" layout_height="wrap_content"
			  padding-inline-start="12dp" padding-inline-end="12dp" margin-top="8dp">
			<TextView id="title" text="UI direction demo — שלום עולם — مرحبا بالعالم"
					  layout_width="0dp" layout_weight="1" layout_height="wrap_content"
					  text-align="start" font-size="16dp" />
			<PushButton id="toggle" text="Toggle direction (F2)" layout_gravity="center_vertical" />
		</hbox>
		<hbox layout_width="match_parent" layout_height="0dp" layout_weight="1" margin-top="8dp"
			  padding-inline-start="12dp" padding-inline-end="12dp">
			<vbox layout_width="0dp" layout_weight="1" layout_height="match_parent">
				<TextView text="Name / שם" text-align="start" layout_width="match_parent" />
				<TextInput id="name" layout_width="match_parent" hint="Type here / הקלד כאן" />
				<CheckBox id="check" text="Remember me / זכור אותי" margin-top="8dp"
						  layout_width="match_parent" />
				<RadioButton text="Option A / אפשרות א" layout_width="match_parent" />
				<RadioButton text="Option B / אפשרות ב" layout_width="match_parent" />
				<hbox layout_gravity="end" margin-top="8dp">
					<PushButton text="Cancel" />
					<PushButton text="OK" margin-inline-start="8dp" />
				</hbox>
				<TabWidget id="tabs" layout_width="match_parent" layout_height="0dp"
						   layout_weight="1" margin-top="8dp" />
			</vbox>
			<vbox id="ltr_panel" direction="ltr" layout_width="0dp" layout_weight="1"
				  layout_height="match_parent" margin-inline-start="12dp">
				<TextView text="LTR panel (direction override)" layout_width="match_parent" />
				<hbox>
					<PushButton text="One" />
					<PushButton text="Two" margin-inline-start="4dp" />
					<PushButton text="Three" margin-inline-start="4dp" />
				</hbox>
				<WebView id="html_host" layout_width="match_parent" layout_height="0dp"
						 layout_weight="1" margin-top="8dp">
					<html><body dir="rtl">
						<h3>HTML dir="rtl"</h3>
						<p>פסקה בעברית with an English phrase inside.</p>
						<ul><li>פריט ראשון</li><li>Second item</li></ul>
						<div style="display:flex; gap:4px">
							<div style="background:#cde; padding:4px">1</div>
							<div style="background:#dec; padding:4px">2</div>
							<div style="background:#ecd; padding:4px">3</div>
						</div>
						<p dir="ltr">A nested dir="ltr" paragraph.</p>
					</body></html>
				</WebView>
			</vbox>
		</hbox>
	</vbox>
	)xml" );

	UIMenuBar* menuBar = ui->find<UIMenuBar>( "menubar" );
	UIPopUpMenu* fileMenu = UIPopUpMenu::New();
	fileMenu->add( "Open / פתח", nullptr, "Ctrl+O" );
	fileMenu->add( "Save / שמור", nullptr, "Ctrl+S" );
	UIPopUpMenu* recent = UIPopUpMenu::New();
	recent->add( "first.txt" );
	recent->add( "second.txt" );
	fileMenu->addSubMenu( "Recent / אחרונים", nullptr, recent );
	menuBar->addMenuButton( "File / קובץ", fileMenu );
	UIPopUpMenu* editMenu = UIPopUpMenu::New();
	editMenu->add( "Undo / בטל", nullptr, "Ctrl+Z" );
	menuBar->addMenuButton( "Edit / עריכה", editMenu );

	UITabWidget* tabs = ui->find<UITabWidget>( "tabs" );
	tabs->add( "First / ראשון", UITextView::New()->setText( "Tab one" ) );
	tabs->add( "Second / שני", UITextView::New()->setText( "Tab two" ) );
	tabs->add( "Third / שלישי", UITextView::New()->setText( "Tab three" ) );

	auto toggle = [ui] {
		ui->setDirection( ui->getDirection() == UIDirection::RightToLeft
							  ? UIDirection::LeftToRight
							  : UIDirection::RightToLeft );
	};
	ui->find<UIPushButton>( "toggle" )->onClick( [toggle]( auto ) { toggle(); } );
	ui->on( Event::KeyUp, [ui, toggle]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F2 )
			toggle();
		else if ( event->asKeyEvent()->getKeyCode() == KEY_F11 )
			UIWidgetInspector::create( ui );
	} );

	return app.run();
}
