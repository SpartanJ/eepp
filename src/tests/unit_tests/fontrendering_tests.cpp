#include "compareimages.hpp"
#include "utest.hpp"

#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/fontbmfont.hpp>
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontsprite.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderergl.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::UI;
using namespace EE::UI::CSS;

UTEST( FontRendering, drawingEmptyTextDoesNotCreateFontPage ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Empty Text Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	FontDesc desc;
	desc.family = "Empty Text Test";
	desc.path = Sys::getProcessPath() + "assets/fonts/NotoSansKR-Regular.ttf";
	FontTrueTypePtr font = app.getUI()->getResourceScope()->getFontService().loadSystemFont( desc );
	ASSERT_TRUE( font );

	TextureFactory* textureFactory = TextureFactory::instance();
	const Uint32 textureCount = textureFactory->getTextureCount();
	Text::draw( String{}, Vector2f::Zero, font.get(), 10, Color::White );

	EXPECT_EQ( textureCount, textureFactory->getTextureCount() );
}

UTEST( FontRendering, glyphAdvanceDoesNotCreateTexturePages ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Glyph Advance Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	ResourceScope& scope = *app.getUI()->getResourceScope();
	FontTrueTypePtr font = FontTrueType::New( "GlyphAdvance-Regular", scope );
	ASSERT_TRUE(
		font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );

	TextureFactory* textureFactory = TextureFactory::instance();
	const Uint32 textureCount = textureFactory->getTextureCount();
	const Float advance = font->getGlyphAdvance( ' ', 10 );
	const Float outlinedAdvance = font->getGlyphAdvance( ' ', 10, false, false, 2.f );
	EXPECT_TRUE( advance > 0 );
	EXPECT_EQ( advance, outlinedAdvance );
	EXPECT_EQ( textureCount, textureFactory->getTextureCount() );
}

UTEST( FontRendering, subpixelCoverageCompositesPerChannel ) {
	UIApplication app(
		WindowSettings( 360, 220, "eepp - Subpixel Text Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	ResourceScope& scope = *app.getUI()->getResourceScope();
	FontTrueTypePtr font = FontTrueType::New( "SubpixelText-Regular", scope );
	ASSERT_TRUE(
		font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );
	const Float grayscaleLAdvance = font->getGlyphAdvance( 'l', 28 );
	const Float grayscaleDAdvance = font->getGlyphAdvance( 'd', 28 );
	const Float grayscaleKerning = font->getKerning( 'i', 'd', 28, false, false );
	UIIconPtr icon = UIGlyphIcon::New( "glyph-cache-test", font.get(), 'S' );
	const DrawablePtr grayscaleIcon = icon->getSource( 28 );
	ASSERT_TRUE( grayscaleIcon );
	ASSERT_EQ( GlyphRenderMode::Mask,
			   static_cast<GlyphDrawable*>( grayscaleIcon.get() )->getGlyphRenderMode() );
	font->setAntialiasing( FontAntialiasing::Subpixel );
	EXPECT_EQ( grayscaleLAdvance, font->getGlyphAdvance( 'l', 28 ) );
	EXPECT_EQ( grayscaleDAdvance, font->getGlyphAdvance( 'd', 28 ) );
	EXPECT_EQ( grayscaleKerning, font->getKerning( 'i', 'd', 28, false, false ) );
	const DrawablePtr subpixelIcon = icon->getSource( 28 );
	ASSERT_TRUE( subpixelIcon );
	EXPECT_NE( grayscaleIcon.get(), subpixelIcon.get() );
	EXPECT_EQ( GlyphRenderMode::Subpixel,
			   static_cast<GlyphDrawable*>( subpixelIcon.get() )->getGlyphRenderMode() );

	GlyphDrawable* drawable = font->getGlyphDrawable( 'S', 28 );
	ASSERT_TRUE( drawable );
	ASSERT_EQ( GlyphRenderMode::Subpixel, drawable->getGlyphRenderMode() );

	EE::Window::Window* window = app.getWindow();
	window->setClearColor( Color::White );
	window->clear();
	Text::draw( String( "Subpixel static" ), { 8.f, 4.f }, font.get(), 28, Color::Black );

	Text retained( "Subpixel retained", font.get(), 28 );
	retained.setFillColor( Color::Black );
	retained.draw( 8.f, 52.f );

	Primitives primitives;
	primitives.setColor( Color( 40, 42, 54 ) );
	primitives.drawRectangle( Rectf( Vector2f( 0.f, 110.f ), Sizef( 360.f, 110.f ) ) );
	const Color lightText( 248, 248, 242 );
	Text::draw( String( "Subpixel static light" ), { 8.f, 114.f }, font.get(), 28, lightText );
	retained.setString( "Subpixel retained light" );
	retained.setFillColor( lightText );
	retained.draw( 8.f, 162.f );

	Image image = window->getFrontBufferImage();
	auto hasColoredCoverage = [&image]( Uint32 top, Uint32 bottom ) {
		for ( Uint32 y = top; y < bottom; ++y ) {
			for ( Uint32 x = 0; x < image.getWidth(); ++x ) {
				Color pixel = image.getPixel( x, y );
				if ( eeabs( static_cast<Int32>( pixel.r ) - static_cast<Int32>( pixel.g ) ) > 3 ||
					 eeabs( static_cast<Int32>( pixel.g ) - static_cast<Int32>( pixel.b ) ) > 3 )
					return true;
			}
		}
		return false;
	};

	EXPECT_TRUE_MSG( hasColoredCoverage( 0, image.getHeight() / 2 ),
					 "Static text lost independent LCD channel coverage" );
	EXPECT_TRUE_MSG( hasColoredCoverage( image.getHeight() / 2, image.getHeight() ),
					 "Retained text lost independent LCD channel coverage" );
	compareImages( utest_state, utest_result, window, "eepp-subpixel-text" );

	FrameBufferUniquePtr frameBuffer = FrameBuffer::New( 240, 48, false, false, false, 4, window );
	ASSERT_TRUE( frameBuffer && frameBuffer->created() );
	frameBuffer->setClearColor( ColorAf( 0.f, 0.f, 0.f, 0.f ) );
	frameBuffer->bind();
	frameBuffer->clear();
	Text::draw( String( "Transparent subpixel" ), { 4.f, 4.f }, font.get(), 28, Color::White );
	GlobalBatchRenderer::instance()->draw();
	std::vector<Uint8> pixels( frameBuffer->getWidth() * frameBuffer->getHeight() * 4 );
	GLi->readPixels( 0, 0, frameBuffer->getWidth(), frameBuffer->getHeight(), pixels.data() );
	frameBuffer->unbind();
	bool hasCoverageAlpha = false;
	for ( size_t i = 3; i < pixels.size(); i += 4 ) {
		if ( pixels[i] != 0 ) {
			hasCoverageAlpha = true;
			break;
		}
	}
	EXPECT_TRUE_MSG( hasCoverageAlpha,
					 "Subpixel text did not update a transparent target's alpha" );
}

UTEST( FontRendering, scaledSubpixelGlyphAtlas ) {
	UIApplication app(
		WindowSettings( 256, 64, "eepp - Scaled Subpixel Glyph Atlas", VisualTestWindowStyle,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	ResourceScope& scope = *app.getUI()->getResourceScope();
	FontTrueTypePtr font = FontTrueType::New( "ScaledSubpixelNonicons", scope );
	ASSERT_TRUE( font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/nonicons.ttf" ) );
	font->setAntialiasing( FontAntialiasing::Subpixel );
	font->setIsEmojiFont( true );

	EE::Window::Window* window = app.getWindow();
	window->setClearColor( Color( 40, 44, 52 ) );
	window->clear();
	const std::array<Uint32, 8> codePoints = { 61718, 61719, 61720, 61743,
											   61752, 61775, 61789, 61799 };
	Float x = 8.f;
	for ( Uint32 codePoint : codePoints ) {
		GlyphDrawable* glyph = font->getGlyphDrawable( codePoint, 18 );
		ASSERT_TRUE( glyph );
		ASSERT_EQ( GlyphRenderMode::Subpixel, glyph->getGlyphRenderMode() );
		const Sizef size = glyph->getPixelsSize();
		glyph->setColor( Color::White );
		glyph->draw( { std::trunc( x + ( 24.f - size.getWidth() ) * 0.5f ),
					   std::trunc( ( 64.f - size.getHeight() ) * 0.5f ) } );
		x += 30.f;
	}
	compareImages( utest_state, utest_result, window, "eepp-scaled-subpixel-glyph-atlas" );
}

UTEST( FontRendering, loadingFontFamilyDoesNotCreateTexturePages ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Font Family Metrics Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	ResourceScope& scope = *app.getUI()->getResourceScope();
	FontTrueTypePtr font = FontTrueType::New( "FontFamilyMetrics-Regular", scope );
	ASSERT_TRUE(
		font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );

	TextureFactory* textureFactory = TextureFactory::instance();
	const Uint32 textureCount = textureFactory->getTextureCount();

	FontFamily::loadFromRegular( font.get() );
	EXPECT_TRUE( font->hasBold() );
	EXPECT_TRUE( font->hasItalic() );
	EXPECT_EQ( textureCount, textureFactory->getTextureCount() );
}

UTEST( FontRendering, regularFontOwnsRelatedFonts ) {
	FontTrueTypePtr font = FontTrueType::New( "RelatedFonts-Regular" );
	FontTrueTypePtr bold = FontTrueType::New( "RelatedFonts-Bold" );
	FontTrueTypePtr italic = FontTrueType::New( "RelatedFonts-Italic" );
	FontTrueTypePtr boldItalic = FontTrueType::New( "RelatedFonts-BoldItalic" );
	FontTrueTypeWeakPtr weakBold = bold;
	FontTrueTypeWeakPtr weakItalic = italic;
	FontTrueTypeWeakPtr weakBoldItalic = boldItalic;

	font->setBoldFont( bold );
	font->setItalicFont( italic );
	font->setBoldItalicFont( boldItalic );

	defaultResourceScope().eraseLocalFont( bold.get() );
	defaultResourceScope().eraseLocalFont( italic.get() );
	defaultResourceScope().eraseLocalFont( boldItalic.get() );
	bold.reset();
	italic.reset();
	boldItalic.reset();

	EXPECT_TRUE( font->getBoldFont() );
	EXPECT_TRUE( font->getItalicFont() );
	EXPECT_TRUE( font->getBoldItalicFont() );
	EXPECT_FALSE( weakBold.expired() );
	EXPECT_FALSE( weakItalic.expired() );
	EXPECT_FALSE( weakBoldItalic.expired() );

	defaultResourceScope().eraseLocalFont( font.get() );
	font.reset();

	EXPECT_TRUE( weakBold.expired() );
	EXPECT_TRUE( weakItalic.expired() );
	EXPECT_TRUE( weakBoldItalic.expired() );
}

UTEST( FontRendering, destroyingFontInvalidatesTextLayoutCache ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Text Layout Cache Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	ResourceScope& scope = *app.getUI()->getResourceScope();
	FontTrueTypePtr font = FontTrueType::New( "TextLayoutCache-Regular", scope );
	FontTrueTypePtr retainedFont = FontTrueType::New( "TextLayoutCache-Retained", scope );
	ASSERT_TRUE(
		font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );
	ASSERT_TRUE( retainedFont->loadFromFile( Sys::getProcessPath() +
											 "../assets/fonts/NotoSans-Regular.ttf" ) );

	TextLayout::Cache layout =
		TextLayout::layout( String( "cached shaped text" ), font.get(), 14, Text::Regular );
	TextLayout::Cache retainedLayout = TextLayout::layout( String( "unrelated cached text" ),
														   retainedFont.get(), 14, Text::Regular );
	std::weak_ptr<const TextLayout> layoutWeak = layout;
	std::weak_ptr<const TextLayout> retainedLayoutWeak = retainedLayout;
	layout.reset();
	retainedLayout.reset();
	EXPECT_FALSE( layoutWeak.expired() );
	EXPECT_FALSE( retainedLayoutWeak.expired() );

	EXPECT_TRUE( scope.eraseLocalFont( font.get() ) );
	font.reset();

	EXPECT_TRUE( layoutWeak.expired() );
	EXPECT_FALSE( retainedLayoutWeak.expired() );
}

UTEST( FontRendering, fontsTest ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "eepp - Fonts", VisualTestWindowStyle, WindowBackend::Default,
						32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	UTEST_PRINT_INFO( GLi->getRenderer().c_str() );

	win->setClearColor( RGB( 230, 230, 230 ) );

	String Txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod "
				"tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
				"quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo "
				"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse "
				"cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non "
				"proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

	FontTrueType* fontTest = FontTrueType::New( "DejaVuSansMono" ).get();
	fontTest->loadFromFile( "../assets/fonts/DejaVuSansMono.ttf" );

	FontTrueType* fontTest2 = FontTrueType::New( "NotoSans-Regular" ).get();
	fontTest2->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	FontTrueType* fontEmoji = FontTrueType::New( "NotoEmoji-Regular" ).get();
	fontEmoji->loadFromFile( "../assets/fonts/NotoEmoji-Regular.ttf" );

	FontTrueType* fontEmojiColor = FontTrueType::New( "NotoColorEmoji" ).get();
	fontEmojiColor->loadFromFile( "../assets/fonts/NotoColorEmoji.ttf" );

	FontBMFont* fontBMFont = FontBMFont::New( "bmfont" ).get();
	fontBMFont->loadFromFile( "../assets/fonts/bmfont.fnt" );

	FontSprite* fontSprite = FontSprite::New( "alagard" ).get();
	fontSprite->loadFromFile( "../assets/fonts/custom_alagard.png", Color::Fuchsia, 32, -4 );

	Text text;
	text.setFont( fontTest );
	text.setFontSize( 24 );
	text.setAlign( TEXT_ALIGN_CENTER );
	text.setString( Txt );
	text.hardWrapText( win->getWidth() - 96 );

	int size = (int)text.getString().size();

	for ( int i = 0; i < size; i++ ) {
		text.setFillColor( Color( 255 * i / size, 0, 0, 255 ), i, i + 1 );
	}

	Text text2;
	text2.setFont( fontTest2 );
	text2.setString( "Lorem ipsum dolor sit amet, consectetur adipisicing elit. 👽" );
	text2.setFontSize( 32 );
	text2.setFillColor( Color::Black );

	Text text3;
	text3.setFont( fontTest );
	text3.setString( text2.getString() );
	text3.setFontSize( 24 );
	text3.setFillColor( Color( 255, 255, 255, 255 ) );
	text3.setOutlineThickness( 2 );
	text3.setOutlineColor( Color( 0, 0, 0, 255 ) );

	Text text4;
	text4.setFont( fontBMFont );
	text4.setString( text2.getString() );
	text4.setFontSize( 45 );
	text4.setFillColor( Color::Black );

	Text text5;
	text5.setFont( fontSprite );
	text5.setString( text2.getString() );
	text5.setFontSize( 38 );

	Text text6;
	text6.setFont( fontEmojiColor );
	text6.setFontSize( 64 );
	text6.setString( "👽 😀 💩 😃 👻" );

	Text text7;
	text7.setFont( fontEmoji );
	text7.setFontSize( 32 );
	text7.setString( "👽 😀 💩 😃 👻" );
	text7.setFillColor( Color::Gray );
	text7.setOutlineThickness( 2 );
	text7.setOutlineColor( Color( 0, 0, 0, 255 ) );

	{
		const auto runTest = [&]() {
			win->clear();

			Float offsetY = 0;
			text.draw( 0, 0 );
			text2.draw( 0, ( offsetY += text.getTextHeight() + 16 ) );
			text7.draw( 0, ( offsetY += text2.getTextHeight() + 16 ) );
			text3.draw( 0, ( offsetY += text7.getTextHeight() + 16 ) );
			text4.draw( 0, ( offsetY += text3.getTextHeight() + 16 ) );
			text5.draw( 0, ( offsetY += text4.getTextHeight() + 16 ) );
			text6.draw( 0, ( offsetY += text5.getTextHeight() + 16 ) );

			compareImages( utest_state, utest_result, win, "eepp-fonts" );
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
}

UTEST( FontRendering, fontWeighToStringRoundtrip ) {
	std::string s;
	s = Text::fontWeightToString( FontWeight::Bold );
	EXPECT_STREQ( "bold", s.c_str() );
	s = Text::fontWeightToString( FontWeight::Normal );
	EXPECT_STREQ( "normal", s.c_str() );
	s = Text::fontWeightToString( FontWeight::Light );
	EXPECT_STREQ( "light", s.c_str() );
	s = Text::fontWeightToString( FontWeight::Medium );
	EXPECT_STREQ( "medium", s.c_str() );
	s = Text::fontWeightToString( FontWeight::SemiBold );
	EXPECT_STREQ( "semi-bold", s.c_str() );
	s = Text::fontWeightToString( FontWeight::ExtraBold );
	EXPECT_STREQ( "extra-bold", s.c_str() );
	s = Text::fontWeightToString( FontWeight::Black );
	EXPECT_STREQ( "black", s.c_str() );
	s = Text::fontWeightToString( FontWeight::Thin );
	EXPECT_STREQ( "thin", s.c_str() );
	s = Text::fontWeightToString( FontWeight::ExtraLight );
	EXPECT_STREQ( "extra-light", s.c_str() );
}

UTEST( FontRendering, stringToFontWeight ) {
	EXPECT_EQ( FontWeight::Bold, Text::stringToFontWeight( "bold" ) );
	EXPECT_EQ( FontWeight::Normal, Text::stringToFontWeight( "normal" ) );
	EXPECT_EQ( FontWeight::Light, Text::stringToFontWeight( "light" ) );
	EXPECT_EQ( FontWeight::Medium, Text::stringToFontWeight( "medium" ) );
	EXPECT_EQ( FontWeight::SemiBold, Text::stringToFontWeight( "semi-bold" ) );
	EXPECT_EQ( FontWeight::Bold, Text::stringToFontWeight( "700" ) );
	EXPECT_EQ( FontWeight::Normal, Text::stringToFontWeight( "400" ) );
	EXPECT_EQ( FontWeight::Light, Text::stringToFontWeight( "300" ) );
	EXPECT_EQ( FontWeight::Medium, Text::stringToFontWeight( "500" ) );
	EXPECT_EQ( FontWeight::SemiBold, Text::stringToFontWeight( "600" ) );
	EXPECT_EQ( FontWeight::ExtraBold, Text::stringToFontWeight( "800" ) );
	EXPECT_EQ( FontWeight::Black, Text::stringToFontWeight( "900" ) );
	EXPECT_EQ( FontWeight::Thin, Text::stringToFontWeight( "100" ) );
	EXPECT_EQ( FontWeight::ExtraLight, Text::stringToFontWeight( "200" ) );
}

UTEST( FontRendering, stringToStyleFlagWeightMapping ) {
	EXPECT_NE( 0U, Text::stringToStyleFlag( "bold" ) & Text::Bold );
	EXPECT_EQ( 0U, Text::stringToStyleFlag( "normal" ) & Text::Bold );
	EXPECT_EQ( 0U, Text::stringToStyleFlag( "italic" ) & Text::Bold );
	EXPECT_NE( 0U, Text::stringToStyleFlag( "italic" ) & Text::Italic );

	EXPECT_EQ( FontWeight::Bold, Text::stringToFontWeight( "bold" ) );
	EXPECT_EQ( FontWeight::Bold, Text::stringToFontWeight( "700" ) );
	EXPECT_EQ( FontWeight::Normal, Text::stringToFontWeight( "400" ) );
	EXPECT_EQ( FontWeight::Light, Text::stringToFontWeight( "300" ) );
	EXPECT_EQ( FontWeight::Medium, Text::stringToFontWeight( "500" ) );
	EXPECT_EQ( FontWeight::SemiBold, Text::stringToFontWeight( "600" ) );
	EXPECT_EQ( FontWeight::ExtraBold, Text::stringToFontWeight( "800" ) );
	EXPECT_EQ( FontWeight::Black, Text::stringToFontWeight( "900" ) );
}

UTEST( FontRendering, fontSizeStyleWeightOnUITextView ) {
	UIApplication app(
		WindowSettings( 400, 300, "eepp - FontWeight Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* tv = UITextView::New();
	tv->setParent( app.getUI() );
	tv->setVisible( true );
	tv->setEnabled( true );
	tv->setText( "Hello World" );
	tv->setPixelsSize( app.getUI()->getPixelsSize() );

	tv->setFontStyle( Text::Bold );
	EXPECT_NE( 0U, tv->getFontStyle() & Text::Bold );

	tv->setFontStyle( Text::Regular );
	EXPECT_EQ( 0U, tv->getFontStyle() & Text::Bold );

	UIFontStyleConfig config = tv->getFontStyleConfig();
	config.Weight = FontWeight::Medium;
	config.Style = Text::Regular;
	tv->setFontStyleConfig( config );
	EXPECT_EQ( FontWeight::Medium, tv->getFontStyleConfig().Weight );
	EXPECT_EQ( 0U, tv->getFontStyle() & Text::Bold );

	config.Weight = FontWeight::Bold;
	config.Style = ( config.Style & ~Text::Bold ) | Text::Bold;
	tv->setFontStyleConfig( config );
	EXPECT_EQ( FontWeight::Bold, tv->getFontStyleConfig().Weight );
	EXPECT_NE( 0U, tv->getFontStyle() & Text::Bold );
}

UTEST( FontRendering, loadFontFaceDataURI ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "eepp - Font Data URI", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();

	FontTrueType* baseFont = FontTrueType::New( "NotoSans-Regular" ).get();
	baseFont->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	themeManager->setDefaultFont( baseFont );

	std::string fontData;
	bool readOk = FileSystem::fileGet( "../assets/fonts/DejaVuSansMono.ttf", fontData );
	ASSERT_TRUE_MSG( readOk, "Failed to read font file" );
	ASSERT_TRUE_MSG( !fontData.empty(), "Font data is empty" );

	std::string base64Font;
	Base64::encode( std::string_view( fontData ), base64Font );
	ASSERT_TRUE_MSG( !base64Font.empty(), "Base64 encoding failed" );

	std::string css = "@font-face {\n"
					  "	font-family: 'DataURIFont';\n"
					  "	src: url('data:font/ttf;charset=utf-8;base64," +
					  base64Font +
					  "') format('truetype');\n"
					  "	font-weight: normal;\n"
					  "	font-style: normal;\n"
					  "}";

	sceneNode->combineStyleSheet( css, false, String::hash( css ), URI() );

	Font* loadedFont = sceneNode->getFontFromNamesList( "DataURIFont" );
	ASSERT_NE( loadedFont, nullptr );
	ASSERT_TRUE_MSG( loadedFont->loaded(), "Font loaded via data URI is not loaded" );
	EXPECT_EQ( nullptr, defaultResourceScope().findFont( "DataURIFont" ).get() );

	Engine::destroySingleton();
}

UTEST( FontRendering, fontFaceAuthorFamilyIsSceneScoped ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "eepp - Scoped Font Face", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	UISceneNode* sceneA = UISceneNode::New();
	UISceneNode* sceneB = UISceneNode::New();
	SceneManager::instance()->add( sceneA );
	SceneManager::instance()->add( sceneB );

	const std::string processPath( Sys::getProcessPath() );
	const std::string cssA =
		"@font-face { font-family: 'ScopedAuthorFace'; src: url('file://" + processPath +
		"../assets/fonts/NotoSans-Regular.ttf'); font-weight: normal; font-style: normal; }";
	const std::string cssB =
		"@font-face { font-family: 'ScopedAuthorFace'; src: url('file://" + processPath +
		"../assets/fonts/DejaVuSansMono.ttf'); font-weight: normal; font-style: normal; }";

	sceneA->combineStyleSheet( cssA, false, String::hash( cssA ), URI() );
	sceneB->combineStyleSheet( cssB, false, String::hash( cssB ), URI() );

	Font* fontA = sceneA->getFontFromNamesList( "ScopedAuthorFace" );
	Font* fontB = sceneB->getFontFromNamesList( "ScopedAuthorFace" );
	ASSERT_NE( fontA, nullptr );
	ASSERT_NE( fontB, nullptr );
	EXPECT_TRUE( fontA->loaded() );
	EXPECT_TRUE( fontB->loaded() );
	EXPECT_NE( fontA, fontB );
	EXPECT_TRUE( fontA->getName() != fontB->getName() );
	EXPECT_EQ( nullptr, defaultResourceScope().findFont( "ScopedAuthorFace" ).get() );

	Engine::destroySingleton();
}

UTEST( FontRendering, fontFaceReevaluateStyleUsesAuthorFamily ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "eepp - Font Face Style Reevaluate", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	UISceneNode* scene = UISceneNode::New();
	SceneManager::instance()->add( scene );

	const std::string processPath( Sys::getProcessPath() );
	const std::string css =
		"@font-face { font-family: 'AuthorWeightedFace'; src: url('file://" + processPath +
		"../assets/fonts/NotoSans-Regular.ttf'); font-weight: normal; font-style: normal; }"
		"@font-face { font-family: 'AuthorWeightedFace'; src: url('file://" +
		processPath +
		"../assets/fonts/NotoSans-Bold.ttf'); font-weight: bold; font-style: normal; }";

	scene->combineStyleSheet( css, false, String::hash( css ), URI() );

	Font* regularFont = scene->getFontFromNamesList( "AuthorWeightedFace" );
	Font* boldFont =
		scene->getFontFromNamesList( "AuthorWeightedFace", Text::Bold, FontWeight::Bold );
	ASSERT_NE( regularFont, nullptr );
	ASSERT_NE( boldFont, nullptr );
	EXPECT_NE( regularFont, boldFont );

	Font* reevaluatedFont = scene->reevaluateFontStyle( regularFont, Text::Bold, FontWeight::Bold );
	EXPECT_EQ( boldFont, reevaluatedFont );

	Engine::destroySingleton();
}

UTEST( FontRendering, editorTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - CodeEditor", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* editor = UICodeEditor::New();
		editor->setPixelsSize( app.getUI()->getPixelsSize() );
		editor->loadFromFile( "assets/textformat/english.utf8.lf.bom.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-editor-monospace" );
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

UTEST( FontRendering, textEditTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - TextEdit", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* editor = UITextEdit::New();
		editor->setPixelsSize( app.getUI()->getPixelsSize() );
		editor->loadFromFile( "assets/textformat/english.utf8.lf.bom.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-textedit" );
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

UTEST( FontRendering, tabsTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - Tabs Test", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* editor = UICodeEditor::New();
		editor->setPixelsSize( app.getUI()->getPixelsSize() );
		editor->loadFromFile( "assets/textfiles/test-tabs.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(),
					   "eepp-editor-monospace-tabs-test" );
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

UTEST( FontRendering, tabStopTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - Tab Stop Test", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* editor = UICodeEditor::New();
		editor->setTabStops( true );
		editor->setPixelsSize( app.getUI()->getPixelsSize() );
		editor->loadFromFile( "assets/textfiles/test-tabs.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(),
					   "eepp-editor-monospace-tab-stop-test" );
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

UTEST( FontRendering, tabsTextEditTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - TextEdit - Tabs Test", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* editor = UITextEdit::New();
		editor->setPixelsSize( app.getUI()->getPixelsSize() );
		editor->loadFromFile( "assets/textfiles/test-tabs.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-edit-tabs-test" );
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

UTEST( FontRendering, tabStopTextEditTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - TextEdit - Tab Stop Test", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* editor = UITextEdit::New();
		editor->setTabStops( true );
		editor->setPixelsSize( app.getUI()->getPixelsSize() );
		editor->loadFromFile( "assets/textfiles/test-tabs.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-edit-tab-stop-test" );
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

UTEST( FontRendering, textViewTest ) {
	const auto runTest = [&]() {
		UIApplication app( WindowSettings( 1024, 650, "eepp - TextView", VisualTestWindowStyle,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		auto* view = UITextView::New();
		view->setPixelsSize( app.getUI()->getPixelsSize() );
		std::string file;
		FileSystem::fileGet( "assets/textformat/english.utf8.lf.bom.txt", file );
		view->setText( file );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-textview" );
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

UTEST( FontRendering, textEditBengaliTest ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app(
		WindowSettings( 1024, 650, "eepp - TextEdit Bengali", VisualTestWindowStyle,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* bengaliFont =
		FontTrueType::New( "NotoSansBengali-Regular", "assets/fonts/NotoSansBengali-Regular.ttf" )
			.get();
	defaultResourceScope().getFontService().addFallbackFont( bengaliFont );
	UTEST_PRINT_STEP( "Text Shaper enabled" );
	auto* editor = UITextEdit::New();
	// editor->setFontSize( PixelDensity::dpToPx( 12 ) );
	editor->setPixelsSize( app.getUI()->getPixelsSize() );
	editor->loadFromFile( "assets/textfiles/test-bengali.uext" );
	SceneManager::instance()->update();
	SceneManager::instance()->draw();
	compareImages( utest_state, utest_result, app.getWindow(), "eepp-textedit-bengali" );
}

UTEST( FontRendering, textEditArabicTest ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app(
		WindowSettings( 1024, 650, "eepp - TextEdit Arabic", VisualTestWindowStyle,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* arabicFont =
		FontTrueType::New( "NotoNaskhArabic-Regular", "assets/fonts/NotoNaskhArabic-Regular.ttf" )
			.get();
	defaultResourceScope().getFontService().addFallbackFont( arabicFont );
	UTEST_PRINT_STEP( "Text Shaper enabled" );
	auto* editor = UITextEdit::New();
	// editor->setFontSize( PixelDensity::dpToPx( 12 ) );
	editor->setPixelsSize( app.getUI()->getPixelsSize() );
	editor->loadFromFile( "assets/textfiles/test-arabic.uext" );
	SceneManager::instance()->update();
	SceneManager::instance()->draw();
	compareImages( utest_state, utest_result, app.getWindow(), "eepp-textedit-arabic" );
}

UTEST( FontRendering, textEditHebrewTest ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app(
		WindowSettings( 1024, 650, "eepp - TextEdit Hebrew", VisualTestWindowStyle,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* hebrewFont =
		FontTrueType::New( "NotoSansHebrew-Regular", "assets/fonts/NotoSansHebrew-Regular.ttf" )
			.get();
	defaultResourceScope().getFontService().addFallbackFont( hebrewFont );
	UTEST_PRINT_STEP( "Text Shaper enabled" );
	auto* editor = UITextEdit::New();
	// editor->setFontSize( PixelDensity::dpToPx( 12 ) );
	editor->setPixelsSize( app.getUI()->getPixelsSize() );
	editor->loadFromFile( "assets/textfiles/test-hebrew.uext" );
	SceneManager::instance()->update();
	SceneManager::instance()->draw();
	compareImages( utest_state, utest_result, app.getWindow(), "eepp-textedit-hebrew" );
}

UTEST( FontRendering, textSizes ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "eepp - Text Sizes", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	Text::TextShaperEnabled = false;

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	FontStyleConfig config;
	config.Font = font;
	config.CharacterSize = 12;
	config.Style = 0;

	String txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod\n"
				"tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,\n"
				"quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo\n"
				"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse\n"
				"cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non\n"
				"proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

	const auto runTest = [&]() {
		Sizef size = Text::draw( txt, Vector2f::Zero, config );
		EXPECT_EQ( 445, size.getWidth() );
		EXPECT_EQ( 96, size.getHeight() );
		EXPECT_EQ( 445, Text::getTextWidth( txt, config ) );

		Vector2i topPos{ 120, 0 };
		EXPECT_EQ( 19, Text::findCharacterFromPos( topPos, true, config.Font, config.CharacterSize,
												   txt, 0 ) );
		EXPECT_EQ( 19, Text::findCharacterFromPos( topPos, false, config.Font, config.CharacterSize,
												   txt, 0 ) );

		Vector2i startPos{ 120, 7 };
		EXPECT_EQ( 19, Text::findCharacterFromPos( startPos, true, config.Font,
												   config.CharacterSize, txt, 0 ) );
		EXPECT_EQ( 19, Text::findCharacterFromPos( startPos, false, config.Font,
												   config.CharacterSize, txt, 0 ) );

		Vector2i middlePos{ 120, 64 };
		EXPECT_EQ( 242, Text::findCharacterFromPos( middlePos, true, config.Font,
													config.CharacterSize, txt, 0 ) );
		EXPECT_EQ( 242, Text::findCharacterFromPos( middlePos, false, config.Font,
													config.CharacterSize, txt, 0 ) );

		Vector2i endPos{ 120, 103 };
		EXPECT_EQ( 395, Text::findCharacterFromPos( endPos, true, config.Font, config.CharacterSize,
													txt, 0 ) );
		EXPECT_EQ( -1, Text::findCharacterFromPos( endPos, false, config.Font, config.CharacterSize,
												   txt, 0 ) );

		EXPECT_EQ( 18ul, Text::findLastCharPosWithinLength( txt, 120, config ) );
		EXPECT_EQ( 446ul, Text::findLastCharPosWithinLength( txt, 1000, config ) );

		Vector2f pos = Text::findCharacterPos( 19, config.Font, config.CharacterSize, txt, 0 );
		EXPECT_EQ( 120, pos.x );
		EXPECT_EQ( 0, pos.y );

		Text text;
		text.setStyleConfig( config );
		text.setString( txt );
		EXPECT_EQ( 445, text.getTextWidth() );
		EXPECT_EQ( 96, text.getTextHeight() );
		EXPECT_EQ( 446, text.getLocalBounds().getWidth() );
		EXPECT_EQ( 93, text.getLocalBounds().getHeight() );
		EXPECT_EQ( 19, text.findCharacterFromPos( startPos, true ) );
		EXPECT_EQ( 19, text.findCharacterFromPos( startPos, false ) );
		EXPECT_EQ( 242, text.findCharacterFromPos( middlePos, true ) );
		EXPECT_EQ( 242, text.findCharacterFromPos( middlePos, false ) );
		EXPECT_EQ( 395, text.findCharacterFromPos( endPos ) );
		EXPECT_EQ( -1, text.findCharacterFromPos( endPos, false ) );
		pos = text.findCharacterPos( 19 );
		EXPECT_EQ( 120, pos.x );
		EXPECT_EQ( 0, pos.y );
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
	}

	UTEST_PRINT_STEP( "Text Shaper enabled w/o optimizations" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
		BoolScopedOp op2( Text::TextShaperOptimizations, false );
		runTest();
	}

	Engine::destroySingleton();
}

UTEST( FontRendering, textStyles ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 230, "eepp - Text Styles", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	Text::TextShaperEnabled = false;

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	win->setClearColor( RGB( 255, 255, 255 ) );

	FontStyleConfig config;
	config.Font = font;
	config.FontColor = Color::Black;
	config.CharacterSize = 20;
	config.OutlineColor = Color::Black;
	config.ShadowColor = Color::lightgray;

	String txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod\n"
				"tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,\n"
				"quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo\n"
				"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse\n"
				"cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non\n"
				"proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

	const auto runTest = [&]( std::string_view styleName, Uint32 textAlign ) {
		win->clear();
		Text text;
		text.setStyleConfig( config );
		text.setString( txt );
		text.setAlign( textAlign );
		text.draw( 32, 32 );
		compareImages( utest_state, utest_result, win, "eepp-text-style-" + styleName );
	};

	const auto runTestSuite = [&]( Uint32 style, std::string_view styleName,
								   Uint32 textAlign = TEXT_ALIGN_LEFT ) {
		config.Style = style;

		UTEST_PRINT_STEP( styleName.data() );

		{
			UTEST_PRINT_STEP( "	Text Shaper disabled" );
			BoolScopedOp op( Text::TextShaperEnabled, false );
			runTest( styleName, textAlign );
		}

		{
			UTEST_PRINT_STEP( "	Text Shaper enabled" );
			BoolScopedOp op( Text::TextShaperEnabled, true );
			runTest( styleName, textAlign );

			UTEST_PRINT_STEP( "	Text Shaper enabled w/o optimizations" );
			BoolScopedOp op2( Text::TextShaperOptimizations, false );
			runTest( styleName, textAlign );
		}
	};

	runTestSuite( Text::Regular, "regular" );
	runTestSuite( Text::Bold, "bold" );
	runTestSuite( Text::Italic, "italic" );
	runTestSuite( Text::Underlined, "underline" );
	runTestSuite( Text::StrikeThrough, "strikethrough" );
	runTestSuite( Text::Shadow, "shadow" );
	config.FontColor = Color::White;
	config.OutlineThickness = 1;
	runTestSuite( Text::Regular, "outline" );
	config.FontColor = Color::Black;
	config.OutlineThickness = 0;
	runTestSuite( Text::Regular, "regular-center", TEXT_ALIGN_CENTER );
	runTestSuite( Text::Regular, "regular-right", TEXT_ALIGN_RIGHT );
	runTestSuite( Text::Underlined, "underline-center", TEXT_ALIGN_CENTER );
	runTestSuite( Text::Underlined, "underline-right", TEXT_ALIGN_RIGHT );
	runTestSuite( Text::StrikeThrough, "strikethrough-center", TEXT_ALIGN_CENTER );
	runTestSuite( Text::StrikeThrough, "strikethrough-right", TEXT_ALIGN_RIGHT );

	Engine::destroySingleton();
}

UTEST( FontRendering, emojisWithText ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 230, "eepp - Emojis With Text", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	Text::TextShaperEnabled = false;

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	FontTrueType* fontEmojiColor = FontTrueType::New( "NotoColorEmoji" ).get();
	fontEmojiColor->loadFromFile( "../assets/fonts/NotoColorEmoji.ttf" );

	win->setClearColor( RGB( 255, 255, 255 ) );

	FontStyleConfig config;
	config.Font = font;
	config.FontColor = Color::Black;
	config.CharacterSize = 16;

	String txt(
		R"txt(👻 Lorem ipsum dolor sit amet, 👻 consectetur adipisicing elit, sed do eiusmod🤯 tempor incididunt ut labore et dolore magna
aliqua. Ut enim ad😎 minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat 🤖.
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur 🧐. Excepteur sint occaecat
cupidatat non proident👽, sunt in culpa qui officia deserunt mollit anim id est laborum. 😀)txt" );

	const auto runTest = [&]() {
		win->clear();
		Text text;
		text.setStyleConfig( config );
		text.setString( txt );
		text.draw( 32, 32 );
		compareImages( utest_state, utest_result, win, "eepp-emojis-with-text" );
	};

	UTEST_PRINT_STEP( "	Text Shaper disabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, false );
		runTest();
	}

	UTEST_PRINT_STEP( "	Text Shaper enabled" );
	BoolScopedOp op( Text::TextShaperEnabled, true );
	runTest();

	UTEST_PRINT_STEP( "	Text Shaper enabled w/o optimizations" );
	BoolScopedOp op2( Text::TextShaperOptimizations, false );
	runTest();

	Engine::destroySingleton();
}

UTEST( FontRendering, textSetFillColor ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 230, "eepp - Text Set Fill Color", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	UTEST_PRINT_INFO( GLi->getRenderer().c_str() );

	win->setClearColor( RGB( 230, 230, 230 ) );

	FontTrueType* arabicFont =
		FontTrueType::New( "NotoNaskhArabic-Regular", "assets/fonts/NotoNaskhArabic-Regular.ttf" )
			.get();

	Text text;
	text.setFont( arabicFont );
	text.setFontSize( 64 );
	text.setAlign( TEXT_ALIGN_CENTER );
	std::string arabicTxtUtf8;
	FileSystem::fileGet( "assets/textfiles/test-arabic-simple.uext", arabicTxtUtf8 );
	String arabicTxt( arabicTxtUtf8 );
	text.setString( arabicTxt );
	text.setFillColor( Color::Black );

	const auto runTest = [&]( std::string_view testName ) {
		win->clear();
		text.draw( 0, win->getHeight() * 0.5f - text.getTextHeight() * 0.5f );
		compareImages( utest_state, utest_result, win,
					   std::string( "eepp-text-set-fill-color-" ) + std::string( testName ) );
	};

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );

		// Test Vector Fill
		{
			std::vector<Color> colors;
			for ( size_t i = 0; i < arabicTxt.size(); i++ ) {
				// Alternating colors
				if ( i % 3 == 0 )
					colors.push_back( Color::Red );
				else if ( i % 3 == 1 )
					colors.push_back( Color::Green );
				else
					colors.push_back( Color::Blue );
			}
			text.setFillColor( colors );
			runTest( "vector" );
		}

		// Test Range Fill
		{
			text.setFillColor( Color::Black );
			// Color "World" (بالعالم) in Red. It's at the end of the string.
			// "مرحباً" (Hello) is 6 chars + space = 7.
			// "بالعالم" (World) starts at index 7.
			if ( arabicTxt.size() > 7 ) {
				text.setFillColor( Color::Red, 7, arabicTxt.size() );
			}
			runTest( "range" );
		}
	}

	Engine::destroySingleton();
}

UTEST( FontRendering, UITextTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - UI Text Test", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		app.getUI()->loadLayoutFromFile( "assets/layouts/ui_text_test.xml" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-ui-text-test" );
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

UTEST( FontRendering, TextWrap ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	std::string loremIpsum;
	FileSystem::fileGet( "assets/textfiles/lorem-ipsum.uext", loremIpsum );

	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 512, 555, "eepp - Text Wrap", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		app.getWindow()->setClearColor( RGB( 255, 255, 255 ) );
		app.getWindow()->clear();
		Vector2f pos{ 5, 5 };
		Primitives p;
		p.setColor( Color::Red );
		p.drawRectangle( Rectf( pos - 1.f, { 1, 546 } ) );
		p.drawRectangle( Rectf( pos - 1.f, { 501, 1 } ) );
		p.drawRectangle( Rectf( { pos.x - 1.f, 544 + pos.y }, { 502, 1 } ) );
		p.drawRectangle( Rectf( { 500 + pos.x, pos.y - 1.f }, { 1, 546 } ) );

		Text text;
		text.setFont( app.getUI()->getUIThemeManager()->getDefaultFont() );
		text.setFontSize( 16 );
		text.setColor( Color::Black );
		text.setString( loremIpsum );
		text.setLineWrapMode( LineWrapMode::Word );
		text.setMaxWrapWidth( 500 );
		text.draw( pos.x, pos.y );

		text.setAlign( TEXT_ALIGN_CENTER );
		pos.y += text.getTextHeight() + 8;
		text.draw( pos.x, pos.y );

		pos.y += text.getTextHeight() + 8;
		text.setAlign( TEXT_ALIGN_RIGHT );
		text.draw( pos.x, pos.y );

		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-wrap" );
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

UTEST( FontRendering, TextLayoutWrap ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	std::string loremIpsum;
	FileSystem::fileGet( "assets/textfiles/lorem-ipsum.uext", loremIpsum );

	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 512, 555, "eepp - Text Layout Wrap", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		BatchRenderer* BR = GlobalBatchRenderer::instance();
		auto drawGlyph = [BR]( GlyphDrawable* gd, const Vector2f& position, const Color& color ) {
			BR->quadsSetColor( color );
			BR->quadsSetTexCoord( gd->getSrcRect().Left, gd->getSrcRect().Top,
								  gd->getSrcRect().Left + gd->getSrcRect().Right,
								  gd->getSrcRect().Top + gd->getSrcRect().Bottom );
			BR->batchQuad( position.x + gd->getGlyphOffset().x, position.y + gd->getGlyphOffset().y,
						   gd->getDestSize().getWidth(), gd->getDestSize().getHeight() );
		};

		app.getWindow()->setClearColor( RGB( 255, 255, 255 ) );
		app.getWindow()->clear();

		Vector2f pos{ 5, 5 };
		Primitives p;
		p.setColor( Color::Red );
		p.drawPixelPerfectLineRectangle( { { 4, 4 }, { 502, 546 } } );

		FontTrueType* font =
			static_cast<FontTrueType*>( app.getUI()->getUIThemeManager()->getDefaultFont() );
		auto fontSize = 16;
		const TexturePtr& fontTexture = font->getTexture( fontSize );
		BR->setBlendMode( BlendMode::Alpha() );
		BR->quadsBegin();
		BR->setTexture( fontTexture, fontTexture->getCoordinateType() );

		String string( loremIpsum );

		// Remove the emoji since it won't work in this context
		if ( Font::isEmojiCodePoint( string[string.size() - 1] ) )
			string.pop_back();

		auto layout = TextLayout::layout( string, font, fontSize, 0, 4, 0, {}, 0,
										  TextDirection::LeftToRight, LineWrapMode::Word, 500 );

		for ( const auto& sp : layout->paragraphs ) {
			for ( const auto& sg : sp.shapedGlyphs ) {
				auto* gd = sg.font->getGlyphDrawableFromGlyphIndex( sg.glyphIndex, fontSize );
				if ( gd )
					drawGlyph( gd, pos + sg.position, Color::Black );
			}
		}

		BR->draw();

		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-layout-wrap" );
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

UTEST( FontRendering, LineWrapInfo ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UIApplication app(
		WindowSettings( 1024, 650, "eepp - LineWrapInfo Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	Font* font = app.getUI()->getUIThemeManager()->getDefaultFont();
	Float width = app.getWindow()->getSize().getWidth();
	int fontSize = 16;
	LineWrapMode mode = LineWrapMode::Word;

	const auto runTest = [&]( const std::string& path ) {
		UTEST_PRINT_STEP( String::format( "Test File: %s", path.c_str() ).c_str() );
		UTEST_PRINT_STEP( "Line Breaks" );

		std::string string;
		FileSystem::fileGet( path, string );
		LineWrapInfoEx lineWrapShaperDisabled;
		LineWrapInfoEx lineWrapShaperEnabled;
		LineWrapInfoEx lineWrapShaperEnabledWOO;

		{
			UTEST_PRINT_STEP( "Text Shaper disabled" );
			BoolScopedOp op( Text::TextShaperEnabled, false );
			String str( string );
			lineWrapShaperDisabled =
				LineWrap::computeLineBreaksEx( string, font, fontSize, width, mode );
		}

		UTEST_PRINT_STEP( "Text Shaper enabled" );
		{
			BoolScopedOp op( Text::TextShaperEnabled, true );
			String str( string );
			lineWrapShaperEnabled =
				LineWrap::computeLineBreaksEx( string, font, fontSize, width, mode );

			EXPECT_VECTOREQ( lineWrapShaperDisabled.wraps, lineWrapShaperEnabled.wraps );

			{
				UTEST_PRINT_STEP( "Text Shaper enabled w/o optimizations" );
				BoolScopedOp op2( Text::TextShaperOptimizations, false );
				String str( string );
				lineWrapShaperEnabledWOO =
					LineWrap::computeLineBreaksEx( string, font, fontSize, width, mode );
				EXPECT_VECTOREQ( lineWrapShaperDisabled.wraps, lineWrapShaperEnabledWOO.wraps );
			}
		}

		UTEST_PRINT_STEP( "Test Widths" );

		Text text;
		text.setFont( font );
		text.setFontSize( fontSize );
		text.setString( string );
		text.hardWrapText( width );

		const auto linesWidth = text.getLinesWidth();

		UTEST_PRINT_STEP( "Text Shaper disabled" );
		EXPECT_VECTOREQ( linesWidth, lineWrapShaperDisabled.wrapsWidth );
		UTEST_PRINT_STEP( "Text Shaper enabled" );
		EXPECT_VECTOREQ( linesWidth, lineWrapShaperEnabled.wrapsWidth );
		UTEST_PRINT_STEP( "Text Shaper enabled w/o optimizations" );
		EXPECT_VECTOREQ( linesWidth, lineWrapShaperEnabledWOO.wrapsWidth );
	};

	runTest( "assets/textfiles/test-hard-wrap.uext" );
	runTest( "assets/textfiles/lorem-ipsum.uext" );
	runTest( "assets/textfiles/test-tabs.txt" );
	runTest( "assets/textfiles/quota.uext" );
}

UTEST( FontRendering, TextHardWrap ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	std::string string;
	FileSystem::fileGet( "assets/textfiles/test-hard-wrap.uext", string );

	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - Text Hard Wrap", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		auto colors = SyntaxColorScheme::getDefaultDark();
		auto syntax = SyntaxDefinitionManager::instance()->getByLanguageName( "Markdown" );

		app.getWindow()->setClearColor( colors.getEditorColor( "background"_sst ).toRGB() );
		app.getWindow()->clear();

		Text text;
		text.setFont( app.getUI()->getUIThemeManager()->getDefaultFont() );
		text.setFontSize( 16 );
		text.setColor( Color::Black );
		text.setString( string );
		text.hardWrapText( app.getWindow()->getSize().getWidth() );
		SyntaxTokenizer::tokenizeText( syntax, colors, &text );
		text.draw( 0, 0 );

		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-hard-wrap" );
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

UTEST( FontRendering, UITextViewWrappedSelection ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - TextView Wrapped Selection", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 1.5f ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		std::string buffer;
		FileSystem::fileGet( "assets/textfiles/lorem-ipsum.uext", buffer );
		auto textView = UITextView::New();
		textView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
		textView->setPixelsSize( app.getUI()->getPixelsSize() );
		textView->setText( buffer );
		textView->setWordWrap( true );
		textView->setTextSelectionEnabled( true );
		textView->setTextSelectionRange( { 51, 286 } );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(),
					   "eepp-textview-wrapped-selection" );
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

UTEST( FontRendering, UITextViewWrappedSelection2 ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - TextView Wrapped Selection", VisualTestWindowStyle,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 1.5f ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
		std::string buffer;
		FileSystem::fileGet( "assets/textfiles/quota.uext", buffer );
		auto textView = UITextView::New();
		textView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
		textView->setPixelsSize( app.getUI()->getPixelsSize() );
		textView->setText( buffer );
		textView->setWordWrap( true );
		textView->setTextSelectionEnabled( true );
		textView->setTextSelectionRange( { 51, 286 } );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(),
					   "eepp-textview-wrapped-selection-2" );
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

UTEST( FontRendering, TextSoftWrapPos ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 768, "eepp - Text Soft Wrap Pos", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		FontTrueType* font = FontTrueType::New( "DejaVuSansMono" ).get();
		font->loadFromFile( "../assets/fonts/DejaVuSansMono.ttf" );

		Text text;
		text.setFont( font );
		text.setFontSize( 20 );
		text.setString( "This is a long string that should wrap when the width is restricted." );
		text.setColor( Color::White );
		text.setLineWrapMode( LineWrapMode::Word );
		text.setMaxWrapWidth( 200.f );

		Vector2f pos = text.findCharacterPos( 30 );
		EXPECT_GT( pos.y, 0.f );

		Float vspace = text.getFont()->getLineSpacing( text.getCharacterSize() );
		Vector2i queryPos( 10, (int)vspace + 5 );
		Int32 foundIndex = text.findCharacterFromPos( queryPos );

		EXPECT_GT( foundIndex, (Int32)14 );

		Vector2f foundPos = text.findCharacterPos( foundIndex );
		EXPECT_GT( foundPos.y, 0.f );
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

UTEST( FontRendering, TextSelection ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "eepp - Text Selection", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );
	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	Text::TextShaperEnabled = false;

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	bool loaded = font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( loaded );
	FontFamily::loadFromRegular( font );

	FontStyleConfig config;
	config.Font = font;
	config.CharacterSize = 20;
	config.FontColor = Color::Black;
	config.Style = Text::Regular;

	String txt( "Line 1\nLine 2 is longer\nLine 3" );

	Text text;
	text.setStyleConfig( config );
	text.setString( txt );

	// Test 1: Single line selection (Line 1)
	{
		auto rects = text.getSelectionRects( { 0, 4 } ); // "Line"
		EXPECT_EQ( 1ul, rects.size() );
		if ( !rects.empty() ) {
			EXPECT_EQ( 0, rects[0].Top );
			EXPECT_GT( rects[0].getWidth(), 0 );
			EXPECT_EQ( text.findCharacterPos( 0 ).x, rects[0].Left );
			EXPECT_EQ( text.findCharacterPos( 4 ).x, rects[0].Right );
		}
	}

	// Test 2: Multi-line selection (Line 1 to Line 2)
	{
		// "Line 1\nLine 2" -> Indices: "Line 1" (0-5), "\n" (6), "Line 2" (7-12)
		// Select from index 2 ("n" in "Line 1") to index 9 ("i" in "Line 2")
		auto rects = text.getSelectionRects( { 2, 9 } );
		EXPECT_EQ( 2ul, rects.size() );
		if ( rects.size() >= 2 ) {
			// First line rect: From index 2 to end of line 1
			EXPECT_EQ( text.findCharacterPos( 2 ).x, rects[0].Left );
			EXPECT_GT( rects[0].Right, rects[0].Left );

			// Second line rect: From start of line 2 to index 9
			EXPECT_EQ( 0, rects[1].Left ); // Left aligned
			EXPECT_EQ( text.findCharacterPos( 9 ).x, rects[1].Right );
		}
	}

	// Test 3: Full selection
	{
		auto rects = text.getSelectionRects( { 0, static_cast<Int64>( txt.size() ) } );
		EXPECT_EQ( 3ul, rects.size() );
	}

	// Test 4: Soft wrap
	{
		text.setLineWrapMode( LineWrapMode::Word );
		text.setMaxWrapWidth( 50 ); // Force wrap

		text.setString( "This is a very long string that should wrap multiple times." );
		// Ensure layout is updated
		text.getVisualLineCount();

		EXPECT_GT( text.getVisualLineCount(), (Uint32)1 );

		auto rects = text.getSelectionRects( { 0, static_cast<Int64>( text.getString().size() ) } );
		EXPECT_EQ( (size_t)text.getVisualLineCount(), rects.size() );
	}

	Engine::destroySingleton();
}

UTEST( FontRendering, TextInitialOffset ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	std::string loremIpsum;
	FileSystem::fileGet( "assets/textfiles/lorem-ipsum.uext", loremIpsum );
	String string( loremIpsum );
	string.pop_back();

	const auto runTest = [&]() {
		auto win = Engine::instance()->createWindow(
			WindowSettings( 512, 400, "eepp - Text Initial Offset", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ) );

		ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

		win->setClearColor( RGB( 255, 255, 255 ) );
		win->clear();

		FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
		font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

		Primitives p;
		Float lw = p.getLineWidth();
		p.setColor( Color::Red );
		p.setFillMode( PrimitiveFillMode::DRAW_LINE );

		// Test 1: Text with paragraph indent (initial X offset)
		Float paragraphIndent = 40.f;
		Float maxWidth = 400.f;
		Vector2f pos{ 20, 20 };

		Text text;
		text.setFont( font );
		text.setFontSize( 14 );
		text.setColor( Color::Black );
		text.setString( string );
		text.setLineWrapMode( LineWrapMode::Word );
		text.setMaxWrapWidth( maxWidth );
		text.setInitialOffset( { paragraphIndent, 0 } );
		p.drawPixelPerfectLineRectangle(
			text.getLocalBounds().move( pos ).move( { -lw, -lw } ).enlarge( { lw * 2, lw * 2 } ) );
		text.draw( pos.x, pos.y );

		// Test 2: Text with Y offset (simulating a text block shifted down)
		pos.y += text.getTextHeight() + 30;
		text.setInitialOffset( { paragraphIndent * 2, 0 } ); // Larger indent
		p.drawPixelPerfectLineRectangle(
			text.getLocalBounds().move( pos ).move( { -lw, -lw } ).enlarge( { lw * 2, lw * 2 } ) );
		text.draw( pos.x, pos.y );

		compareImages( utest_state, utest_result, win, "eepp-text-initial-offset" );

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

UTEST( FontRendering, TextContiguousOffset ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	const auto runTest = [&]() {
		auto win = Engine::instance()->createWindow(
			WindowSettings( 512, 300, "eepp - Text Contiguous Offset", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ) );

		ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

		win->setClearColor( RGB( 255, 255, 255 ) );
		win->clear();

		FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
		font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

		Float maxWidth = 450.f;
		Vector2f pos{ 20, 20 };
		Float paragraphIndent = 30.f;

		Primitives p;
		p.setColor( Color::Red );
		Float lw = p.getLineWidth();

		// Simulate RichText: Two contiguous Text instances
		// First Text: "Hello " in black (with paragraph indent)
		Text text1;
		text1.setFont( font );
		text1.setFontSize( 16 );
		text1.setColor( Color::seagreen );
		text1.setString( "Hello " );
		text1.setInitialOffset( { paragraphIndent, 0 } );
		text1.draw( pos.x, pos.y );

		Float text1EndX = text1.getLastLineWidth();

		// Second Text: "World! This is a long text that should wrap to a second visual line..."
		// in red, continuing from text1's end position
		Text text2;
		text2.setFont( font );
		text2.setFontSize( 16 );
		text2.setColor( Color::darkmagenta );
		text2.setString( "World! This is a RichText simulation where the second Text segment "
						 "continues from where the first left off and should wrap correctly "
						 "to multiple visual lines. The wrap should respect the initial offset." );
		text2.setLineWrapMode( LineWrapMode::Word );
		text2.setMaxWrapWidth( maxWidth );
		text2.setInitialOffset( { text1EndX, 0 } ); // Continue from text1's end
		text2.draw( pos.x, pos.y );

		p.drawPixelPerfectLineRectangle( text1.getLocalBounds()
											 .move( pos )
											 .expand( text2.getLocalBounds().move( pos ) )
											 .move( { -lw, -lw } )
											 .enlarge( { lw * 2, lw * 2 } ) );

		// Verify the second text wrapped
		EXPECT_GT( text2.getVisualLineCount(), (Uint32)1 );

		// Second example: Without paragraph indent
		pos.y += text2.getTextHeight() + 30;

		Text text3;
		text3.setFont( font );
		text3.setFontSize( 16 );
		text3.setColor( Color::Blue );
		text3.setString( "Start: " );
		text3.draw( pos.x, pos.y );

		Float text3EndX = text3.getLastLineWidth();

		Text text4;
		text4.setFont( font );
		text4.setFontSize( 16 );
		text4.setColor( Color::darkorchid );
		text4.setString( "This text continues from \"Start: \" and also wraps to show "
						 "that the initial X offset is correctly applied only to the first "
						 "visual line, while subsequent lines start at x=0." );
		text4.setLineWrapMode( LineWrapMode::Word );
		text4.setMaxWrapWidth( maxWidth );
		text4.setInitialOffset( { text3EndX, 0 } );
		text4.draw( pos.x, pos.y );

		p.drawPixelPerfectLineRectangle( text3.getLocalBounds()
											 .move( pos )
											 .expand( text4.getLocalBounds().move( pos ) )
											 .move( { -lw, -lw } )
											 .enlarge( { lw * 2, lw * 2 } ) );

		EXPECT_GT( text4.getVisualLineCount(), (Uint32)1 );

		compareImages( utest_state, utest_result, win, "eepp-text-contiguous-offset" );

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

UTEST( FontRendering, TextBackgroundColor ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	const auto runTest = [&]() {
		auto win = Engine::instance()->createWindow(
			WindowSettings( 512, 400, "eepp - Text Background Color", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ) );

		ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

		win->setClearColor( RGB( 255, 255, 255 ) );
		win->clear();

		FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
		font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

		Vector2f pos{ 20, 20 };
		Text text;
		text.setFont( font );
		text.setFontSize( 20 );
		text.setFillColor( Color::Black );
		text.setBackgroundColor( Color::Yellow );
		text.setString( "Text with background color\nand multiple lines." );
		text.draw( pos.x, pos.y );

		pos.y += text.getTextHeight() + 20;
		text.setAlign( TEXT_ALIGN_CENTER );
		text.setString( "Centered text with\nbackground color." );
		text.draw( pos.x, pos.y );

		pos.y += text.getTextHeight() + 20;
		text.setAlign( TEXT_ALIGN_LEFT );
		text.setLineWrapMode( LineWrapMode::Word );
		text.setMaxWrapWidth( 200 );
		text.setString(
			"Wrapped text with background color that should only cover the text area." );
		text.draw( pos.x, pos.y );

		pos.y += text.getTextHeight() + 20;
		text.setLineWrapMode( LineWrapMode::NoWrap );
		text.setBackgroundColor( Color::cyan );
		text.setString( "    " ); // Only spaces
		text.draw( pos.x, pos.y );

		compareImages( utest_state, utest_result, win, "eepp-text-background-color" );

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
