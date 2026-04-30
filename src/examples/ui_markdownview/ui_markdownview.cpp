#include <eepp/ee.hpp>
#include <eepp/ui/uimarkdownview.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1280, 720, "eepp - UIMarkdownView Example" } );

	app.getUI()->loadLayoutFromString( R"xml(
	<ScrollView layout_width="match_parent" layout_height="match_parent">
		<MarkdownView id="markdown_view" layout_width="match_parent" layout_height="wrap_content" padding="16dp">
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
		</MarkdownView>
	</ScrollView>
	)xml" );

	auto markdownView = app.getUI()->find<UIMarkdownView>( "markdown_view" );

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

	app.getUI()->on( Event::KeyUp, [&app]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( app.getUI() );
		}
	} );

	return app.run();
}
