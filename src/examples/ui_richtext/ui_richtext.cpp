#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 800, 600, "eepp - UIRichText Example" } );
	app.getUI()->loadLayoutFromString( R"xml(
			<LinearLayout layout_width="match_parent"
						  layout_height="match_parent"
						  orientation="vertical">
				<RichText id="rich_text"
						  layout_width="match_parent"
						  layout_height="wrap_content"
						  font-size="24dp"
						  font-color="white">Welcome to the <span color="#FFD700" font-style="bold">UIRichText</span> example!
					This component supports <span color="#00FF00" font-style="italic">styled text</span>,
					<span color="#00BFFF" font-style="shadow">shadows</span>,
					and <span color="#FF4500" text-stroke-width="1dp" text-stroke-color="black">outlines</span> using <span font-family="monospace" color="#A9A9A9">HTML-like tags</span>.
				</RichText>
				<Image src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAIAAAD8GO2jAAAAqElEQVR4nO1VWw6AIAwD46G4/5e30g8Tg3t0BYkmhn7hWFu3BUhp4vfIZN6+WeQywsCU5m2QQSjN2CxD1EG+bdCqDlhrSLtqF3wvLmBUUBPqzjJrbebOYBSkQV/3gUJQQZ3NrDXkOXheQbpP5fUZTIPQQNxZuUSXpUoQn1+0SB99r4gz7l0trgHQwhGb6G3g/+LTGl40wce7lIEpZEh0v8mhDTmGiY9xADIWNJt0vYyHAAAAAElFTkSuQmCC" margin="4dp" layout-gravity="center_horizontal" />
				<RichText id="rich_text"
						  layout_width="match_parent"
						  layout_height="wrap_content"
						  font-size="24dp"
						  font-color="white">We can also mix <span color="#FFD700" font-style="bold">contents</span> with more <span color="#00FF00" font-style="italic">text</span>!
				</RichText>
			</LinearLayout>
	)xml" );
	return app.run();
}
