#include "compareimages.hpp"
#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;

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

UTEST( RichText, selection ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	RichText richText;
	richText.getFontStyleConfig().Font = font;
	richText.getFontStyleConfig().CharacterSize = 20;

	richText.addSpan( "Hello " );
	richText.addSpan( "world" );

	// "Hello world" is 11 characters
	EXPECT_EQ( richText.getCharacterCount(), (Int64)11 );

	// Test findCharacterFromPos
	richText.getSize(); // Force layout

	// First character 'H' at (0, 0)
	EXPECT_EQ( richText.findCharacterFromPos( { 0, 5 } ), (Int64)0 );

	// Somewhere in "Hello "
	Int64 pos5 = richText.findCharacterFromPos( { 20, 5 } );
	EXPECT_GT( pos5, 0 );
	EXPECT_LT( pos5, 6 );

	// End of string
	EXPECT_EQ( richText.findCharacterFromPos( { 1000, 5 } ), (Int64)11 );

	// Test findCharacterPos
	Vector2f char0Pos = richText.findCharacterPos( 0 );
	EXPECT_EQ( char0Pos.x, 0.f );

	Vector2f char11Pos = richText.findCharacterPos( 11 );
	EXPECT_GT( char11Pos.x, 0.f );

	// Test selection rects
	richText.setSelection( { 0, 5 } ); // "Hello"
	auto rects = richText.getSelectionRects();
	EXPECT_FALSE( rects.empty() );
	if ( !rects.empty() ) {
		EXPECT_NEAR( rects[0].getWidth(), richText.findCharacterPos( 5 ).x, 1.0f );
	}

	// Test multi-span selection
	richText.setSelection( { 0, 11 } ); // "Hello world"
	rects = richText.getSelectionRects();
	EXPECT_FALSE( rects.empty() );

	// Test selection across lines
	richText.setMaxWidth( 50 ); // Should wrap
	richText.getSize();
	richText.setSelection( { 0, 11 } );
	rects = richText.getSelectionRects();
	EXPECT_GT( rects.size(), (size_t)1 );

	// Test getSelectionString
	EXPECT_STRINGEQ( richText.getSelectionString(), "Hello world" );
	richText.setSelection( { 0, 5 } );
	EXPECT_STRINGEQ( richText.getSelectionString(), "Hello" );
	richText.setSelection( { 6, 11 } );
	EXPECT_STRINGEQ( richText.getSelectionString(), "world" );

	// Test with explicit newlines
	richText.clear();
	richText.addSpan( "Hello\n" );
	richText.addSpan( "world" );
	EXPECT_EQ( richText.getCharacterCount(), (Int64)11 );
	richText.setSelection( { 0, 11 } );
	EXPECT_STRINGEQ( richText.getSelectionString(), "Hello\nworld" );
	richText.setSelection( { 5, 7 } );
	EXPECT_STRINGEQ( richText.getSelectionString(), "\nw" );

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

	// Expected offsets
	Float expectedLargeY = largeAscent - 30;
	Float expectedSmallY = largeAscent - 12;

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
		auto win = Engine::instance()->createWindow(
			WindowSettings( 1024, 650, "RichText Example", WindowStyle::Default,
							WindowBackend::Default, 32, "", 1, EE_SCREEN_KEYBOARD_ENABLED, true ) );

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

UTEST( UIRichText, IntegrationAndLayoutVerification ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content">Hello <span color="#FF0000">Red</span><Widget id="placeholder" layout_width="50dp" layout_height="50dp"/>World</RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	auto graphicsRt = rt->getRichText();
	const auto& blocks = graphicsRt.getBlocks();

	ASSERT_EQ( blocks.size(), (size_t)4 );

	// Check Text block
	EXPECT_TRUE( std::holds_alternative<std::shared_ptr<Graphics::Text>>( blocks[1] ) );
	auto text1 = std::get<std::shared_ptr<Graphics::Text>>( blocks[1] );
	EXPECT_TRUE( text1->getFillColor() == Color::fromString( "#FF0000" ) );

	// Check CustomSize block
	EXPECT_TRUE( std::holds_alternative<Sizef>( blocks[2] ) );
	EXPECT_EQ( std::get<Sizef>( blocks[2] ).getWidth(), PixelDensity::dpToPx( 50 ) );

	UI::UIWidget* placeholder = rt->find<UI::UIWidget>( "placeholder" );
	ASSERT_TRUE( placeholder != nullptr );

	auto text0 = std::get<std::shared_ptr<Graphics::Text>>( blocks[0] );
	Vector2f pos = placeholder->getPixelsPosition();
	Float expectedX = text0->getTextWidth() + text1->getTextWidth();
	EXPECT_NEAR( pos.x, expectedX, 2.0f );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, selection ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content" text-selection="true">Hello <span color="#FF0000">Red</span> World</RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	EXPECT_TRUE( rt->isTextSelectionEnabled() );

	// Force layout
	sceneNode->update( Time::Zero );

	// Test findCharacterFromPos
	Int64 charPos = rt->getRichText().findCharacterFromPos( { 0, 5 } );
	EXPECT_EQ( charPos, 0 );

	// Test selection manually
	rt->setTextSelectionRange( { 0, 5 } );
	auto range = rt->getTextSelectionRange();
	EXPECT_EQ( range.first, 0 );
	EXPECT_EQ( range.second, 5 );
	EXPECT_STRINGEQ( rt->getSelectionString(), "Hello" );

	rt->setTextSelectionRange( { 0, 11 } );
	EXPECT_STRINGEQ( rt->getSelectionString(), "Hello Red W" );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, NestedWidgetsIntegration ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content">Hello <strong id="strong"><span>Beautiful </span><Widget id="placeholder" layout_width="50dp" layout_height="50dp"/> World</strong></RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );
	sceneNode->draw();

	auto graphicsRt = rt->getRichText();
	const auto& blocks = graphicsRt.getBlocks();

	ASSERT_EQ( blocks.size(), (size_t)4 );

	// Check block types
	EXPECT_TRUE( std::holds_alternative<std::shared_ptr<Graphics::Text>>( blocks[0] ) );
	EXPECT_TRUE( std::holds_alternative<std::shared_ptr<Graphics::Text>>( blocks[1] ) );
	EXPECT_TRUE( std::holds_alternative<Sizef>( blocks[2] ) );
	EXPECT_TRUE( std::holds_alternative<std::shared_ptr<Graphics::Text>>( blocks[3] ) );

	EXPECT_EQ( std::get<Sizef>( blocks[2] ).getWidth(), PixelDensity::dpToPx( 50 ) );

	UI::UIWidget* strongNode = rt->find<UI::UIWidget>( "strong" );
	ASSERT_TRUE( strongNode != nullptr );

	UI::UIWidget* placeholder = rt->find<UI::UIWidget>( "placeholder" );
	ASSERT_TRUE( placeholder != nullptr );

	auto text0 = std::get<std::shared_ptr<Graphics::Text>>( blocks[0] );
	auto text1 = std::get<std::shared_ptr<Graphics::Text>>( blocks[1] );

	Vector2f pos = placeholder->getScreenPos();
	Float expectedX = text0->getTextWidth() + text1->getTextWidth();

	EXPECT_NEAR( expectedX, pos.x, 2.0f );

	// Determine if strong got its bounds correctly
	EXPECT_GT( strongNode->getPixelsSize().getWidth(), 0 );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, DefaultStyleInheritance ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <RichText id="rt" font-size="24dp" color="#FF0000" layout_width="300dp" layout_height="wrap_content">Default size<span font-size="16dp" color="#00FF00">Small</span></RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	auto graphicsRt = rt->getRichText();
	const auto& blocks = graphicsRt.getBlocks();

	// blocks[0] should be "Default size" with parent's size and color
	// blocks[1] should be "Small" with overridden size and color
	ASSERT_TRUE( blocks.size() >= 2 );

	EXPECT_TRUE( std::holds_alternative<std::shared_ptr<Graphics::Text>>( blocks[0] ) );
	auto text0 = std::get<std::shared_ptr<Graphics::Text>>( blocks[0] );
	EXPECT_EQ( text0->getCharacterSize(), rt->getFontSize() );
	EXPECT_EQ( text0->getFillColor().getValue(), rt->getFontColor().getValue() );
	EXPECT_EQ( text0->getFillColor().getValue(), Color::fromString( "#FF0000" ).getValue() );

	EXPECT_TRUE( std::holds_alternative<std::shared_ptr<Graphics::Text>>( blocks[1] ) );
	auto text1 = std::get<std::shared_ptr<Graphics::Text>>( blocks[1] );
	EXPECT_EQ( text1->getCharacterSize(), (unsigned int)PixelDensity::dpToPxI( 16 ) );
	EXPECT_EQ( text1->getFillColor().getValue(), Color::fromString( "#00FF00" ).getValue() );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, RichTextTest ) {
	const auto runTest = [&]() {
		UIApplication app( WindowSettings( 800, 600, "eepp - UIRichText Test", WindowStyle::Default,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5 ) );

		app.getUI()->loadLayoutFromString( R"xml(
			<LinearLayout layout_width="match_parent"
						  layout_height="match_parent"
						  orientation="vertical">
				<RichText font-size="12dp"
					color="white">Welcome to the <span color="#FFD700" font-style="bold">UIRichText</span> example!
					This component supports <span color="#00FF00" font-style="italic">styled text</span>,
					<span color="#00BFFF" font-style="shadow">shadows</span>,
					and <span color="#FF4500" text-stroke-width="1dp" text-stroke-color="black">outlines</span> using <span font-family="monospace" color="#A9A9A9">HTML-like tags</span>.
				</RichText>
				<Image src="file://assets/icon/ee.png" margin="4dp" layout-gravity="center_horizontal" />
				<RichText font-size="12dp"
				color="#efefef">We can also mix <span color="#FFD700" font-style="bold">contents</span> with more <span color="#00FF00" font-style="italic">text</span>!
				</RichText>
			</LinearLayout>
		)xml" );

		SceneManager::instance()->update();
		SceneManager::instance()->draw();

		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-ui-richtext" );
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

UTEST( UIRichText, UIAnchorTest ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <RichText id="rt" font-size="24dp" color="#FF0000" layout_width="300dp" layout_height="wrap_content">Default size <a id="anchor1" href="https://example.com" color="#00FF00">Link text</a> and <a id="anchor2" href="https://example.org">Another link</a></RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	UI::UIAnchorSpan* anchor1 = sceneNode->find<UI::UIAnchorSpan>( "anchor1" );
	ASSERT_TRUE( anchor1 != nullptr );
	EXPECT_STRINGEQ( anchor1->getHref(), "https://example.com" );
	EXPECT_TRUE( anchor1->getHitBoxes().size() >= 1 );

	UI::UIAnchorSpan* anchor2 = sceneNode->find<UI::UIAnchorSpan>( "anchor2" );
	ASSERT_TRUE( anchor2 != nullptr );
	EXPECT_STRINGEQ( anchor2->getHref(), "https://example.org" );
	EXPECT_TRUE( anchor2->getHitBoxes().size() >= 1 );

	// Test that overFind correctly returns the anchor
	if ( !anchor1->getHitBoxes().empty() ) {
		Vector2f hitPos = anchor1->convertToWorldSpace(
			{ anchor1->getHitBoxes()[0].Left + 1, anchor1->getHitBoxes()[0].Top + 1 } );
		Node* hitNode = rt->overFind( hitPos );
		EXPECT_EQ( hitNode, anchor1 );
	}

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, WhitespaceCollapseTest ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <RichText id="rt">
           <span>Hello</span>
           <ul>
               <li>Item</li>
           </ul>

        </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	int spanCount = 0;
	Node* child = rt->getFirstChild();
	while ( child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
			UI::UITextSpan* span = static_cast<UI::UITextSpan*>( child );
			if ( !span->getText().empty() ) {
				spanCount++;
			}
		}
		child = child->getNextNode();
	}

	// Only 1 text span ("Hello") should be generated,
	// the whitespace between <span> and <ul>, and after <ul>
	// should be correctly collapsed into nothing since they aren't adjacent to inline elements on
	// both sides.
	EXPECT_EQ( spanCount, 1 );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, WhitespaceCollapseCodeTest ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
	    <vbox lw="mp" lh="mp">
		<RichText id="rt">Hello <a href="#">World</a>. <code>HI in monospace!</code></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	bool foundDotSpace = false;
	Node* child = rt->getFirstChild();
	while ( child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
			UI::UITextSpan* span = static_cast<UI::UITextSpan*>( child );
			if ( span->getText() == ". " ) {
				foundDotSpace = true;
			}
		}
		child = child->getNextNode();
	}

	EXPECT_TRUE( foundDotSpace );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIHTMLTable, basicLayout ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "HTML Table Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
		<table id="table" layout_width="400dp" layout_height="wrap_content">
			<thead>
				<tr>
					<th>Header 1</th>
					<th>Header 2</th>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td>Row 1 Col 1</td>
					<td>Row 1 Col 2</td>
				</tr>
				<tr>
					<td>Row 2 Col 1 which is very long and should cause wrapping if the table is narrow enough</td>
					<td>Row 2 Col 2</td>
				</tr>
			</tbody>
		</table>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIHTMLTable* table = sceneNode->find<UI::UIHTMLTable>( "table" );
	ASSERT_TRUE( table != nullptr );

	// Force layout
	sceneNode->update( Time::Zero );

	// Check that we have rows and cells
	int rowCount = 0;
	std::function<void( Node* )> countRows = [&]( Node* node ) {
		Node* child = node->getFirstChild();
		while ( child ) {
			if ( child->isWidget() ) {
				UIWidget* widget = static_cast<UIWidget*>( child );
				if ( widget->getType() == UI_TYPE_HTML_TABLE_ROW ) {
					rowCount++;
				} else if ( widget->getType() != UI_TYPE_HTML_TABLE ) {
					countRows( widget );
				}
			}
			child = child->getNextNode();
		}
	};
	countRows( table );
	EXPECT_EQ( rowCount, 3 );

	// Verify that the table has a height greater than zero
	EXPECT_GT( table->getPixelsSize().getHeight(), 0 );

	// Check column synchronization
	std::vector<UIHTMLTableRow*> rows;
	std::function<void( Node* )> collectRows = [&]( Node* node ) {
		Node* child = node->getFirstChild();
		while ( child ) {
			if ( child->isWidget() ) {
				UIWidget* widget = static_cast<UIWidget*>( child );
				if ( widget->getType() == UI_TYPE_HTML_TABLE_ROW ) {
					rows.push_back( static_cast<UIHTMLTableRow*>( widget ) );
				} else if ( widget->getType() != UI_TYPE_HTML_TABLE ) {
					collectRows( widget );
				}
			}
			child = child->getNextNode();
		}
	};
	collectRows( table );

	if ( rows.size() >= 2 ) {
		Node* cell00 = rows[0]->getFirstChild();
		Node* cell10 = rows[1]->getFirstChild();
		if ( cell00 && cell10 && cell00->isWidget() && cell10->isWidget() ) {
			EXPECT_EQ( cell00->asType<UIWidget>()->getPixelsPosition().x,
					   cell10->asType<UIWidget>()->getPixelsPosition().x );
		}
	}

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, WhitespaceCollapseBRTest ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
<h1 align="center" id="rt">
  <img src="icon" /><br/>
  ecode
</h1>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	// The "ecode" text span should NOT have a leading space.
	bool foundEcodeWithLeadingSpace = false;
	auto checkSpansRecursive = [&]( Node* n, auto&& checkSpansRecursiveRef ) -> void {
		if ( !n )
			return;
		if ( n->isWidget() && n->isType( UI_TYPE_TEXTSPAN ) ) {
			UI::UITextSpan* span = static_cast<UI::UITextSpan*>( n );
			if ( span->getText().size() > 0 && span->getText()[0] == ' ' &&
				 span->getText().find( "ecode" ) != String::InvalidPos ) {
				foundEcodeWithLeadingSpace = true;
			}
		}
		for ( Node* child = n->getFirstChild(); child; child = child->getNextNode() ) {
			checkSpansRecursiveRef( child, checkSpansRecursiveRef );
		}
	};
	checkSpansRecursive( rt, checkSpansRecursive );
	EXPECT_FALSE( foundEcodeWithLeadingSpace );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, MarginsTest ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
<richtext id="rt" layout-width="wrap_content" layout-height="wrap_content">
	<div id="d1" margin="10px 20px 30px 40px" width="50px" height="50px" />
	<div id="d2" margin="5px" width="50px" height="50px" />
</richtext>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	UI::UIWidget* d1 = sceneNode->find<UI::UIWidget>( "d1" );
	ASSERT_TRUE( d1 != nullptr );
	UI::UIWidget* d2 = sceneNode->find<UI::UIWidget>( "d2" );
	ASSERT_TRUE( d2 != nullptr );

	sceneNode->update( Time::Zero );

	// Check the layout position of the first div
	Vector2f pos1 = d1->getPixelsPosition();
	// margin left is 40px, top is 10px, so position inside richtext should be (40, 10)
	// (CSS order is: top right bottom left -> 10px 20px 30px 40px)
	EXPECT_EQ( 40.f, pos1.x );
	EXPECT_EQ( 10.f, pos1.y );

	// Check the layout position of the second div
	Vector2f pos2 = d2->getPixelsPosition();
	// The widgets flow inline (horizontally) since total width < 800.
	// d1 footprint width: 40 (left) + 50 (width) + 20 (right) = 110.
	// d2 left margin: 5.
	// Therefore d2 x position = 110 + 5 = 115.
	// Line height is determined by max footprint height.
	// d1 footprint height: 10 + 50 + 30 = 90.
	// d2 footprint height: 5 + 50 + 5 = 60.
	// Max ascent = 90.
	// RichText baseline aligns elements to the bottom by default.
	// d2 offsetY = 90 - 60 = 30.
	// d2 y position = offsetY (30) + d2 margin top (5) = 35.
	EXPECT_EQ( 115.f, pos2.x );
	EXPECT_EQ( 35.f, pos2.y );

	// Check UIRichText bounds
	// Width = d1 footprint (110) + d2 footprint (60) = 170.
	// Height = line height (90).
	EXPECT_EQ( 170.f, rt->getPixelsSize().getWidth() );
	EXPECT_EQ( 90.f, rt->getPixelsSize().getHeight() );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, ForcedLineBreak ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "BR Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ) );

	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	String xml = R"xml(<richtext id="rt">Line 1<br/>Line 2</richtext>)xml";

	sceneNode->loadLayoutFromString( xml );
	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	const auto& richText = rt->getRichText();
	EXPECT_EQ( richText.getLines().size(), (size_t)3 );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, CustomBRHeight ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "BR Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ) );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	String xml = R"xml(<richtext id="rt">Line 1<br font-size="50px"/>Line 2</richtext>)xml";

	sceneNode->loadLayoutFromString( xml );
	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	const auto& richText = rt->getRichText();
	const auto& lines = richText.getLines();
	EXPECT_EQ( lines.size(), (size_t)3 );

	if ( lines.size() >= 2 ) {
		EXPECT_GT( lines[0].height, lines[1].height );
		EXPECT_GT( lines[2].height, 0.f );
	}

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, MinMaxWidth ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Min/Max Width Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
		<LinearLayout id="container" layout_width="match_parent" layout_height="match_parent">
			<RichText id="rt_min" layout_width="wrap_content" layout_height="wrap_content" min-width="200dp">Short</RichText>
			<RichText id="rt_max" layout_width="wrap_content" layout_height="wrap_content" max-width="100dp">This is a very long text that should definitely wrap because of the max-width property being set to 100dp.</RichText>
			<RichText id="rt_max_fixed" layout_width="500dp" layout_height="wrap_content" max-width="100dp">This is another very long text with fixed width policy.</RichText>
		</LinearLayout>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rtMin = sceneNode->find<UI::UIRichText>( "rt_min" );
	UI::UIRichText* rtMax = sceneNode->find<UI::UIRichText>( "rt_max" );
	UI::UIRichText* rtMaxFixed = sceneNode->find<UI::UIRichText>( "rt_max_fixed" );
	ASSERT_TRUE( rtMin != nullptr );
	ASSERT_TRUE( rtMax != nullptr );
	ASSERT_TRUE( rtMaxFixed != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_EQ( rtMin->getSize().getWidth(), PixelDensity::dpToPx( 200 ) );
	EXPECT_LE( rtMax->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( rtMax->getSize().getHeight(), PixelDensity::dpToPx( 30 ) ); // should wrap to multiple lines
	EXPECT_LE( rtMaxFixed->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( rtMaxFixed->getSize().getHeight(), PixelDensity::dpToPx( 30 ) ); // should wrap to multiple lines

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, MinMaxWidthChildren ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Min/Max Width Children Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
		<LinearLayout id="container" layout_width="match_parent" layout_height="match_parent">
			<RichText id="rt_parent" layout_width="wrap_content" layout_height="wrap_content" max-width="100dp">
				This is a long text that expands the RichText so its max-width is reached.
				<Widget id="child_widget" layout_width="match_parent" layout_height="50dp" />
			</RichText>
		</LinearLayout>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rtParent = sceneNode->find<UI::UIRichText>( "rt_parent" );
	UI::UIWidget* childWidget = sceneNode->find<UI::UIWidget>( "child_widget" );
	ASSERT_TRUE( rtParent != nullptr );
	ASSERT_TRUE( childWidget != nullptr );

	sceneNode->update( Time::Zero );
	sceneNode->update( Time::Zero ); // Run a second pass to allow MatchParent to resolve against the new clamped parent size
	sceneNode->update( Time::Zero );

	EXPECT_LE( rtParent->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( rtParent->getSize().getWidth(), 0 ); // Assert it's not 0
	EXPECT_EQ( childWidget->getSize().getWidth(), rtParent->getSize().getWidth() );
	EXPECT_LE( childWidget->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( childWidget->getSize().getWidth(), 0 ); // Assert it's not 0

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UIRichText, MatchParentChildPadding ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText MatchParent Child Padding Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
		<LinearLayout id="container" layout_width="match_parent" layout_height="match_parent">
			<RichText id="rt_parent" layout_width="200dp" layout_height="wrap_content" padding="10dp">
				<Widget id="child_widget" layout_width="match_parent" layout_height="50dp" />
			</RichText>
		</LinearLayout>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rtParent = sceneNode->find<UI::UIRichText>( "rt_parent" );
	UI::UIWidget* childWidget = sceneNode->find<UI::UIWidget>( "child_widget" );
	ASSERT_TRUE( rtParent != nullptr );
	ASSERT_TRUE( childWidget != nullptr );

	sceneNode->update( Time::Zero );

	Float parentWidth = rtParent->getSize().getWidth();
	Float childWidth = childWidget->getSize().getWidth();
	Float expectedChildWidth = parentWidth - PixelDensity::dpToPx( 20 );

	EXPECT_EQ( childWidth, expectedChildWidth );

	eeDelete( sceneNode );
	Engine::destroySingleton();
}

UTEST( UILayout, MinMaxWidthChildren ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Layout Min/Max Width Children Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	String xml = R"xml(
		<LinearLayout id="container" layout_width="match_parent" layout_height="match_parent">
			<LinearLayout id="ll_parent" layout_width="wrap_content" layout_height="wrap_content" max-width="150dp">
				<Widget id="child_widget1" layout_width="300dp" layout_height="50dp" />
				<Widget id="child_widget2" layout_width="match_parent" layout_height="50dp" />
			</LinearLayout>
		</LinearLayout>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIWidget* llParent = sceneNode->find<UI::UIWidget>( "ll_parent" );
	UI::UIWidget* childWidget2 = sceneNode->find<UI::UIWidget>( "child_widget2" );
	ASSERT_TRUE( llParent != nullptr );
	ASSERT_TRUE( childWidget2 != nullptr );

	sceneNode->update( Time::Zero );
	sceneNode->update( Time::Zero );
	sceneNode->update( Time::Zero );

	EXPECT_LE( llParent->getSize().getWidth(), PixelDensity::dpToPx( 150 ) );
	EXPECT_GT( llParent->getSize().getWidth(), 0 ); // Assert it's not 0
	EXPECT_EQ( childWidget2->getSize().getWidth(), llParent->getSize().getWidth() );
	EXPECT_LE( childWidget2->getSize().getWidth(), PixelDensity::dpToPx( 150 ) );
	EXPECT_GT( childWidget2->getSize().getWidth(), 0 ); // Assert it's not 0

	eeDelete( sceneNode );
	Engine::destroySingleton();
}
