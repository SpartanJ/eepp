#include "compareimages.hpp"
#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;

UTEST( RichText, basicFunctionality ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	RichText richText;
	richText.getFontStyleConfig().Font = font;
	richText.getFontStyleConfig().CharacterSize = 12;

	richText.addSpan( "Hello " );
	richText.addSpan( "world" );
	richText.addSpan( "bold", nullptr, 0, Color::White, Text::Bold );

	// Force layout update
	Sizef size = richText.getSize();
	EXPECT_TRUE( size.getWidth() > 0 );
	EXPECT_TRUE( size.getHeight() > 0 );

	// Check that we have lines and spans
	const auto& lines = richText.getLines();
	EXPECT_FALSE( lines.empty() );
	if ( !lines.empty() ) {
		EXPECT_FALSE( lines[0].spans.empty() );
		// Check that spans have increasing X positions
		if ( lines[0].spans.size() >= 2 ) {
			EXPECT_GT( lines[0].spans[1].position.x, lines[0].spans[0].position.x );
		}
	}

	// Check wrapping
	Float fullWidth = size.getWidth();
	richText.setMaxWidth( fullWidth / 2 );

	Sizef wrappedSize = richText.getSize();
	EXPECT_LT( wrappedSize.getWidth(), fullWidth );
	EXPECT_GT( wrappedSize.getHeight(), size.getHeight() );

	Engine::destroySingleton();
}

UTEST( RichText, BaselineAlignment ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Baseline",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	RichText richText;
	richText.getFontStyleConfig().Font = font;
	richText.addSpan( "Large", nullptr, 30 );
	richText.addSpan( "Small", nullptr, 12 );

	richText.getSize(); // Update layout

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines[0].spans.size(), (size_t)2 );

	const auto& largeSpan = lines[0].spans[0];
	const auto& smallSpan = lines[0].spans[1];

	// Large span should be at the top of the line (offset 0 relative to ascent difference)
	// Small span should be pushed down
	Float largeAscent = font->getAscent( 30 );
	Float smallAscent = font->getAscent( 12 );

	// Expected offsets
	Float expectedLargeY = 0; // maxAscent - largeAscent = 0
	Float expectedSmallY = largeAscent - smallAscent;

	EXPECT_NEAR( largeSpan.position.y, expectedLargeY, 0.001f );
	EXPECT_NEAR( smallSpan.position.y, expectedSmallY, 0.001f );

	EXPECT_GT( smallSpan.position.y, largeSpan.position.y );

	Engine::destroySingleton();
}

UTEST( LineWrap, SoftWrapPreventsWordSplitWithOffset ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "LineWrap Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	String text = " World";
	Float width = Text::getTextWidth( font, 20, text, 0, 4, 0.f );
	Float maxWidth = width * 1.5f;
	Float offset = maxWidth - ( width * 0.5f );

	LineWrapInfo info =
		LineWrap::computeLineBreaks( text, font, 20, maxWidth, LineWrapMode::Word, 0, 0.f, false, 4,
									 0.f, TextHints::None, false, offset );

	// With " World" (space at 0, W at 1).
	// LineWrap returns the index of the wrap.
	// Index 0 is always pushed at start.
	// If we wrap at 1 (skipping space), we should have at least 2 wraps: 0 and 1.
	ASSERT_GE( info.wraps.size(), (size_t)2 );
	EXPECT_EQ( info.wraps[1], 1 );

	delete font;

	Engine::destroySingleton();
}

UTEST( RichText, RichTextTest ) {
	const auto& createRichText = []( Font* font ) -> RichText {
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
		return richText;
	};

	const auto& runTest = [&createRichText, &utest_result]() {
		auto win =
			Engine::instance()->createWindow( WindowSettings( 1024, 650, "RichText Example" ) );

		if ( !win->isOpen() )
			return;

		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		FontTrueType* font =
			FontTrueType::New( "NotoSans-Regular", "../assets/fonts/NotoSans-Regular.ttf" );

		ASSERT_TRUE( font && font->loaded() );

		FontFamily::loadFromRegular( font );

		RichText richText = createRichText( font );
		richText.setMaxWidth( std::ceil( win->getWidth() * 0.4 ) );
		richText.setPosition( { 50.f, 50.f } );

		richText.setMaxWidth( std::ceil( win->getWidth() * 0.4 ) );
		richText.setPosition( { 25.f, 50.f } );

		RichText richText2 = richText;
		richText2.setPosition(
			richText2.getPosition() + Vector2f{ 25.f, 0.f } +
			Vector2f{ static_cast<Float>( std::ceil( win->getWidth() * 0.4 ) ), 0 } );
		richText2.setMaxWidth( std::ceil( win->getWidth() * 0.15 ) );

		RichText richText3 = richText2;
		richText3.setPosition(
			Vector2f{ 25.f, 50.f } +
			Vector2f{ static_cast<Float>( std::ceil( win->getWidth() * 0.6 ) ), 0 } );
		richText3.setMaxWidth( win->getWidth() - richText3.getPosition().x );

		win->setClearColor( Color( 200, 200, 200 ) );
		win->clear();

		// Draw a line to show the wrap width
		Float boxWidth = std::ceil( win->getWidth() * 0.4 );
		Primitives p;
		p.setColor( Color::Black );

		Float line1X = richText.getPosition().x + boxWidth;
		p.drawPixelPerfectLineRectangle(
			{ line1X, 0, line1X + p.getLineWidth(), (Float)win->getHeight() } );

		Float line2X =
			richText2.getPosition().x + static_cast<Float>( std::ceil( win->getWidth() * 0.15 ) );
		p.drawPixelPerfectLineRectangle(
			{ line2X, 0, line2X + p.getLineWidth(), (Float)win->getHeight() } );

		richText.draw();
		richText2.draw();
		richText3.draw();

		compareImages( utest_state, utest_result, win, "eepp-rich-text" );

		Engine::destroySingleton();
	};

	UTEST_PRINT_STEP( "Text Shaper disabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, false );
		runTest();
	}

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
		runTest();

		UTEST_PRINT_STEP( "Text Shaper enabled w/o optimizations" );
		BoolScopedOp op2( Text::TextShaperOptimizations, false );
		runTest();
	}
}
