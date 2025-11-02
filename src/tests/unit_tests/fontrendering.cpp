#include "utest.hpp"

#include <eepp/graphics/fontbmfont.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fontsprite.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/renderer/renderergl.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextview.hpp>
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
		std::string withTextShaper = Text::TextShaperEnabled ? "_text_shape" : "";
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
	text.wrapText( win->getWidth() - 96 );

	int size = (int)Txt.size();

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
		runTest();

		UTEST_PRINT_STEP( "Text Shaper enabled" );
		{
			BoolScopedOp op( Text::TextShaperEnabled, true );
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
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
		editor->loadFromFile( "assets/fontrendering/tabs_test.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(),
					   "eepp-editor-monospace-tabs-test" );
	};

	UTEST_PRINT_STEP( "Text Shaper disabled" );
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
		editor->loadFromFile( "assets/fontrendering/tabs_test.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(),
					   "eepp-editor-monospace-tab-stop-test" );
	};

	UTEST_PRINT_STEP( "Text Shaper disabled" );
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
		editor->loadFromFile( "assets/fontrendering/tabs_test.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-edit-tabs-test" );
	};

	UTEST_PRINT_STEP( "Text Shaper disabled" );
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
		editor->loadFromFile( "assets/fontrendering/tabs_test.txt" );
		SceneManager::instance()->update();
		SceneManager::instance()->draw();
		compareImages( utest_state, utest_result, app.getWindow(), "eepp-text-edit-tab-stop-test" );
	};

	UTEST_PRINT_STEP( "Text Shaper disabled" );
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
	runTest();

	UTEST_PRINT_STEP( "Text Shaper enabled" );
	{
		BoolScopedOp op( Text::TextShaperEnabled, true );
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
	FontTrueType* bengaliFont = FontTrueType::New( "NotoSansBengali-Regular",
												   "assets/fonts/NotoSansBengali-Regular.ttf" );
	FontManager::instance()->addFallbackFont( bengaliFont );
	UTEST_PRINT_STEP( "Text Shaper enabled" );
	auto* editor = UITextEdit::New();
	editor->setPixelsSize( app.getUI()->getPixelsSize() );
	editor->loadFromFile( "assets/textfiles/test-bengali.uext" );
	SceneManager::instance()->update();
	SceneManager::instance()->draw();
	compareImages( utest_state, utest_result, app.getWindow(), "eepp-textedit-bengali" );
}
