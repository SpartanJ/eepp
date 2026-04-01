// #include "compareimages.hpp"
#include "utest.h"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
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

UTEST( UIHTMLTable, complexLayout ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "HTML Tables Test", WindowStyle::Default, WindowBackend::Default,
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
	sceneNode->setURI( Sys::getProcessPath() + "assets/html/" );
	sceneNode->loadLayoutFromFile( "assets/html/hn_thread_test.html" );
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

	// EXPECT_LT( totalTds, mainTotal );
	// compareImages( utest_state, utest_result, win, "eepp-uihtmltable-complex-layout", "html" );

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
