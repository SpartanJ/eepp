#include "compareimages.hpp"
#include "utest.h"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/htmlinput.hpp>
#include <eepp/ui/htmltextarea.hpp>
#include <eepp/ui/htmltextinput.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Tools;

static void init_ui_test() {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "HTML Tables Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
}

UTEST( UIHTMLTable, complexLayout ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 600, "HTML Tables Test", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string html;
	FileSystem::fileGet( "assets/html/hn_thread_test.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	/* while ( win->isRunning() ) */ {
		win->getInput()->update();
		SceneManager::instance()->update();

		win->clear();
		SceneManager::instance()->draw();
		win->display();
	}

	auto hnMain = sceneNode->getRoot()->find( "hnmain" );
	auto bigbox = sceneNode->getRoot()->find( "bigbox" );
	auto commentTree = sceneNode->getRoot()->findByClass( "comment-tree" );
	auto votelinks = sceneNode->getRoot()->findByClass( "votelinks" );
	auto commentTd = sceneNode->getRoot()->findByClass( "default" );
	auto comment = sceneNode->getRoot()->findByClass( "comment" );
	auto commtext = sceneNode->getRoot()->findByClass( "commtext" );

	EXPECT_GT( commentTree->getPixelsSize().getWidth(), 0 );
	EXPECT_GT( commentTree->getPixelsSize().getHeight(), 0 );

	EXPECT_GT( comment->getPixelsSize().getWidth(), 0 );
	EXPECT_GT( commtext->getPixelsSize().getWidth(), 0 );

	EXPECT_GT( commentTd->getPixelsSize().getWidth(), 0 );
	EXPECT_GT( commentTd->getPixelsSize().getHeight(), 0 );

	EXPECT_GE( hnMain->getPixelsSize().getHeight(), bigbox->getPixelsSize().getHeight() );
	Float totalTds = commentTd->getPixelsSize().getWidth() + votelinks->getPixelsSize().getWidth();
	Float mainTotal = hnMain->getPixelsSize().getWidth();

	EXPECT_GT( totalTds, 0 );
	EXPECT_GT( mainTotal, 0 );

	EXPECT_NEAR( totalTds, mainTotal, 0.1 );
	compareImages( utest_state, utest_result, win, "eepp-uihtmltable-complex-layout", "html" );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, complexLayout2 ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "HTML Tables Test 2", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string html;
	FileSystem::fileGet( "assets/html/hn_threaded_test.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-uihtmltable-complex-layout-2", "html" );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, complexLayout3 ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "HTML Tables Test 3", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string html;
	FileSystem::fileGet( "assets/html/hn_frontpage.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-uihtmltable-complex-layout-3", "html" );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, nestedPerformance ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "HTML Tables Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	// Create nested tables.
	UIHTMLTable* rootTable = UIHTMLTable::New();
	rootTable->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	rootTable->setParent( sceneNode->getRoot() );

	UIHTMLTable* currentTable = rootTable;
	for ( int i = 0; i < 10; ++i ) {
		UIHTMLTableRow* row = UIHTMLTableRow::New();
		row->setParent( currentTable );
		UIHTMLTableCell* cell = UIHTMLTableCell::New( "td" );
		cell->setParent( row );

		UIHTMLTable* childTable = UIHTMLTable::New();
		childTable->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		childTable->setParent( cell );
		currentTable = childTable;
	}

	UIHTMLTableRow* row = UIHTMLTableRow::New();
	row->setParent( currentTable );
	UIHTMLTableCell* cell = UIHTMLTableCell::New( "td" );
	cell->setParent( row );
	UITextSpan* span = UITextSpan::New();
	span->setParent( cell );
	span->setText( "Deeply nested text" );

	Clock clock;
	sceneNode->updateDirtyLayouts();

	Log::info( "Time for nested layout (10 levels): %.2f ms",
			   clock.getElapsedTime().asMilliseconds() );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, specifiedWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "HTML Tables Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	sceneNode->loadLayoutFromString(
		R"(<table style="width: 500px; height: wrap-content;">
			<tr>
				<td id="c1">C1</td>
				<td id="c2" style="width: 200px;">C2</td>
			</tr>
		</table>)" );

	sceneNode->updateDirtyLayouts();

	auto c1 = sceneNode->getRoot()->find( "c1" );
	auto c2 = sceneNode->getRoot()->find( "c2" );

	ASSERT_TRUE( c1 != nullptr );
	ASSERT_TRUE( c2 != nullptr );

	// Cell 2 should be at least 200px.
	EXPECT_GE( c2->getPixelsSize().getWidth(), 200.f );
	// Total width should be 500px (minus padding if any, but default is 0).
	EXPECT_NEAR( c1->getPixelsSize().getWidth() + c2->getPixelsSize().getWidth(), 500.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, nestedSpecifiedWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "HTML Tables Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	sceneNode->loadLayoutFromString(
		R"(<table style="width: 500px; height: wrap-content;">
			<tr>
				<td id="c1"><img style="width: 50px; height: 10px;" /></td>
				<td id="c2">Flexible text content that should take the rest of the space</td>
			</tr>
		</table>)" );

	sceneNode->updateDirtyLayouts();

	auto c1 = sceneNode->getRoot()->find( "c1" );
	auto c2 = sceneNode->getRoot()->find( "c2" );

	ASSERT_TRUE( c1 != nullptr );
	ASSERT_TRUE( c2 != nullptr );

	// Cell 1 should be exactly 50px because it's rigid (only contains fixed-width image)
	// and Cell 2 is flexible.
	EXPECT_NEAR( c1->getPixelsSize().getWidth(), 50.f, 1.f );
	EXPECT_NEAR( c2->getPixelsSize().getWidth(), 450.f, 1.f );

	Engine::destroySingleton();
}

UTEST( HTMLInput, sizeAttribute ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( R"html(
		<vbox layout_width="wrap_content" layout_height="wrap_content">
			<input id="i1" size="10" />
			<input id="i2" size="20" />
			<input id="i3" />
			<input id="i_pwd" type="password" />
			<input id="i_mode_pwd" input-mode="password" />
			<input id="i_chk" type="checkbox" />
		</vbox>
	)html" );

	auto c1 = sceneNode->getRoot()->find( "i1" )->asType<HTMLInput>();
	auto c2 = sceneNode->getRoot()->find( "i2" )->asType<HTMLInput>();
	auto c3 = sceneNode->getRoot()->find( "i3" )->asType<HTMLInput>();
	auto cp = sceneNode->getRoot()->find( "i_pwd" )->asType<HTMLInput>();
	auto cm = sceneNode->getRoot()->find( "i_mode_pwd" )->asType<HTMLInput>();
	auto cc = sceneNode->getRoot()->find( "i_chk" )->asType<HTMLInput>();

	ASSERT_TRUE( c1 != nullptr );
	ASSERT_TRUE( c2 != nullptr );
	ASSERT_TRUE( c3 != nullptr );
	ASSERT_TRUE( cp != nullptr );
	ASSERT_TRUE( cm != nullptr );
	ASSERT_TRUE( cc != nullptr );

	auto i1 = c1->getChildWidget()->asType<HTMLTextInput>();
	auto i2 = c2->getChildWidget()->asType<HTMLTextInput>();
	auto i3 = c3->getChildWidget()->asType<HTMLTextInput>();

	ASSERT_TRUE( i1 != nullptr );
	ASSERT_TRUE( i2 != nullptr );
	ASSERT_TRUE( i3 != nullptr );

	EXPECT_EQ( i1->getHtmlSize(), 10u );
	EXPECT_EQ( i2->getHtmlSize(), 20u );
	EXPECT_EQ( i3->getHtmlSize(), 20u );

	EXPECT_GT( i2->getPixelsSize().getWidth(), i1->getPixelsSize().getWidth() );
	EXPECT_NEAR( i2->getPixelsSize().getWidth(), i3->getPixelsSize().getWidth(), 1.f );

	EXPECT_TRUE( cp->getChildWidget()->isType( UI_TYPE_TEXTINPUT ) );
	EXPECT_TRUE( cp->getChildWidget()->asType<UITextInput>()->getMode() ==
				 UITextInput::TextInputMode::Password );
	EXPECT_TRUE( cp->getChildWidget()->asType<UITextInput>()->getMode() ==
				 UITextInput::TextInputMode::Password );
	EXPECT_TRUE( cm->getChildWidget()->asType<UITextInput>()->getMode() ==
				 UITextInput::TextInputMode::Password );
	EXPECT_TRUE( cc->getChildWidget()->isType( UI_TYPE_CHECKBOX ) );

	Engine::destroySingleton();
}

UTEST( HTMLTextArea, rowsColsAttribute ) {
	init_ui_test();
	auto* scene = SceneManager::instance()->getUISceneNode();
	auto* c1_raw = scene->loadLayoutFromString( R"html(
		<vbox layout_width="wrap_content" layout_height="wrap_content">
			<textarea id="t1" rows="2" cols="20"></textarea>
			<textarea id="t2" rows="4" cols="40"></textarea>
		</vbox>
	)html" );
	ASSERT_TRUE( c1_raw != nullptr );
	auto* t1 = c1_raw->find( "t1" )->asType<HTMLTextArea>();
	auto* t2 = c1_raw->find( "t2" )->asType<HTMLTextArea>();
	ASSERT_TRUE( t1 != nullptr );
	ASSERT_TRUE( t2 != nullptr );
	EXPECT_EQ( t1->getRows(), 2u );
	EXPECT_EQ( t1->getCols(), 20u );
	EXPECT_EQ( t2->getRows(), 4u );
	EXPECT_EQ( t2->getCols(), 40u );
	EXPECT_GT( t2->getPixelsSize().getWidth(), t1->getPixelsSize().getWidth() );
	EXPECT_GT( t2->getPixelsSize().getHeight(), t1->getPixelsSize().getHeight() );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, tableLayoutFixed ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "HTML Tables Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	sceneNode->loadLayoutFromString(
		R"(<table style="width: 600px; table-layout: fixed;">
			<tr>
				<td id="c1" style="width: 100px;">C1</td>
				<td id="c2" style="width: 200px;">C2</td>
				<td id="c3">C3</td>
			</tr>
			<tr>
				<td style="width: 500px;">C4 (Should be ignored)</td>
				<td>C5</td>
				<td>C6</td>
			</tr>
		</table>)" );

	sceneNode->updateDirtyLayouts();

	auto c1 = sceneNode->getRoot()->find( "c1" );
	auto c2 = sceneNode->getRoot()->find( "c2" );
	auto c3 = sceneNode->getRoot()->find( "c3" );

	ASSERT_TRUE( c1 != nullptr );
	ASSERT_TRUE( c2 != nullptr );
	ASSERT_TRUE( c3 != nullptr );

	// Total width is 600px. C1=100, C2=200, C3 takes remaining 300px.
	EXPECT_NEAR( c1->getPixelsSize().getWidth(), 100.f, 1.f );
	EXPECT_NEAR( c2->getPixelsSize().getWidth(), 200.f, 1.f );
	EXPECT_NEAR( c3->getPixelsSize().getWidth(), 300.f, 1.f );

	Engine::destroySingleton();
}
