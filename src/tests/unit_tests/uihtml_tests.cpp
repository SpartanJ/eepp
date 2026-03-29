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
