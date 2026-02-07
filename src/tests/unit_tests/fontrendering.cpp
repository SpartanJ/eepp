#include "utest.hpp"

#include <eepp/graphics/batchrenderer.hpp>
#include <eepp/graphics/fontbmfont.hpp>
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fontsprite.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderergl.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>

#include <iostream>

using namespace EE;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::UI;

static void compareImages( utest_state_s& utest_state, int* utest_result, EE::Window::Window* win,
						   const std::string& imageName ) {
	auto saveType = Image::SaveType::WEBP;
	auto saveExt( Image::saveTypeToExtension( saveType ) );
	std::string expectedImagePath( "assets/fontrendering/" + imageName + "." + saveExt );

	Image::FormatConfiguration fconf;
	fconf.webpSaveLossless( true );

	Image actualImage = win->getFrontBufferImage();
	actualImage.setImageFormatConfiguration( fconf );

	if ( !FileSystem::fileExists( expectedImagePath ) )
		actualImage.saveToFile( expectedImagePath, saveType );

	Image expectedImage( expectedImagePath );
	ASSERT_TRUE( expectedImage.getPixelsPtr() != nullptr );
	EXPECT_EQ_MSG( expectedImage.getWidth(), actualImage.getWidth(), "Images width not equal" );
	EXPECT_EQ_MSG( expectedImage.getHeight(), actualImage.getHeight(), "Images height not equal" );

	Image::DiffResult result = actualImage.diff( expectedImage );
	EXPECT_TRUE( result.areSame() );
	if ( !result.areSame() ) {
		auto saveExt( Image::saveTypeToExtension( saveType ) );
		std::string withTextShaper =
			Text::TextShaperEnabled
				? ( Text::TextShaperOptimizations ? "_text_shape_no_opt" : "_text_shape" )
				: "";
		std::cerr << "Test FAILED: " << result.numDifferentPixels << " pixels differ." << std::endl;
		std::cerr << "Maximum perceptual difference (Delta E): " << result.maxDeltaE << std::endl;
		if ( !FileSystem::fileExists( "output" ) )
			FileSystem::makeDir( "output" );
		std::string actualImagePath =
			"output/" + imageName + "_actual_output" + withTextShaper + "." + saveExt;
		actualImage.saveToFile( actualImagePath, saveType );
		std::cerr << "Actual image saved to: " << actualImagePath << std::endl;
		if ( result.diffImage ) {
			std::string diffImagePath =
				"output/" + imageName + "_diff_output" + withTextShaper + "." + saveExt;
			result.diffImage->setImageFormatConfiguration( fconf );
			result.diffImage->saveToFile( diffImagePath, saveType );
			std::cerr << "Visual diff saved to: " << diffImagePath << std::endl;
		}
	}
}

UTEST( FontRendering, fontsTest ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "eepp - Fonts", WindowStyle::Default, WindowBackend::Default, 32,
						{}, 1, false, true ) );

	ASSERT_TRUE_MSG( win->isOpen(), "Failed to create Window" );

	UTEST_PRINT_INFO( GLi->getRenderer().c_str() );

	win->setClearColor( RGB( 230, 230, 230 ) );

	String Txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod "
				"tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
				"quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo "
				"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse "
				"cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non "
				"proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

	FontTrueType* fontTest = FontTrueType::New( "DejaVuSansMono" );
	fontTest->loadFromFile( "../assets/fonts/DejaVuSansMono.ttf" );

	FontTrueType* fontTest2 = FontTrueType::New( "NotoSans-Regular" );
	fontTest2->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );

	FontTrueType* fontEmoji = FontTrueType::New( "NotoEmoji-Regular" );
	fontEmoji->loadFromFile( "../assets/fonts/NotoEmoji-Regular.ttf" );

	FontTrueType* fontEmojiColor = FontTrueType::New( "NotoColorEmoji" );
	fontEmojiColor->loadFromFile( "../assets/fonts/NotoColorEmoji.ttf" );

	FontBMFont* fontBMFont = FontBMFont::New( "bmfont" );
	fontBMFont->loadFromFile( "../assets/fonts/bmfont.fnt" );

	FontSprite* fontSprite = FontSprite::New( "alagard" );
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

	Engine::destroySingleton();
}

UTEST( FontRendering, editorTest ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - CodeEditor", WindowStyle::Default,
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
			WindowSettings( 1024, 650, "eepp - TextEdit", WindowStyle::Default,
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
			WindowSettings( 1024, 650, "eepp - Tabs Test", WindowStyle::Default,
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
			WindowSettings( 1024, 650, "eepp - Tab Stop Test", WindowStyle::Default,
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
			WindowSettings( 1024, 650, "eepp - TextEdit - Tabs Test", WindowStyle::Default,
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
			WindowSettings( 1024, 650, "eepp - TextEdit - Tab Stop Test", WindowStyle::Default,
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
		UIApplication app( WindowSettings( 1024, 650, "eepp - TextView", WindowStyle::Default,
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
		WindowSettings( 1024, 650, "eepp - TextEdit Bengali", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* bengaliFont =
		FontTrueType::New( "NotoSansBengali-Regular", "assets/fonts/NotoSansBengali-Regular.ttf" );
	FontManager::instance()->addFallbackFont( bengaliFont );
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
		WindowSettings( 1024, 650, "eepp - TextEdit Arabic", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* arabicFont =
		FontTrueType::New( "NotoNaskhArabic-Regular", "assets/fonts/NotoNaskhArabic-Regular.ttf" );
	FontManager::instance()->addFallbackFont( arabicFont );
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
		WindowSettings( 1024, 650, "eepp - TextEdit Hebrew", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* hebrewFont =
		FontTrueType::New( "NotoSansHebrew-Regular", "assets/fonts/NotoSansHebrew-Regular.ttf" );
	FontManager::instance()->addFallbackFont( hebrewFont );
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

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
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

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
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

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	FontTrueType* fontEmojiColor = FontTrueType::New( "NotoColorEmoji" );
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
		FontTrueType::New( "NotoNaskhArabic-Regular", "assets/fonts/NotoNaskhArabic-Regular.ttf" );

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
			WindowSettings( 1024, 650, "eepp - UI Text Test", WindowStyle::Default,
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
		Texture* fontTexture = font->getTexture( fontSize );
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
}

UTEST( FontRendering, TextHardWrap ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	std::string string;
	FileSystem::fileGet( "assets/textfiles/test-hard-wrap.uext", string );

	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 650, "eepp - Text Hard Wrap", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		auto colors = SyntaxColorScheme::getDefault();
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
			WindowSettings( 1024, 650, "eepp - TextView Wrapped Selection", WindowStyle::Default,
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

UTEST( FontRendering, TextSoftWrapPos ) {
	const auto runTest = [&]() {
		UIApplication app(
			WindowSettings( 1024, 768, "eepp - Text Soft Wrap Pos", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		FontTrueType* font = FontTrueType::New( "DejaVuSansMono" );
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

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
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
		std::vector<Rectf> rects = text.getSelectionRects( { 0, 4 } ); // "Line"
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
		std::vector<Rectf> rects = text.getSelectionRects( { 2, 9 } );
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
		std::vector<Rectf> rects =
			text.getSelectionRects( { 0, static_cast<Int64>( txt.size() ) } );
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

		std::vector<Rectf> rects =
			text.getSelectionRects( { 0, static_cast<Int64>( text.getString().size() ) } );
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

		FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
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

		FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
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
