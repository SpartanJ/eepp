#include <eepp/ee.hpp>
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/richtext.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;

void runRichTextTest() {
	auto win = Engine::instance()->createWindow( WindowSettings( 1024, 768, "RichText Example" ) );

	if ( !win->isOpen() )
		return;

	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font =
		FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

	if ( !font || !font->loaded() )
		return;

	FontFamily::loadFromRegular( font );

	RichText richText;
	richText.getFontStyleConfig().Font = font;
	richText.getFontStyleConfig().CharacterSize = 24;
	richText.setAlign( TEXT_ALIGN_LEFT );

	// Add spans using the helper method
	richText.addSpan( "Hello " );
	richText.addSpan( "bold world", nullptr, 24, Color::Red,
					  Text::Bold ); // Use nullptr to use the font associated with the style
									// (if loaded via FontFamily)
	richText.addSpan( "! This is a " );
	richText.addSpan( "colored", nullptr, 0,
					  Color::Green ); // Inherit font and size, change color
	richText.addSpan( " processing example. " );
	richText.addSpan( "It should support " );
	richText.addSpan( "soft wrapping", nullptr, 0, Color::Blue, Text::Italic );
	richText.addSpan( " across multiple lines if the text is long enough. " );
	richText.addSpan( "And also " );
	richText.addSpan( "different font sizes", nullptr, 32,
					  Color( 255, 0, 255, 255 ) ); // Magenta manually
	richText.addSpan( " in the same block." );

	richText.setMaxWidth( std::ceil( win->getWidth() * 0.4 ) );
	richText.setPosition( { 25.f, 50.f } );

	RichText richText2 = richText;
	richText2.setPosition(
		richText2.getPosition() + Vector2f{ 25.f, 0.f } +
		Vector2f{ static_cast<Float>( std::ceil( win->getWidth() * 0.4 ) ), 0 } );
	richText2.setMaxWidth( std::ceil( win->getWidth() * 0.1 ) );

	RichText richText3 = richText2;
	richText3.setPosition(
		Vector2f{ 25.f, 50.f } +
		Vector2f{ static_cast<Float>( std::ceil( win->getWidth() * 0.6 ) ), 0 } );
	richText3.setMaxWidth( win->getWidth() - richText3.getPosition().x );

	win->getInput()->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::VideoResize ) {
			richText.setMaxWidth( std::ceil( win->getWidth() * 0.4 ) );

			richText2.setPosition(
				richText.getPosition() + Vector2f{ 25.f, 0.f } +
				Vector2f{ static_cast<Float>( std::ceil( win->getWidth() * 0.4 ) ), 0 } );
			richText2.setMaxWidth( std::ceil( win->getWidth() * 0.1 ) );

			richText3.setPosition(
				Vector2f{ 25.f, 50.f } +
				Vector2f{ static_cast<Float>( std::ceil( win->getWidth() * 0.6 ) ), 0 } );
			richText3.setMaxWidth( win->getWidth() - richText3.getPosition().x );
		}
	} );

	while ( win->isRunning() ) {
		win->getInput()->update();

		if ( win->getInput()->isKeyUp( KEY_ESCAPE ) ) {
			win->close();
		}

		win->setClearColor( Color( 200, 200, 200 ) );
		win->clear();

		// Draw a line to show the wrap width
		Float boxWidth = std::ceil( win->getWidth() * 0.4 );
		Primitives p;
		p.setColor( Color::Black );

		Float line1X = richText.getPosition().x + boxWidth;
		p.drawLine( { { line1X, 0 }, { line1X, (Float)win->getHeight() } } );

		Float line2X =
			richText2.getPosition().x + static_cast<Float>( std::ceil( win->getWidth() * 0.1 ) );
		p.drawLine( { { line2X, 0 }, { line2X, (Float)win->getHeight() } } );

		richText.draw();
		richText2.draw();
		richText3.draw();

		win->display();
	}

	Engine::destroySingleton();
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	runRichTextTest();
	return 0;
}
