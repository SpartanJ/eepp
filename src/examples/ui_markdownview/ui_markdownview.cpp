#include <eepp/ee.hpp>

#include <args/args.hxx>
#include <iostream>

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	UIApplication app(
		{ 1280, 720, "eepp - UIMarkdownView Example" }, {},
		ContextSettings( false, ContextSettings::FrameRateLimitScreenRefreshRate, 4 ) );

	args::ArgumentParser parser( "eepp Markdown View Example" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );

	args::Positional<std::string> uri( parser, "URI", "The local URI to load" );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme",
		"Set the preferred color scheme (\"light\", \"dark\" or \"system\")",
		{ 'c', "prefers-color-scheme" } );
	args::Flag benchmarkMode( parser, "benchmark-mode",
							  "Render as much as possible to measure the rendering performance.",
							  { "benchmark-mode" } );
	args::ValueFlag<Float> pixelDensityConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );

	try {
		parser.ParseCLI( Sys::parseArguments( argc, argv ) );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	auto ui = app.getUI();
	ui->setColorSchemePreference(
		!prefersColorScheme.Get().empty()
			? ColorSchemePreferences::fromStringExt( prefersColorScheme.Get() )
			: ColorSchemeExtPreference::Dark );

	ui->loadLayoutFromString( R"xml(
	<ScrollView layout_width="match_parent" layout_height="match_parent">
		<MarkdownView id="markdown_view" layout_width="match_parent" layout_height="wrap_content" padding="16dp"></MarkdownView>
	</ScrollView>
	)xml" );

	auto markdownView = ui->find<UIMarkdownView>( "markdown_view" );

	if ( uri.Get().empty() ) {
		markdownView->loadFromString( R"markdown(
# Markdown Header 1
## Markdown Header 2
### Markdown Header 3

This is a **bold** text and this is an *italic* text.

* Item 1
	* Sub Item 1
	* Sub Item 2
* Item 2
* Item 3

1. Ordered 1
2. Ordered 2

> This is a blockquote.

`inline code`

[this is a link](https://eepp.ensoft.dev)

```cpp
void main() {
	printf("Hello World");
}
```
	)markdown" );
	} else if ( FileSystem::fileExists( uri.Get() ) ) {
		std::string file( uri.Get() );
		std::string data;
		FileSystem::fileGet( file, data );
		markdownView->loadFromString( data );
	} else {
		std::cout << "File not found\n";

		return EXIT_FAILURE;
	}

	app.getWindow()->getInput()->pushCallback( [markdownView]( InputEvent* event ) {
		switch ( event->Type ) {
			case InputEvent::FileDropped: {
				std::string file( event->file.file );
				std::string data;
				FileSystem::fileGet( file, data );
				markdownView->loadFromString( data );
				break;
			}
			case InputEvent::TextDropped: {
				markdownView->loadFromString( event->textdrop.text );
				break;
			}
			default:
				break;
		}
	} );

	ui->on( Event::KeyUp, [ui]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( ui );
		}
	} );

	return app.run();
}
