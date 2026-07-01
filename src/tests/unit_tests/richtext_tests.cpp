#include "compareimages.hpp"
#include "utest.hpp"

#include <algorithm>
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uibackgrounddrawable.hpp>
#include <eepp/ui/uiborderdrawable.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <limits>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Tools;

static UI::UISceneNode* createRichTextScene() {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	if ( !font->loaded() ) {
		Engine::destroySingleton();
		return nullptr;
	}
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	return sceneNode;
}

static void destroyRichTextScene( UI::UISceneNode* sceneNode ) {
	eeDelete( sceneNode );
	Engine::destroySingleton();
}

static String richTextRenderedText( const RichText& richText ) {
	String text;
	const auto& lines = richText.getLines();
	for ( size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex ) {
		if ( lineIndex > 0 )
			text += '\n';
		for ( const auto& span : lines[lineIndex].spans ) {
			if ( span.type == RichText::RenderSpan::Type::Text && span.text )
				text += span.text->getString();
		}
	}
	return text;
}

static String richTextLineText( const RichText& richText, size_t lineIndex ) {
	String text;
	const auto& lines = richText.getLines();
	if ( lineIndex >= lines.size() )
		return text;
	for ( const auto& span : lines[lineIndex].spans ) {
		if ( span.type == RichText::RenderSpan::Type::Text && span.text )
			text += span.text->getString();
	}
	while ( !text.empty() && ( text.front() == '\n' || text.front() == '\r' ) )
		text = text.substr( 1 );
	while ( !text.empty() && ( text.back() == '\n' || text.back() == '\r' ) )
		text.pop_back();
	return text;
}

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

	Float largeAscent = font->getAscent( 30 );
	Float smallAscent = font->getAscent( 12 );

	Float expectedLargeY = 0;
	Float expectedSmallY = largeAscent - smallAscent;

	EXPECT_NEAR( largeSpan.position.y, expectedLargeY, 0.001f );
	EXPECT_NEAR( smallSpan.position.y, expectedSmallY, 0.001f );

	EXPECT_GT( smallSpan.position.y, largeSpan.position.y );

	Engine::destroySingleton();
}

UTEST( RichText, VerticalAlignAtomicBoxes ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Vertical Align",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	RichText baselineRt;
	baselineRt.getFontStyleConfig().Font = font;
	baselineRt.getFontStyleConfig().CharacterSize = 20;
	baselineRt.addSpan( "A", nullptr, 20 );
	baselineRt.addCustomSize( Sizef( 20, 20 ), RichText::InlineFloat::None,
							  RichText::InlineClear::None, 10.f );
	baselineRt.getSize();
	ASSERT_EQ( baselineRt.getLines().front().spans.size(), (size_t)2 );
	Float baselineY = baselineRt.getLines().front().spans[1].position.y;

	RichText::BaselineAlignValue middleAlign;
	middleAlign.type = RichText::BaselineAlignment::Middle;
	RichText middleRt;
	middleRt.getFontStyleConfig().Font = font;
	middleRt.getFontStyleConfig().CharacterSize = 20;
	middleRt.addSpan( "A", nullptr, 20 );
	middleRt.addCustomSize( Sizef( 20, 20 ), RichText::InlineFloat::None,
							RichText::InlineClear::None, 10.f, middleAlign );
	middleRt.getSize();
	ASSERT_EQ( middleRt.getLines().front().spans.size(), (size_t)2 );
	EXPECT_NEAR( middleRt.getLines().front().spans[1].position.y, baselineY, 0.001f );

	RichText mixedRt;
	mixedRt.getFontStyleConfig().Font = font;
	mixedRt.getFontStyleConfig().CharacterSize = 20;
	mixedRt.addSpan( "Large", nullptr, 30 );
	mixedRt.addCustomSize( Sizef( 10, 10 ), RichText::InlineFloat::None,
						   RichText::InlineClear::None, 5.f, middleAlign );
	mixedRt.getSize();
	ASSERT_EQ( mixedRt.getLines().front().spans.size(), (size_t)2 );
	const auto& mixedLine = mixedRt.getLines().front();
	Float expectedMiddleY = mixedLine.maxAscent - 5.f;
	EXPECT_NEAR( mixedLine.spans[1].position.y, expectedMiddleY, 0.001f );

	RichText nestedRt;
	nestedRt.getFontStyleConfig().Font = font;
	nestedRt.getFontStyleConfig().CharacterSize = 16;
	FontStyleConfig smallStyle = nestedRt.getFontStyleConfig();
	smallStyle.CharacterSize = 10;
	nestedRt.addSpan( "Title ", nullptr, 16 );
	nestedRt.pushInlineBox( Rectf::Zero, Rectf::Zero, 0, {}, Color::Transparent, 0,
							Color::Transparent, 0, {} );
	nestedRt.addInlineText( "(", smallStyle, Rectf::Zero, Rectf::Zero, 0, {} );
	nestedRt.addCustomSize( Sizef( 36, 14 ), RichText::InlineFloat::None,
							RichText::InlineClear::None, font->getAscent( 10 ), middleAlign );
	nestedRt.addInlineText( ")", smallStyle, Rectf::Zero, Rectf::Zero, 0, {} );
	nestedRt.popInlineBox();
	nestedRt.getSize();
	ASSERT_EQ( nestedRt.getLines().front().spans.size(), (size_t)4 );
	const auto& nestedOpenParen = nestedRt.getLines().front().spans[1];
	const auto& nestedAtomic = nestedRt.getLines().front().spans[2];
	ASSERT_TRUE( nestedOpenParen.text != nullptr );
	EXPECT_STRINGEQ( nestedOpenParen.text->getString(), "(" );
	EXPECT_EQ( nestedAtomic.type, RichText::RenderSpan::Type::AtomicBox );
	EXPECT_NEAR( nestedAtomic.position.y, nestedOpenParen.position.y, 0.5f );

	RichText::BaselineAlignValue lengthAlign;
	lengthAlign.type = RichText::BaselineAlignment::Length;
	lengthAlign.value = 4.f;
	RichText lengthRt;
	lengthRt.getFontStyleConfig().Font = font;
	lengthRt.getFontStyleConfig().CharacterSize = 20;
	lengthRt.addSpan( "A", nullptr, 20 );
	lengthRt.addCustomSize( Sizef( 20, 20 ), RichText::InlineFloat::None,
							RichText::InlineClear::None, 10.f, lengthAlign );
	lengthRt.getSize();
	ASSERT_EQ( lengthRt.getLines().front().spans.size(), (size_t)2 );
	EXPECT_NEAR( lengthRt.getLines().front().spans[1].position.y, baselineY - 4.f, 0.001f );

	RichText::BaselineAlignValue percentAlign;
	percentAlign.type = RichText::BaselineAlignment::Percentage;
	percentAlign.value = 50.f;
	RichText percentRt;
	percentRt.getFontStyleConfig().Font = font;
	percentRt.getFontStyleConfig().CharacterSize = 20;
	percentRt.addSpan( "A", nullptr, 20 );
	percentRt.addCustomSize( Sizef( 20, 20 ), RichText::InlineFloat::None,
							 RichText::InlineClear::None, 10.f, percentAlign );
	percentRt.getSize();
	ASSERT_EQ( percentRt.getLines().front().spans.size(), (size_t)2 );
	EXPECT_NEAR( percentRt.getLines().front().spans[1].position.y, baselineY - 10.f, 0.001f );

	Engine::destroySingleton();
}

UTEST( RichText, InlineTextUsesActiveInlineBoxAlignment ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Inline Alignment",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText::BaselineAlignValue bottomAlign;
	bottomAlign.type = RichText::BaselineAlignment::Bottom;

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.addSpan( "A", style );
	richText.pushInlineBox( Rectf::Zero, Rectf::Zero, 0, bottomAlign );
	richText.pushInlineBox( Rectf::Zero, Rectf::Zero, 0, {} );
	richText.addInlineText( "x", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.popInlineBox();
	richText.popInlineBox();
	richText.addSpan( "B", style );
	richText.updateLayout();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines[0].spans.size(), (size_t)3 );
	ASSERT_EQ( lines[0].spans[1].type, RichText::RenderSpan::Type::Text );
	EXPECT_EQ( lines[0].spans[1].baselineAlign.type, RichText::BaselineAlignment::Baseline );

	bool foundTextFragment = false;
	for ( const auto& fragment : richText.getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::TextRun &&
			 fragment.itemPath.size() == 3 ) {
			foundTextFragment = true;
			EXPECT_EQ( fragment.baselineAlign.type, RichText::BaselineAlignment::Bottom );
		}
	}
	EXPECT_TRUE( foundTextFragment );

	Engine::destroySingleton();
}

UTEST( RichText, InlineAncestorLineHeightContributesToLineHeight ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Inline Box Metrics",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText baseline;
	baseline.setFontStyleConfig( style );
	baseline.addSpan( "A", style );
	baseline.updateLayout();
	ASSERT_EQ( baseline.getLines().size(), (size_t)1 );
	Float baselineHeight = baseline.getLines().front().height;

	RichText lineHeightBox;
	lineHeightBox.setFontStyleConfig( style );
	lineHeightBox.pushInlineBox( Rectf::Zero, Rectf::Zero, baselineHeight + 20.f, {} );
	lineHeightBox.addInlineText( "A", style, Rectf::Zero, Rectf::Zero, 0, {} );
	lineHeightBox.popInlineBox();
	lineHeightBox.updateLayout();

	ASSERT_EQ( lineHeightBox.getLines().size(), (size_t)1 );
	EXPECT_GE( lineHeightBox.getLines().front().height, baselineHeight + 20.f );

	const RichText::InlineFragment* boxFragment = nullptr;
	const RichText::InlineFragment* textFragment = nullptr;
	for ( const auto& fragment : lineHeightBox.getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box )
			boxFragment = &fragment;
		else if ( fragment.type == RichText::InlineFragment::Type::TextRun )
			textFragment = &fragment;
	}
	ASSERT_TRUE( boxFragment != nullptr );
	ASSERT_TRUE( textFragment != nullptr );
	EXPECT_GE( boxFragment->bounds.getHeight(), baselineHeight + 20.f );
	EXPECT_LT( boxFragment->bounds.Top, textFragment->bounds.Top );
	EXPECT_GT( boxFragment->bounds.Bottom, textFragment->bounds.Bottom );

	Engine::destroySingleton();
}

UTEST( RichText, FloatAwareInlineLayoutUsesTreeOrder ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Float Inline Tree Order",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.addInlineAtomicBox( Sizef( 30, 20 ), RichText::InlineFloat::Left,
								 RichText::InlineClear::None, 20, false, {} );
	richText.addInlineText( "A", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.updateLayout();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)2 );
	EXPECT_EQ( lines.front().spans[0].type, RichText::RenderSpan::Type::AtomicBox );

	ASSERT_EQ( lines.front().spans[1].type, RichText::RenderSpan::Type::Text );
	ASSERT_TRUE( lines.front().spans[1].text != nullptr );
	EXPECT_STRINGEQ( lines.front().spans[1].text->getString(), "A" );
	EXPECT_GE( lines.front().spans[1].position.x, 30.f );

	Engine::destroySingleton();
}

UTEST( RichText, RenderSpanPayloadSupportsDrawableAndAtomicSelection ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText RenderSpan Blocks",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	class TestDrawable : public Drawable {
	  public:
		TestDrawable() : Drawable( Drawable::CUSTOM ) {}

		Sizef getSize() override { return Sizef( 8, 6 ); }
		Sizef getPixelsSize() override { return Sizef( 8, 6 ); }
		void draw() override { drawCount++; }
		void draw( const Vector2f& ) override { drawCount++; }
		void draw( const Vector2f&, const Sizef& ) override { drawCount++; }
		bool isStateful() override { return false; }

		int drawCount{ 0 };
	};

	auto drawable = std::make_shared<TestDrawable>();

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.addInlineText( "A", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.addDrawable( drawable );
	richText.addInlineAtomicBox( Sizef( 5, 4 ), RichText::InlineFloat::None,
								 RichText::InlineClear::None, 4, false, {} );
	richText.addInlineText( "B", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.updateLayout();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)4 );
	EXPECT_EQ( lines.front().spans[0].type, RichText::RenderSpan::Type::Text );
	EXPECT_EQ( lines.front().spans[1].type, RichText::RenderSpan::Type::Drawable );
	EXPECT_EQ( lines.front().spans[2].type, RichText::RenderSpan::Type::AtomicBox );
	EXPECT_EQ( lines.front().spans[3].type, RichText::RenderSpan::Type::Text );

	EXPECT_EQ( richText.getCharacterCount(), 4 );
	richText.setSelection( { 0, richText.getCharacterCount() } );
	EXPECT_STRINGEQ( richText.getSelectionString(), "A  B" );

	richText.draw( 0, 0 );
	EXPECT_EQ( drawable->drawCount, 1 );

	Engine::destroySingleton();
}

UTEST( RichText, InlineBoxHorizontalEdgesContributeToAdvance ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Inline Box Edges",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.addSpan( "A", style );
	richText.pushInlineBox( Rectf( 5, 0, 7, 0 ), Rectf( 11, 0, 13, 0 ), 0, {} );
	richText.addInlineText( "B", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.popInlineBox();
	richText.addSpan( "C", style );
	richText.updateLayout();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)3 );

	const auto& a = lines.front().spans[0];
	const auto& b = lines.front().spans[1];
	const auto& c = lines.front().spans[2];
	EXPECT_NEAR( b.position.x, a.position.x + a.size.getWidth() + 16.f, 0.001f );
	EXPECT_NEAR( c.position.x, b.position.x + b.size.getWidth() + 20.f, 0.001f );
	EXPECT_NEAR( lines.front().width,
				 a.size.getWidth() + b.size.getWidth() + c.size.getWidth() + 36.f, 0.001f );
	EXPECT_NEAR( richText.getMaxIntrinsicWidth(), lines.front().width, 0.001f );

	RichText atomicRichText;
	atomicRichText.setFontStyleConfig( style );
	atomicRichText.addSpan( "A", style );
	atomicRichText.pushInlineBox( Rectf( 3, 0, 5, 0 ), Rectf( 7, 0, 11, 0 ), 0, {} );
	atomicRichText.addInlineAtomicBox( Sizef( 17, 9 ), RichText::InlineFloat::None,
									   RichText::InlineClear::None, 8, false, {} );
	atomicRichText.popInlineBox();
	atomicRichText.addSpan( "C", style );
	atomicRichText.updateLayout();

	const auto& atomicLines = atomicRichText.getLines();
	ASSERT_EQ( atomicLines.size(), (size_t)1 );
	ASSERT_EQ( atomicLines.front().spans.size(), (size_t)3 );
	const auto& atomicA = atomicLines.front().spans[0];
	const auto& atomicBox = atomicLines.front().spans[1];
	const auto& atomicC = atomicLines.front().spans[2];
	EXPECT_NEAR( atomicBox.position.x, atomicA.position.x + atomicA.size.getWidth() + 10.f,
				 0.001f );
	EXPECT_NEAR( atomicC.position.x, atomicBox.position.x + atomicBox.size.getWidth() + 16.f,
				 0.001f );
	EXPECT_NEAR( atomicRichText.getMaxIntrinsicWidth(), atomicLines.front().width, 0.001f );

	Engine::destroySingleton();
}

UTEST( RichText, HitTestingSnapsAcrossInlineBoxSpacing ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Inline Box Hit Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.addSpan( "A", style );
	richText.pushInlineBox( Rectf::Zero, Rectf( 20, 0, 20, 0 ), 0, {} );
	richText.addInlineText( "B", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.popInlineBox();
	richText.addSpan( "C", style );
	richText.updateLayout();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)3 );

	const auto& a = lines.front().spans[0];
	const auto& b = lines.front().spans[1];
	const auto& c = lines.front().spans[2];
	const Int64 beforeInlineText = b.startCharIndex;
	const Int64 afterInlineText = b.endCharIndex;
	const Int32 lineY = static_cast<Int32>( lines.front().y + b.position.y + 1 );

	EXPECT_EQ( richText.findCharacterFromPos(
				   { static_cast<Int32>( a.position.x + a.size.getWidth() + 5 ), lineY } ),
			   beforeInlineText );
	EXPECT_EQ( richText.findCharacterFromPos(
				   { static_cast<Int32>( b.position.x + b.size.getWidth() + 5 ), lineY } ),
			   afterInlineText );
	EXPECT_EQ( c.startCharIndex, afterInlineText );

	Engine::destroySingleton();
}

UTEST( RichText, SelectionRectsUseInlineFragments ) {
	Engine::instance()->createWindow(
		WindowSettings( 800, 600, "RichText Inline Fragment Selection", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.addSpan( "A", style );
	richText.pushInlineBox( Rectf::Zero, Rectf( 20, 0, 20, 0 ), 0, {} );
	richText.addInlineText( "B", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.popInlineBox();
	richText.addSpan( "C", style );
	richText.updateLayout();

	const RichText::InlineFragment* selectedFragment = nullptr;
	for ( const auto& fragment : richText.getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::TextRun && fragment.text &&
			 fragment.text->getString() == "B" ) {
			selectedFragment = &fragment;
			break;
		}
	}
	ASSERT_TRUE( selectedFragment != nullptr );
	EXPECT_LT( selectedFragment->startCharIndex, selectedFragment->endCharIndex );

	richText.setSelection( { selectedFragment->startCharIndex, selectedFragment->endCharIndex } );
	auto rects = richText.getSelectionRects();
	ASSERT_EQ( rects.size(), (size_t)1 );
	EXPECT_NEAR( rects[0].Left, selectedFragment->bounds.Left, 0.001f );
	EXPECT_NEAR( rects[0].Top, selectedFragment->bounds.Top, 0.001f );
	EXPECT_NEAR( rects[0].Right, selectedFragment->bounds.Right, 0.001f );
	EXPECT_NEAR( rects[0].Bottom, selectedFragment->bounds.Bottom, 0.001f );

	Engine::destroySingleton();
}

UTEST( RichText, InlineParentTextDecorationReachesFragments ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Inline Decoration",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 20;
	style.FontColor = Color::White;

	RichText richText;
	richText.setFontStyleConfig( style );
	richText.pushInlineBox( Rectf::Zero, Rectf::Zero, 0, {}, Color::Transparent, 0,
							Color::Transparent, Text::Underlined );
	richText.pushInlineBox( Rectf::Zero, Rectf::Zero, 0, {} );
	richText.addInlineText( "child", style, Rectf::Zero, Rectf::Zero, 0, {} );
	richText.popInlineBox();
	richText.popInlineBox();
	richText.updateLayout();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans[0].type, RichText::RenderSpan::Type::Text );
	ASSERT_TRUE( lines.front().spans[0].text != nullptr );
	EXPECT_TRUE( ( lines.front().spans[0].text->getStyle() & Text::Underlined ) != 0 );

	const RichText::InlineFragment* textFragment = nullptr;
	const RichText::InlineFragment* outerFragment = nullptr;
	for ( const auto& fragment : richText.getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::TextRun )
			textFragment = &fragment;
		else if ( fragment.type == RichText::InlineFragment::Type::Box &&
				  fragment.itemPath.size() == 1 )
			outerFragment = &fragment;
	}
	ASSERT_TRUE( textFragment != nullptr );
	ASSERT_TRUE( outerFragment != nullptr );
	EXPECT_TRUE( ( textFragment->textDecoration & Text::Underlined ) != 0 );
	EXPECT_TRUE( ( outerFragment->textDecoration & Text::Underlined ) != 0 );

	Engine::destroySingleton();
}

UTEST( RichText, AtomicInlineBoxBaselineAlignment ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Custom Block Baseline",
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
	richText.addCustomSize( Sizef( 24, 30 ), RichText::InlineFloat::None,
							RichText::InlineClear::None, 12.f );

	richText.getSize();

	const auto& lines = richText.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines[0].spans.size(), (size_t)3 );

	const auto& customSpan = lines[0].spans[2];

	EXPECT_NEAR( customSpan.position.y, font->getAscent( 30 ) - 12.f, 0.001f );
	EXPECT_GT( customSpan.position.y, 0.f );

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
			WindowSettings( 1024, 650, "RichText Example", VisualTestWindowStyle,
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
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content">Hello <span color="#FF0000">Red</span><Widget id="placeholder" layout_width="50dp" layout_height="50dp"/>World</RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	auto graphicsRt = rt->getRichText();
	const auto& lines = graphicsRt.getLines();

	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)4 );

	// Check Text span
	ASSERT_EQ( lines.front().spans[1].type, RichText::RenderSpan::Type::Text );
	auto text1 = lines.front().spans[1].text;
	ASSERT_TRUE( text1 != nullptr );
	EXPECT_TRUE( text1->getFillColor() == Color::fromString( "#FF0000" ) );

	// Check atomic widget span
	ASSERT_EQ( lines.front().spans[2].type, RichText::RenderSpan::Type::AtomicBox );
	EXPECT_EQ( lines.front().spans[2].size.getWidth(), PixelDensity::dpToPx( 50 ) );

	UI::UIWidget* placeholder = rt->find<UI::UIWidget>( "placeholder" );
	ASSERT_TRUE( placeholder != nullptr );

	ASSERT_EQ( lines.front().spans[0].type, RichText::RenderSpan::Type::Text );
	auto text0 = lines.front().spans[0].text;
	ASSERT_TRUE( text0 != nullptr );
	Vector2f pos = placeholder->getPixelsPosition();
	Float expectedX = text0->getTextWidth() + text1->getTextWidth();
	EXPECT_NEAR( pos.x, expectedX, 2.0f );

	destroyRichTextScene( sceneNode );
}

UTEST( RichText, VirtualLineBreakSeparatesAtomicBoxes ) {
	RichText rt;
	rt.addCustomSize( { 10, 5 } );
	rt.addLineBreak();
	rt.addCustomSize( { 20, 7 } );
	rt.updateLayout();

	const auto& lines = rt.getLines();
	ASSERT_EQ( lines.size(), (size_t)2 );
	ASSERT_EQ( lines[0].spans.size(), (size_t)1 );
	ASSERT_EQ( lines[1].spans.size(), (size_t)1 );
	EXPECT_EQ( lines[0].spans[0].position.x, 0 );
	EXPECT_EQ( lines[1].spans[0].position.x, 0 );
	EXPECT_EQ( lines[1].y, lines[0].height );
	EXPECT_EQ( rt.getSize().getHeight(), lines[0].height + lines[1].height );
}

UTEST( UIRichText, selection ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, NestedWidgetsIntegration ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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
	const auto& lines = graphicsRt.getLines();
	ASSERT_EQ( lines.size(), (size_t)1 );
	ASSERT_EQ( lines.front().spans.size(), (size_t)4 );

	EXPECT_EQ( lines.front().spans[0].type, RichText::RenderSpan::Type::Text );
	EXPECT_EQ( lines.front().spans[1].type, RichText::RenderSpan::Type::Text );
	EXPECT_EQ( lines.front().spans[2].type, RichText::RenderSpan::Type::AtomicBox );
	EXPECT_EQ( lines.front().spans[3].type, RichText::RenderSpan::Type::Text );

	EXPECT_EQ( lines.front().spans[2].size.getWidth(), PixelDensity::dpToPx( 50 ) );

	UI::UIWidget* strongNode = rt->find<UI::UIWidget>( "strong" );
	ASSERT_TRUE( strongNode != nullptr );

	UI::UIWidget* placeholder = rt->find<UI::UIWidget>( "placeholder" );
	ASSERT_TRUE( placeholder != nullptr );

	auto text0 = lines.front().spans[0].text;
	auto text1 = lines.front().spans[1].text;
	ASSERT_TRUE( text0 != nullptr );
	ASSERT_TRUE( text1 != nullptr );

	Vector2f pos = placeholder->getScreenPos();
	Float expectedX = text0->getTextWidth() + text1->getTextWidth();

	EXPECT_NEAR( expectedX, pos.x, 2.0f );

	// Determine if strong got its bounds correctly
	EXPECT_GT( strongNode->getPixelsSize().getWidth(), 0 );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineTreePreservesNestedInlineBoxes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content">
	        Hello <span id="outer">before <a id="link" href="#">link</a> after</span> tail
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	auto graphicsRt = rt->getRichText();
	const auto& inlineItems = graphicsRt.getInlineItems();
	ASSERT_GE( inlineItems.size(), (size_t)3 );
	ASSERT_TRUE( inlineItems[1].isBox() );

	const auto& outer = inlineItems[1].asBox();
	ASSERT_EQ( outer.children.size(), (size_t)3 );
	EXPECT_TRUE( outer.children[0].isTextRun() );
	ASSERT_TRUE( outer.children[1].isBox() );
	EXPECT_TRUE( outer.children[2].isTextRun() );

	const auto& link = outer.children[1].asBox();
	ASSERT_EQ( link.children.size(), (size_t)1 );
	EXPECT_TRUE( link.children[0].isTextRun() );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineTreeVerticalAlignStaysOnParentBox ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content">
	        A<span id="outer" vertical-align="bottom"><a id="link" href="#">link</a></span>B
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	auto graphicsRt = rt->getRichText();
	const auto& inlineItems = graphicsRt.getInlineItems();
	ASSERT_GE( inlineItems.size(), (size_t)3 );
	ASSERT_TRUE( inlineItems[1].isBox() );

	const auto& outer = inlineItems[1].asBox();
	EXPECT_EQ( outer.baselineAlign.type, RichText::BaselineAlignment::Bottom );
	ASSERT_EQ( outer.children.size(), (size_t)1 );
	ASSERT_TRUE( outer.children[0].isBox() );

	const auto& link = outer.children[0].asBox();
	EXPECT_EQ( link.baselineAlign.type, RichText::BaselineAlignment::Baseline );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineParentCreatesFragmentsAcrossWrappedLines ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="90dp" layout_height="wrap_content" font-size="18dp">
	        x <span id="outer" vertical-align="bottom" background-color="#00ff00">alpha beta gamma delta epsilon zeta</span> y
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	UI::UITextSpan* outerSpan = sceneNode->find<UI::UITextSpan>( "outer" );
	ASSERT_TRUE( outerSpan != nullptr );

	auto graphicsRt = rt->getRichText();
	const auto& inlineItems = graphicsRt.getInlineItems();
	ASSERT_GE( inlineItems.size(), (size_t)2 );

	RichText::RenderSpan::InlinePath outerPath;
	for ( size_t i = 0; i < inlineItems.size(); ++i ) {
		if ( inlineItems[i].isBox() &&
			 inlineItems[i].asBox().baselineAlign.type == RichText::BaselineAlignment::Bottom ) {
			outerPath = { i };
			break;
		}
	}
	ASSERT_FALSE( outerPath.empty() );

	size_t outerFragmentCount = 0;
	size_t firstLine = std::numeric_limits<size_t>::max();
	size_t lastLine = 0;
	SmallVector<Rectf, 4> outerFragmentBounds;
	SmallVector<const RichText::InlineFragment*, 4> outerFragments;
	for ( const auto& fragment : graphicsRt.getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box &&
			 fragment.itemPath == outerPath ) {
			outerFragmentCount++;
			firstLine = std::min( firstLine, fragment.lineIndex );
			lastLine = std::max( lastLine, fragment.lineIndex );
			outerFragments.push_back( &fragment );
			Rectf fragmentBounds = fragment.bounds;
			fragmentBounds.move(
				{ rt->getPixelsContentOffset().Left, rt->getPixelsContentOffset().Top } );
			fragmentBounds.move( -outerSpan->getPixelsPosition() );
			outerFragmentBounds.push_back( fragmentBounds );
			EXPECT_GT( fragment.bounds.getWidth(), 0 );
			EXPECT_GT( fragment.bounds.getHeight(), 0 );
			EXPECT_EQ( fragment.baselineAlign.type, RichText::BaselineAlignment::Bottom );
			EXPECT_EQ( fragment.backgroundColor.getValue(),
					   Color::fromString( "#00ff00" ).getValue() );
		}
	}

	EXPECT_GE( outerFragmentCount, (size_t)2 );
	EXPECT_LT( firstLine, lastLine );
	ASSERT_FALSE( outerFragments.empty() );
	for ( const auto* fragment : outerFragments ) {
		if ( fragment->lineIndex == firstLine )
			EXPECT_TRUE( fragment->startsInlineBox );
		else
			EXPECT_FALSE( fragment->startsInlineBox );
		if ( fragment->lineIndex == lastLine )
			EXPECT_TRUE( fragment->endsInlineBox );
		else
			EXPECT_FALSE( fragment->endsInlineBox );
	}
	ASSERT_EQ( outerSpan->getHitBoxes().size(), outerFragmentBounds.size() );
	for ( size_t i = 0; i < outerFragmentBounds.size(); ++i ) {
		EXPECT_NEAR( outerSpan->getHitBoxes()[i].Left, outerFragmentBounds[i].Left, 0.001f );
		EXPECT_NEAR( outerSpan->getHitBoxes()[i].Top, outerFragmentBounds[i].Top, 0.001f );
		EXPECT_NEAR( outerSpan->getHitBoxes()[i].Right, outerFragmentBounds[i].Right, 0.001f );
		EXPECT_NEAR( outerSpan->getHitBoxes()[i].Bottom, outerFragmentBounds[i].Bottom, 0.001f );
	}

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineParentLineHeightFromCssContributesToFragments ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content" font-size="18dp">
	        A<span id="outer" line-height="48dp">line</span>B
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	UI::UITextSpan* outerSpan = sceneNode->find<UI::UITextSpan>( "outer" );
	ASSERT_TRUE( outerSpan != nullptr );

	auto graphicsRt = rt->getRichText();
	ASSERT_EQ( graphicsRt.getLines().size(), (size_t)1 );
	EXPECT_GE( graphicsRt.getLines().front().height, PixelDensity::dpToPx( 48 ) );

	const RichText::InlineFragment* boxFragment = nullptr;
	const RichText::InlineFragment* textFragment = nullptr;
	for ( const auto& fragment : graphicsRt.getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box &&
			 fragment.source.type == RichText::InlineSourceType::Widget &&
			 fragment.source.ptr == outerSpan ) {
			boxFragment = &fragment;
		} else if ( fragment.type == RichText::InlineFragment::Type::TextRun &&
					fragment.itemPath.size() > 1 ) {
			textFragment = &fragment;
		}
	}

	ASSERT_TRUE( boxFragment != nullptr );
	ASSERT_TRUE( textFragment != nullptr );
	EXPECT_GE( boxFragment->bounds.getHeight(), PixelDensity::dpToPx( 48 ) );
	EXPECT_LT( boxFragment->bounds.Top, textFragment->bounds.Top );
	EXPECT_GT( boxFragment->bounds.Bottom, textFragment->bounds.Bottom );
	EXPECT_GE( outerSpan->getPixelsSize().getHeight(), PixelDensity::dpToPx( 48 ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineParentBorderIsPreservedInFragments ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content" font-size="18dp">
	        A<span id="outer" padding="3dp" background-color="#00ff00">boxed</span>B
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	UI::UITextSpan* outerSpan = sceneNode->find<UI::UITextSpan>( "outer" );
	ASSERT_TRUE( outerSpan != nullptr );
	outerSpan->setBorderWidth( 2 );
	outerSpan->getBorder()->setColor( Color::White );
	outerSpan->getBorder()->setColorTop( Color::Red );
	outerSpan->getBorder()->setColorRight( Color::Red );
	outerSpan->getBorder()->setColorBottom( Color::Red );
	outerSpan->getBorder()->setColorLeft( Color::Red );

	UIRichText::rebuildRichText( rt, *rt->getRichTextPtr() );
	rt->getRichTextPtr()->updateLayout();

	auto graphicsRt = rt->getRichText();
	const auto& fragments = graphicsRt.getInlineFragments();

	const RichText::InlineFragment* boxFragment = nullptr;
	for ( const auto& fragment : fragments ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box &&
			 fragment.source.type == RichText::InlineSourceType::Widget &&
			 fragment.source.ptr == outerSpan ) {
			boxFragment = &fragment;
		}
	}

	ASSERT_TRUE( boxFragment != nullptr );
	const RichText::InlineFragment* textFragment = nullptr;
	for ( const auto& fragment : fragments ) {
		if ( fragment.type == RichText::InlineFragment::Type::TextRun &&
			 fragment.itemPath.size() > boxFragment->itemPath.size() &&
			 std::equal( boxFragment->itemPath.begin(), boxFragment->itemPath.end(),
						 fragment.itemPath.begin() ) ) {
			textFragment = &fragment;
			break;
		}
	}
	ASSERT_TRUE( textFragment != nullptr );
	EXPECT_GT( boxFragment->borderWidth, 0 );
	EXPECT_EQ( boxFragment->borderColor.getValue(), Color::fromString( "#ff0000" ).getValue() );
	EXPECT_EQ( boxFragment->backgroundColor.getValue(), Color::fromString( "#00ff00" ).getValue() );
	EXPECT_LT( boxFragment->paintBounds.Left, textFragment->bounds.Left );
	EXPECT_GT( boxFragment->paintBounds.Right, textFragment->bounds.Right );
	EXPECT_LT( boxFragment->paintBounds.Top, textFragment->bounds.Top );
	EXPECT_GT( boxFragment->paintBounds.Bottom, textFragment->bounds.Bottom );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineParentFontBackgroundColorIgnoresEmptyBackgroundDrawable ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content" font-size="18dp">
	        A<span id="outer">boxed</span>B
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	UI::UITextSpan* outerSpan = sceneNode->find<UI::UITextSpan>( "outer" );
	ASSERT_TRUE( outerSpan != nullptr );

	const Color backgroundColor = Color::fromString( "#00ff00" );
	outerSpan->setFontBackgroundColor( backgroundColor );
	outerSpan->setBorderWidth( 2 );
	ASSERT_TRUE( outerSpan->hasBackground() );
	ASSERT_EQ( outerSpan->getBackground()->getBackgroundColor().getValue(),
			   Color::Transparent.getValue() );

	UIRichText::rebuildRichText( rt, *rt->getRichTextPtr() );
	rt->getRichTextPtr()->updateLayout();

	const RichText::InlineFragment* boxFragment = nullptr;
	for ( const auto& fragment : rt->getRichText().getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box &&
			 fragment.source.type == RichText::InlineSourceType::Widget &&
			 fragment.source.ptr == outerSpan ) {
			boxFragment = &fragment;
			break;
		}
	}

	ASSERT_TRUE( boxFragment != nullptr );
	EXPECT_EQ( boxFragment->backgroundColor.getValue(), backgroundColor.getValue() );
	EXPECT_TRUE( boxFragment->backgroundDrawable == nullptr );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InlineParentFontBackgroundColorUsesBorderRadiusDrawable ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" layout_width="300dp" layout_height="wrap_content" font-size="18dp">
	        A<span id="outer">boxed</span>B
	    </RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );
	UI::UITextSpan* outerSpan = sceneNode->find<UI::UITextSpan>( "outer" );
	ASSERT_TRUE( outerSpan != nullptr );

	const Color backgroundColor = Color::fromString( "#b45f38" );
	outerSpan->setFontBackgroundColor( backgroundColor );
	outerSpan->setTopLeftRadius( "2px" );
	outerSpan->setTopRightRadius( "2px" );
	outerSpan->setBottomLeftRadius( "2px" );
	outerSpan->setBottomRightRadius( "2px" );

	UIRichText::rebuildRichText( rt, *rt->getRichTextPtr() );
	rt->getRichTextPtr()->updateLayout();

	const RichText::InlineFragment* boxFragment = nullptr;
	for ( const auto& fragment : rt->getRichText().getInlineFragments() ) {
		if ( fragment.type == RichText::InlineFragment::Type::Box &&
			 fragment.source.type == RichText::InlineSourceType::Widget &&
			 fragment.source.ptr == outerSpan ) {
			boxFragment = &fragment;
			break;
		}
	}

	ASSERT_TRUE( boxFragment != nullptr );
	ASSERT_TRUE( boxFragment->backgroundDrawable != nullptr );
	EXPECT_TRUE( boxFragment->backgroundDrawableUsesFragmentColor );
	EXPECT_EQ( boxFragment->backgroundColor.getValue(), backgroundColor.getValue() );
	EXPECT_EQ( boxFragment->backgroundDrawable->getDrawableType(), Drawable::UIBACKGROUNDDRAWABLE );

	auto* background = static_cast<UIBackgroundDrawable*>( boxFragment->backgroundDrawable );
	EXPECT_TRUE( background->hasRadius() );
	EXPECT_NEAR( background->getRadiuses().topLeft.x, 2.f, 0.001f );
	EXPECT_NEAR( background->getRadiuses().topRight.x, 2.f, 0.001f );
	EXPECT_NEAR( background->getRadiuses().bottomLeft.x, 2.f, 0.001f );
	EXPECT_NEAR( background->getRadiuses().bottomRight.x, 2.f, 0.001f );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, DefaultStyleInheritance ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
	    <RichText id="rt" font-size="24dp" color="#FF0000" layout_width="300dp" layout_height="wrap_content">Default size<span font-size="16dp" color="#00FF00">Small</span></RichText>
    )xml";

	sceneNode->loadLayoutFromString( xml );

	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// force layout
	sceneNode->update( Time::Zero );

	auto graphicsRt = rt->getRichText();
	const auto& lines = graphicsRt.getLines();

	// spans[0] should be "Default size" with parent's size and color
	// spans[1] should be "Small" with overridden size and color
	ASSERT_FALSE( lines.empty() );
	ASSERT_TRUE( lines.front().spans.size() >= 2 );

	ASSERT_EQ( lines.front().spans[0].type, RichText::RenderSpan::Type::Text );
	auto text0 = lines.front().spans[0].text;
	ASSERT_TRUE( text0 != nullptr );
	EXPECT_EQ( text0->getCharacterSize(), rt->getFontSize() );
	EXPECT_EQ( text0->getFillColor().getValue(), rt->getFontColor().getValue() );
	EXPECT_EQ( text0->getFillColor().getValue(), Color::fromString( "#FF0000" ).getValue() );

	ASSERT_EQ( lines.front().spans[1].type, RichText::RenderSpan::Type::Text );
	auto text1 = lines.front().spans[1].text;
	ASSERT_TRUE( text1 != nullptr );
	EXPECT_EQ( text1->getCharacterSize(), (unsigned int)PixelDensity::dpToPxI( 16 ) );
	EXPECT_EQ( text1->getFillColor().getValue(), Color::fromString( "#00FF00" ).getValue() );

	destroyRichTextScene( sceneNode );
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
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhitespaceCollapseTest ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhitespaceCollapseCodeTest ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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
		if ( child->isTextNode() ) {
			UI::UITextNode* span = static_cast<UI::UITextNode*>( child );
			if ( span->getText() == ". " ) {
				foundDotSpace = true;
			}
		}
		child = child->getNextNode();
	}

	EXPECT_TRUE( foundDotSpace );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhiteSpaceCollapsePreserveSpacesAndBreaks ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<RichText id="rt" white-space-collapse="preserve" white-space="pre-wrap">alpha  beta
    gamma</RichText>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	const auto& richText = *rt->getRichTextPtr();
	EXPECT_EQ( richText.getLines().size(), (size_t)2 );
	EXPECT_STRINGEQ( richTextLineText( richText, 0 ), "alpha  beta" );
	EXPECT_STRINGEQ( richTextLineText( richText, 1 ), "    gamma" );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhiteSpaceCollapsePreserveBreaks ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<RichText id="rt" white-space-collapse="preserve-breaks">alpha   beta
    gamma</RichText>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	const auto& richText = *rt->getRichTextPtr();
	EXPECT_EQ( richText.getLines().size(), (size_t)2 );
	EXPECT_STRINGEQ( richTextLineText( richText, 0 ), "alpha beta" );
	EXPECT_STRINGEQ( richTextLineText( richText, 1 ), " gamma" );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhiteSpaceCollapsePreserveSpaces ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<RichText id="rt" white-space-collapse="preserve-spaces">alpha	 beta
    gamma</RichText>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_STRINGEQ( richTextRenderedText( *rt->getRichTextPtr() ), "alpha  beta     gamma" );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhiteSpaceCollapsePreserveSpacesNormalizesCRLF ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<RichText id="rt" white-space-collapse="preserve-spaces">alpha&#13;&#10;beta</RichText>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_STRINGEQ( richTextRenderedText( *rt->getRichTextPtr() ), "alpha beta" );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhiteSpaceCollapseDiscard ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<RichText id="rt" white-space-collapse="discard">alpha 	 beta
    gamma</RichText>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_STRINGEQ( richTextRenderedText( *rt->getRichTextPtr() ), "alphabetagamma" );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhiteSpaceCollapseAcrossInlineSpanBoundaries ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<RichText id="rt">alpha <span> beta</span> gamma</RichText>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	UI::UIRichText* rt = sceneNode->find<UI::UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_STRINGEQ( richTextRenderedText( *rt->getRichTextPtr() ), "alpha beta gamma" );

	destroyRichTextScene( sceneNode );
}

UTEST( RichText, BreakSpacesWrapsAfterPreservedSpacesAndTabs ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "RichText Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font->loaded() );
	FontFamily::loadFromRegular( font );

	FontStyleConfig style;
	style.Font = font;
	style.CharacterSize = 16;
	auto textWidth = [&style]( const String& string, Uint32 tabWidth = 8 ) {
		return Text::getTextWidth( string, style, tabWidth, TextHints::AllAscii );
	};

	RichText spacedText;
	spacedText.setFontStyleConfig( style );
	spacedText.setWhiteSpaceWrapMode( RichText::WhiteSpaceWrapMode::BreakSpaces );
	spacedText.addSpan( "aa   bb" );
	spacedText.setMaxWidth( textWidth( "aa   " ) + 1.f );
	spacedText.getSize();
	ASSERT_GE( spacedText.getLines().size(), (size_t)2 );
	EXPECT_STRINGEQ( richTextLineText( spacedText, 0 ), "aa   " );
	EXPECT_STRINGEQ( richTextLineText( spacedText, 1 ), "bb" );
	EXPECT_GT( spacedText.getLines()[0].width, textWidth( "aa" ) );

	RichText onlySpaces;
	onlySpaces.setFontStyleConfig( style );
	onlySpaces.setWhiteSpaceWrapMode( RichText::WhiteSpaceWrapMode::BreakSpaces );
	onlySpaces.addSpan( "     " );
	onlySpaces.setMaxWidth( textWidth( "   " ) + 1.f );
	onlySpaces.getSize();
	ASSERT_GE( onlySpaces.getLines().size(), (size_t)2 );
	EXPECT_STRINGEQ( richTextLineText( onlySpaces, 0 ), "   " );
	EXPECT_STRINGEQ( richTextLineText( onlySpaces, 1 ), "  " );

	RichText tabText;
	tabText.setFontStyleConfig( style );
	tabText.setTabWidth( 4 );
	tabText.setWhiteSpaceWrapMode( RichText::WhiteSpaceWrapMode::BreakSpaces );
	tabText.addSpan( "a\tb" );
	tabText.setMaxWidth( textWidth( "a\t", 4 ) + 1.f );
	tabText.getSize();
	ASSERT_GE( tabText.getLines().size(), (size_t)2 );
	EXPECT_STRINGEQ( richTextLineText( tabText, 0 ), "a\t" );
	EXPECT_STRINGEQ( richTextLineText( tabText, 1 ), "b" );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, basicLayout ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UIHTMLTable, cellHeightIsMinimumForInlineContentHitTest ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<table id="table" layout_width="200dp" layout_height="wrap_content">
			<tr>
				<td id="cell" style="height: 10px; line-height: 24px">
					<a id="link" href="#">Link</a>
				</td>
			</tr>
		</table>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto* table = sceneNode->find<UIHTMLTable>( "table" );
	auto* cell = sceneNode->find<UIHTMLTableCell>( "cell" );
	auto* link = sceneNode->find<UIAnchorSpan>( "link" );
	ASSERT_TRUE( table != nullptr );
	ASSERT_TRUE( cell != nullptr );
	ASSERT_TRUE( link != nullptr );
	ASSERT_FALSE( link->getHitBoxes().empty() );

	const Rectf& hitBox = link->getHitBoxes().front();
	EXPECT_GT( cell->getPixelsSize().getHeight(), 10.f );
	EXPECT_GE( cell->getPixelsSize().getHeight(), link->getPixelsPosition().y + hitBox.Bottom );

	Vector2f hitPos = link->convertToWorldSpace(
		{ hitBox.Left + 1.f, eemax( hitBox.Top, hitBox.Bottom - 1.f ) } );
	Node* hitNode = table->overFind( hitPos );
	EXPECT_EQ( hitNode, link );

	destroyRichTextScene( sceneNode );
}

UTEST( UIHTMLTable, cellHeightStretchesToRowMinimum ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<table id="table" layout_width="200dp" layout_height="wrap_content">
			<tr>
				<td id="logo"><img id="img" style="width: 18px; height: 18px; display: block" /></td>
				<td id="nav" style="height: 10px">Nav</td>
			</tr>
		</table>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto* logo = sceneNode->find<UIHTMLTableCell>( "logo" );
	auto* nav = sceneNode->find<UIHTMLTableCell>( "nav" );
	ASSERT_TRUE( logo != nullptr );
	ASSERT_TRUE( nav != nullptr );

	EXPECT_GT( logo->getPixelsSize().getHeight(), 10.f );
	EXPECT_EQ( nav->getPixelsSize().getHeight(), logo->getPixelsSize().getHeight() );

	destroyRichTextScene( sceneNode );
}

UTEST( UIHTMLTable, tableCellAnchorHoverRelayoutsRichText ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<table id="table" layout_width="200dp" layout_height="wrap_content">
			<style>
				a { text-decoration: none; }
				a:hover { text-decoration: underline; }
			</style>
			<tr>
				<td id="logo"><img id="img" style="width: 18px; height: 18px; display: block" /></td>
				<td id="nav" style="height: 10px"><a id="link" href="#">Nav</a></td>
			</tr>
		</table>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto* nav = sceneNode->find<UIHTMLTableCell>( "nav" );
	auto* link = sceneNode->find<UIAnchorSpan>( "link" );
	ASSERT_TRUE( nav != nullptr );
	ASSERT_TRUE( link != nullptr );
	ASSERT_FALSE( link->getHitBoxes().empty() );
	auto renderedAnchorStyle = [&nav]() -> Uint32 {
		for ( const auto& line : nav->getRichText().getLines() ) {
			for ( const auto& span : line.spans ) {
				if ( span.type == RichText::RenderSpan::Type::Text && span.text &&
					 span.text->getString() == "Nav" )
					return span.text->getStyle();
			}
		}
		return 0;
	};

	EXPECT_FALSE( ( renderedAnchorStyle() & Text::Underlined ) != 0 );
	EXPECT_GT( nav->getPixelsSize().getHeight(), 10.f );

	const Rectf& hitBox = link->getHitBoxes().front();
	Vector2f hoverPos = link->convertToWorldSpace( { hitBox.Left + 1.f, hitBox.Top + 1.f } );
	EXPECT_EQ( sceneNode->overFind( hoverPos ), link );

	sceneNode->getEventDispatcher()->getInput()->setMousePos( hoverPos.asInt() );
	sceneNode->update( Time::Zero );

	EXPECT_TRUE( ( renderedAnchorStyle() & Text::Underlined ) != 0 );
	EXPECT_GT( nav->getPixelsSize().getHeight(), 10.f );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, WhitespaceCollapseBRTest ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, MarginsTest ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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
	// Block elements each occupy their own line; d2 sits below d1 at the same x.
	// d1 footprint width: 40 (left) + 50 (width) + 20 (right) = 110.
	// d1 footprint height: 10 + 50 + 30 = 90.
	// d2 x = d1 left margin = 5 (its own left margin).
	EXPECT_EQ( 5.f, pos2.x );
	// d2 y = d1 footprint height (90) + d2 margin top (5) = 95.
	EXPECT_EQ( 95.f, pos2.y );

	// Check UIRichText bounds
	// Width = max(d1 footprint: 110, d2 footprint: 60) = 110.
	// Height = sum of line heights: d1 line 90 + d2 line 60 = 150.
	EXPECT_EQ( 110.f, rt->getPixelsSize().getWidth() );
	EXPECT_EQ( 150.f, rt->getPixelsSize().getHeight() );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, ForcedLineBreak ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(<richtext id="rt">Line 1<br/>Line 2</richtext>)xml";

	sceneNode->loadLayoutFromString( xml );
	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	sceneNode->update( Time::Zero );

	const auto& richText = rt->getRichText();
	EXPECT_EQ( richText.getLines().size(), (size_t)3 );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, CustomBRHeight ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, MinMaxWidth ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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
	EXPECT_GT( rtMax->getSize().getHeight(),
			   PixelDensity::dpToPx( 30 ) ); // should wrap to multiple lines
	EXPECT_LE( rtMaxFixed->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( rtMaxFixed->getSize().getHeight(),
			   PixelDensity::dpToPx( 30 ) ); // should wrap to multiple lines

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, MinMaxWidthChildren ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	EXPECT_LE( rtParent->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( rtParent->getSize().getWidth(), 0 ); // Assert it's not 0
	EXPECT_EQ( childWidget->getSize().getWidth(), rtParent->getSize().getWidth() );
	EXPECT_LE( childWidget->getSize().getWidth(), PixelDensity::dpToPx( 100 ) );
	EXPECT_GT( childWidget->getSize().getWidth(), 0 ); // Assert it's not 0

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, MatchParentChildPadding ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	destroyRichTextScene( sceneNode );
}

UTEST( UILayout, MinMaxWidthChildren ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

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

	EXPECT_LE( llParent->getSize().getWidth(), PixelDensity::dpToPx( 150 ) );
	EXPECT_GT( llParent->getSize().getWidth(), 0 ); // Assert it's not 0
	EXPECT_EQ( childWidget2->getSize().getWidth(), llParent->getSize().getWidth() );
	EXPECT_LE( childWidget2->getSize().getWidth(), PixelDensity::dpToPx( 150 ) );
	EXPECT_GT( childWidget2->getSize().getWidth(), 0 ); // Assert it's not 0

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InvalidWidthLengthComputation ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<ScrollView id="html_view" layout_width="match_parent" layout_height="match_parent" />
	)xml";

	String html = R"html(
		<!doctype html>
		<html lang="en">
		  <body>
		    <div>
		      <div id="anchor_parent">
		        <a id="anchor" href="#">
		          <div></div>
		          <div>
		            <div>
		              <h2>No, I Won't Download Your App. The Web Version is A-OK.</h2>
		            </div>
		          </div>
		        </a>
		      </div>
		      <footer class="site-footer">
		        <a href="/">← Return to System</a>
		      </footer>
		    </div>
		  </body>
		</html>
	)html";

	sceneNode->loadLayoutFromString( xml );
	auto htmlView = sceneNode->find( "html_view" );
	ASSERT_TRUE( htmlView != nullptr );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ), htmlView );
	auto parent = sceneNode->find<UIWidget>( "anchor_parent" );
	auto anchor = sceneNode->find<UIWidget>( "anchor" );
	ASSERT_TRUE( parent != nullptr );
	ASSERT_TRUE( anchor != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_LE( anchor->getSize().getWidth(), parent->getSize().getWidth() );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InvalidWidthLengthComputation2 ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<ScrollView id="html_view" layout_width="match_parent" layout_height="match_parent" />
	)xml";

	String html = R"html(<!doctype html>
		<html lang="en">
		  <body>
		    <div class="container">
		      <div id="anchor_parent">
		        <a id="anchor" href="#">
		          <div id="anchor_div">
		              <h2 id="anchor_h2">
		                No, I Won't Download Your App. The Web Version is A-OK.
		              </h2>
		              <span id="anchor_span">Apr 6, 2026</span>
		          </div>
		        </a>
		      </div>
		    </div>
		  </body>
		</html>
	)html";

	sceneNode->loadLayoutFromString( xml );
	auto htmlView = sceneNode->find( "html_view" );
	ASSERT_TRUE( htmlView != nullptr );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ), htmlView );
	auto parent = sceneNode->find<UIWidget>( "anchor_parent" );
	auto anchor = sceneNode->find<UIWidget>( "anchor" );
	auto anchorDiv = sceneNode->find<UIWidget>( "anchor_div" );
	auto anchorH2 = sceneNode->find<UIWidget>( "anchor_h2" );
	auto anchorSpan = sceneNode->find<UIWidget>( "anchor_span" );
	ASSERT_TRUE( parent != nullptr );
	ASSERT_TRUE( anchor != nullptr );
	ASSERT_TRUE( anchorDiv != nullptr );
	ASSERT_TRUE( anchorH2 != nullptr );
	ASSERT_TRUE( anchorSpan != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_GT( anchor->getSize().getWidth(), 0 );
	EXPECT_GT( anchorDiv->getSize().getWidth(), 0 );
	EXPECT_GT( anchorH2->getSize().getWidth(), 0 );
	EXPECT_GT( anchorSpan->getSize().getWidth(), 0 );
	EXPECT_LE( anchor->getSize().getWidth(), parent->getSize().getWidth() );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InvalidWidthLengthComputation3 ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<ScrollView id="html_view" layout_width="match_parent" layout_height="match_parent" />
	)xml";

	std::string html;
	FileSystem::fileGet( "assets/html/blog_main_incorrect_widths.html", html );

	sceneNode->loadLayoutFromString( xml );
	auto htmlView = sceneNode->find( "html_view" );
	ASSERT_TRUE( htmlView != nullptr );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ), htmlView );

	auto container = sceneNode->getRoot()->querySelector( ".container" );
	auto posts = sceneNode->getRoot()->querySelectorAll( ".post-list > a" );
	auto items = sceneNode->getRoot()->querySelectorAll( ".post-list > a > .post-item-content" );
	auto titles = sceneNode->getRoot()->querySelectorAll(
		".post-list > a > .post-item-content > .post-header-row" );

	ASSERT_TRUE( container != nullptr );
	ASSERT_TRUE( posts.size() > 0 );
	ASSERT_TRUE( items.size() == posts.size() );
	ASSERT_TRUE( items.size() == titles.size() );

	sceneNode->update( Time::Zero );

	for ( size_t i = 0; i < posts.size(); i++ ) {
		auto anchor = posts[i];
		auto item = items[i];
		auto title = titles[i];
		EXPECT_LE( anchor->getPixelsSize().getWidth(), container->getPixelsSize().getWidth() );
		EXPECT_LE( item->getPixelsSize().getWidth(), anchor->getPixelsSize().getWidth() );
		EXPECT_LE( title->getPixelsSize().getWidth(), item->getPixelsSize().getWidth() );
	}

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, InvalidWidthLengthComputation4 ) {
	UIApplication app( { 1280, 720, "eepp - UI HTML Example" } );

	auto ui = app.getUI();

	ui->loadLayoutFromString( R"xml(
		<ScrollView id="html_view" layout_width="match_parent" layout_height="0" layout_weight="1">
			<vbox layout_width="match_parent" layout_height="wrap_content" id="html_doc"></vbox>
		</ScrollView>
	)xml" );

	auto mainContainer = ui->find( "html_doc" );

	bool exit = false;
	ui->runOnMainThread(
		[&] {
			std::string data;
			URI url( "assets/html/blog_main_incorrect_widths.html" );
			FileSystem::fileGet( url.getPath(), data );
			mainContainer->closeAllChildren();
			ui->getStyleSheet().removeAllWithoutMarker( app.getStyleSheetDefaultMarker() );
			ui->setURIFromURL( url );
			auto urlStr = url.toString();
			auto hash = String::hash( urlStr );
			ui->loadLayoutFromString( HTMLFormatter::HTMLtoXML( data ), mainContainer, hash );
			exit = true;
		},
		Seconds( 0.2 ) );

	while ( !exit ) {
		SceneManager::instance()->update();
		app.getWindow()->clear();
		SceneManager::instance()->draw();
		app.getWindow()->display();
	}

	SceneManager::instance()->update();

	auto container = ui->getRoot()->querySelector( ".container" );
	auto posts = ui->getRoot()->querySelectorAll( ".post-list > a" );
	auto items = ui->getRoot()->querySelectorAll( ".post-list > a > .post-item-content" );
	auto titles =
		ui->getRoot()->querySelectorAll( ".post-list > a > .post-item-content > .post-header-row" );

	ASSERT_TRUE( container != nullptr );
	ASSERT_TRUE( posts.size() > 0 );
	ASSERT_TRUE( items.size() == posts.size() );
	ASSERT_TRUE( items.size() == titles.size() );

	ui->update( Time::Zero );

	for ( size_t i = 0; i < posts.size(); i++ ) {
		auto anchor = posts[i];
		auto item = items[i];
		auto title = titles[i];
		EXPECT_GT( anchor->getPixelsSize().getWidth(), 0 );
		EXPECT_GT( item->getPixelsSize().getWidth(), 0 );
		EXPECT_GT( title->getPixelsSize().getWidth(), 0 );
		EXPECT_LE( anchor->getPixelsSize().getWidth(), container->getPixelsSize().getWidth() );
		EXPECT_LE( item->getPixelsSize().getWidth(), anchor->getPixelsSize().getWidth() );
		EXPECT_LE( title->getPixelsSize().getWidth(), item->getPixelsSize().getWidth() );
	}
}

UTEST( UIRichText, DefaultBlockMarginsFromHTML ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String html = R"html(
		<!doctype html>
		<html><body>
			<p id="p">Paragraph</p>
			<h1 id="h1">H1</h1>
			<h2 id="h2">H2</h2>
			<h3 id="h3">H3</h3>
			<h4 id="h4">H4</h4>
			<h5 id="h5">H5</h5>
			<h6 id="h6">H6</h6>
			<pre id="pre">Pre</pre>
			<blockquote id="bq">BQ</blockquote>
			<hr id="hr" />
			<div id="div">Div</div>
		</body></html>
	)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Time::Zero );

	auto checkMargin = [&]( UIWidget* w, const char* name, bool expectNonZero ) {
		ASSERT_TRUE_MSG( w != nullptr, String::format( "%s not found", name ).c_str() );
		Rectf margin = w->getLayoutPixelsMargin();
		if ( expectNonZero ) {
			EXPECT_GT_MSG( margin.Top, 0.f,
						   String::format( "%s margin-top should be > 0", name ).c_str() );
			EXPECT_GT_MSG( margin.Bottom, 0.f,
						   String::format( "%s margin-bottom should be > 0", name ).c_str() );
		} else {
			EXPECT_EQ_MSG( margin.Top, 0.f,
						   String::format( "%s margin-top should be 0", name ).c_str() );
			EXPECT_EQ_MSG( margin.Bottom, 0.f,
						   String::format( "%s margin-bottom should be 0", name ).c_str() );
		}
	};

	checkMargin( sceneNode->find<UIWidget>( "p" ), "p", true );
	checkMargin( sceneNode->find<UIWidget>( "h1" ), "h1", true );
	checkMargin( sceneNode->find<UIWidget>( "h2" ), "h2", true );
	checkMargin( sceneNode->find<UIWidget>( "h3" ), "h3", true );
	checkMargin( sceneNode->find<UIWidget>( "h4" ), "h4", true );
	checkMargin( sceneNode->find<UIWidget>( "h5" ), "h5", true );
	checkMargin( sceneNode->find<UIWidget>( "h6" ), "h6", true );
	checkMargin( sceneNode->find<UIWidget>( "pre" ), "pre", true );
	checkMargin( sceneNode->find<UIWidget>( "bq" ), "blockquote", true );
	checkMargin( sceneNode->find<UIWidget>( "hr" ), "hr", true );
	checkMargin( sceneNode->find<UIWidget>( "div" ), "div", false );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, DefaultBlockMarginsCssReset ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String html = R"html(
		<!doctype html>
		<html><body>
			<p id="p">Paragraph</p>
			<h1 id="h1">H1</h1>
			<h2 id="h2">H2</h2>
			<blockquote id="bq">BQ</blockquote>
			<hr id="hr" />
		</body></html>
	)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->combineStyleSheet( "* { margin: 0; }" );
	sceneNode->update( Time::Zero );

	auto checkZeroMargin = [&]( UIWidget* w, const char* name ) {
		ASSERT_TRUE_MSG( w != nullptr, String::format( "%s not found", name ).c_str() );
		Rectf margin = w->getLayoutPixelsMargin();
		EXPECT_EQ_MSG( margin.Top, 0.f,
					   String::format( "%s margin-top should be 0 after reset", name ).c_str() );
		EXPECT_EQ_MSG( margin.Bottom, 0.f,
					   String::format( "%s margin-bottom should be 0 after reset", name ).c_str() );
	};

	checkZeroMargin( sceneNode->find<UIWidget>( "p" ), "p" );
	checkZeroMargin( sceneNode->find<UIWidget>( "h1" ), "h1" );
	checkZeroMargin( sceneNode->find<UIWidget>( "h2" ), "h2" );
	checkZeroMargin( sceneNode->find<UIWidget>( "bq" ), "blockquote" );
	checkZeroMargin( sceneNode->find<UIWidget>( "hr" ), "hr" );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, DefaultBlockMarginsOlUl ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String html = R"html(
		<!doctype html>
		<html><body>
			<ol id="ol"><li>Ordered item</li></ol>
			<ul id="ul"><li>Unordered item</li></ul>
		</body></html>
	)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Time::Zero );

	UIWidget* ol = sceneNode->find<UIWidget>( "ol" );
	ASSERT_TRUE( ol != nullptr );
	Rectf olMargin = ol->getLayoutPixelsMargin();
	EXPECT_GT( olMargin.Top, 0.f );
	EXPECT_GT( olMargin.Bottom, 0.f );

	UIWidget* ul = sceneNode->find<UIWidget>( "ul" );
	ASSERT_TRUE( ul != nullptr );
	Rectf ulMargin = ul->getLayoutPixelsMargin();
	EXPECT_GT( ulMargin.Top, 0.f );
	EXPECT_GT( ulMargin.Bottom, 0.f );

	EXPECT_EQ( olMargin.Top, ulMargin.Top );
	EXPECT_EQ( olMargin.Bottom, ulMargin.Bottom );

	destroyRichTextScene( sceneNode );
}

UTEST( UIRichText, DefaultBlockMarginsOlUlCssReset ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String html = R"html(
		<!doctype html>
		<html><body>
			<ol id="ol"><li>Item</li></ol>
			<ul id="ul"><li>Item</li></ul>
		</body></html>
	)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->combineStyleSheet( "* { margin: 0; }" );
	sceneNode->update( Time::Zero );

	UIWidget* ol = sceneNode->find<UIWidget>( "ol" );
	ASSERT_TRUE( ol != nullptr );
	Rectf olMargin = ol->getLayoutPixelsMargin();
	EXPECT_EQ( olMargin.Top, 0.f );
	EXPECT_EQ( olMargin.Bottom, 0.f );

	UIWidget* ul = sceneNode->find<UIWidget>( "ul" );
	ASSERT_TRUE( ul != nullptr );
	Rectf ulMargin = ul->getLayoutPixelsMargin();
	EXPECT_EQ( ulMargin.Top, 0.f );
	EXPECT_EQ( ulMargin.Bottom, 0.f );

	destroyRichTextScene( sceneNode );
}
