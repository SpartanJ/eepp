#include "compareimages.hpp"
#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uihtmldetails.hpp>
#include <eepp/ui/uihtmlimage.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uihtmltextarea.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uimarkdownview.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

#include <cstdlib>
#include <iostream>

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
	FontTrueType* monospace = FontTrueType::New( "monospace" );
	monospace->loadFromFile( "../assets/fonts/DejaVuSansMono.ttf" );
	FontFamily::loadFromRegular( monospace );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->setColorSchemePreference( ColorSchemePreference::Light );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
}

static String uiHtmlRenderedText( const RichText& richText ) {
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

static String uiHtmlLineText( const RichText& richText, size_t lineIndex ) {
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

UTEST( UIHTMLTable, complexLayout ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "HTML Tables Test", VisualTestWindowStyle,
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
		WindowSettings( 1024, 650, "HTML Tables Test 2", VisualTestWindowStyle,
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
	// Keep this fixture close to the captured viewport. Apple Software Renderer drops the
	// large offscreen solid-background quad if the original full HN thread is used, even
	// though GPU renderers and other software renderers rasterize it correctly.
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	const Color expectedMainBg( "#f6f6ef" );
	UIWidget* hnMain = nullptr;
	for ( int i = 0; i < 8; i++ ) {
		win->getInput()->update();
		SceneManager::instance()->update();

		if ( !hnMain )
			hnMain = sceneNode->getRoot()->find( "hnmain" )->asType<UIWidget>();

		win->clear();
		SceneManager::instance()->draw();
		win->display();

		if ( hnMain && hnMain->getBackgroundColor() == expectedMainBg )
			break;
	}

	ASSERT_TRUE( hnMain != nullptr );
	EXPECT_STDSTREQ( hnMain->getBackgroundColor().toHexString(), expectedMainBg.toHexString() );

	compareImages( utest_state, utest_result, win, "eepp-uihtmltable-complex-layout-2", "html" );

	Engine::destroySingleton();
}

UTEST( UIHTML, redditOldThreadWebViewSmoke ) {
	if ( std::getenv( "EEPP_REDDIT_OLD_THREAD_VISUAL" ) == nullptr )
		UTEST_SKIP( "set EEPP_REDDIT_OLD_THREAD_VISUAL=1 to render the old Reddit fixture" );

	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 1000, "Old Reddit Thread Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	if ( !FileSystem::fileExists( "assets/html/reddit_old_thread_files/reddit.ETA_etA2z5U.css" ) ) {

		Engine::destroySingleton();
		UTEST_SKIP( "old Reddit fixture CSS asset is not readable" );
	}

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( win->getWidth(), win->getHeight() );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->loadURI(
		URI( "file://" + Sys::getProcessPath() + "assets/html/reddit_old_thread.html" ) );

	win->setClearColor( Color::White );
	for ( int i = 0; i < 8; i++ ) {
		win->getInput()->update();
		SceneManager::instance()->update();
		win->clear();
		SceneManager::instance()->draw();
		win->display();
	}

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	UIWidget* documentRoot = documentScene->getRoot();
	ASSERT_TRUE( documentRoot != nullptr );
	EXPECT_TRUE( sceneNode->getRoot()->findByClass( "side" ) == nullptr );

	auto side = documentRoot->findByClass( "side" );
	auto siteTable = documentRoot->find( "siteTable" );
	auto midcol = documentRoot->findByClass( "midcol" );
	auto entry = documentRoot->findByClass( "entry" );
	auto arrow = documentRoot->findByClass( "arrow" );
	auto srHeader = documentRoot->find( "sr-header-area" );
	auto redesignButton = documentRoot->find( "redesign-beta-optin-btn" );
	auto srDrop = documentRoot->querySelector( "#sr-header-area .dropdown.srdrop" );
	auto srList = documentRoot->querySelector( "#sr-header-area .sr-list" );
	auto srFlatList = documentRoot->querySelector( "#sr-header-area .sr-list .flat-list" );
	auto headerBottomLeft = documentRoot->find( "header-bottom-left" );
	auto headerBottomRight = documentRoot->find( "header-bottom-right" );
	auto header = documentRoot->find( "header" );
	auto dropChoices = documentRoot->querySelector( ".drop-choices.srdrop" );
	auto selftextMd = documentRoot->querySelector( ".link .usertext-body .md" );
	auto selftextFirstP = documentRoot->querySelector( ".link .usertext-body .md p" );
	auto postTitle = documentRoot->querySelector( ".link .top-matter > p.title" );
	auto postTagline = documentRoot->querySelector( ".link .top-matter > p.tagline" );
	auto postExpando = documentRoot->querySelector( ".link .expando" );
	auto postUsertext = documentRoot->querySelector( ".link .expando > form.usertext" );
	auto commentArea = documentRoot->querySelector( ".commentarea" );
	auto commentForm = documentRoot->find( "form-t3_1tk9dgh5ov" );
	auto commentEdit = documentRoot->querySelector( ".commentarea .usertext-edit" );
	auto commentTextarea = documentRoot->querySelector( ".commentarea textarea" );
	auto commentHelpToggle = documentRoot->querySelector( ".commentarea .help-toggle" );
	auto commentContentPolicy = documentRoot->querySelector( ".commentarea a.reddiquette" );
	auto flairCheckbox = documentRoot->find( "flair_enabled" );

	ASSERT_TRUE( side != nullptr );
	ASSERT_TRUE( siteTable != nullptr );
	ASSERT_TRUE( midcol != nullptr );
	ASSERT_TRUE( entry != nullptr );
	ASSERT_TRUE( arrow != nullptr );
	ASSERT_TRUE( srHeader != nullptr );
	ASSERT_TRUE( redesignButton != nullptr );
	ASSERT_TRUE( srDrop != nullptr );
	ASSERT_TRUE( srList != nullptr );
	ASSERT_TRUE( srFlatList != nullptr );
	ASSERT_TRUE( headerBottomLeft != nullptr );
	ASSERT_TRUE( headerBottomRight != nullptr );
	ASSERT_TRUE( header != nullptr );
	ASSERT_TRUE( dropChoices != nullptr );
	ASSERT_TRUE( selftextMd != nullptr );
	ASSERT_TRUE( selftextFirstP != nullptr );
	ASSERT_TRUE( postTitle != nullptr );
	ASSERT_TRUE( postTagline != nullptr );
	ASSERT_TRUE( postExpando != nullptr );
	ASSERT_TRUE( postUsertext != nullptr );
	ASSERT_TRUE( commentArea != nullptr );
	ASSERT_TRUE( commentForm != nullptr );
	ASSERT_TRUE( commentEdit != nullptr );
	ASSERT_TRUE( commentTextarea != nullptr );
	ASSERT_TRUE( commentHelpToggle != nullptr );
	ASSERT_TRUE( commentContentPolicy != nullptr );
	ASSERT_TRUE( flairCheckbox != nullptr );

	UIWidget* content =
		siteTable->getParent()->isWidget() ? siteTable->getParent()->asType<UIWidget>() : nullptr;
	ASSERT_TRUE( content != nullptr );

	Vector2f sidePos = side->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f contentPos = content->convertToWorldSpace( { 0, 0 } );
	Vector2f midcolPos = midcol->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f entryPos = entry->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f arrowPos = arrow->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f srHeaderPos = srHeader->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f redesignButtonPos =
		redesignButton->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f srDropPos = srDrop->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f srListPos = srList->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f srFlatListPos = srFlatList->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f headerBottomLeftPos =
		headerBottomLeft->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f headerBottomRightPos =
		headerBottomRight->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f headerPos = header->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f dropChoicesPos = dropChoices->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f selftextMdPos = selftextMd->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f selftextFirstPPos =
		selftextFirstP->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f postTitlePos = postTitle->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f postTaglinePos = postTagline->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f postExpandoPos = postExpando->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f postUsertextPos = postUsertext->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f commentAreaPos = commentArea->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f commentFormPos = commentForm->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f commentEditPos = commentEdit->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f commentTextareaPos =
		commentTextarea->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f commentHelpTogglePos =
		commentHelpToggle->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );
	Vector2f commentContentPolicyPos =
		commentContentPolicy->asType<UIWidget>()->convertToWorldSpace( { 0, 0 } );

	auto fontSizeOf = []( Node* node ) -> Uint32 {
		return node->isType( UI_TYPE_RICHTEXT ) ? node->asType<UIRichText>()->getFontSize() : 0;
	};

	std::cerr << "old reddit rects: "
			  << "side=(" << sidePos.x << "," << sidePos.y << " "
			  << side->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << side->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "content=(" << contentPos.x << "," << contentPos.y << " "
			  << content->getPixelsSize().getWidth() << "x" << content->getPixelsSize().getHeight()
			  << ") "
			  << "midcol=(" << midcolPos.x << "," << midcolPos.y << " "
			  << midcol->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << midcol->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "entry=(" << entryPos.x << "," << entryPos.y << " "
			  << entry->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << entry->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "arrow=(" << arrowPos.x << "," << arrowPos.y << " "
			  << arrow->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << arrow->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "srHeader=(" << srHeaderPos.x << "," << srHeaderPos.y << " "
			  << srHeader->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << srHeader->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "redesignButton=(" << redesignButtonPos.x << "," << redesignButtonPos.y << " "
			  << redesignButton->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << redesignButton->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "srDrop=(" << srDropPos.x << "," << srDropPos.y << " "
			  << srDrop->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << srDrop->asType<UIWidget>()->getPixelsSize().getHeight() << " float="
			  << CSSFloatHelper::toString( srDrop->asType<UIHTMLWidget>()->getCSSFloat() ) << ") "
			  << "srList=(" << srListPos.x << "," << srListPos.y << " "
			  << srList->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << srList->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " lines=" << srList->asType<UIRichText>()->getRichTextPtr()->getLines().size()
			  << " wrap=" << srList->asType<UIRichText>()->getLineWrap() << ") "
			  << "headerBottomLeft=(" << headerBottomLeftPos.x << "," << headerBottomLeftPos.y
			  << " " << headerBottomLeft->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << headerBottomLeft->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "headerBottomRight=(" << headerBottomRightPos.x << "," << headerBottomRightPos.y
			  << " " << headerBottomRight->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << headerBottomRight->asType<UIWidget>()->getPixelsSize().getHeight() << " position="
			  << CSSPositionHelper::toString(
					 headerBottomRight->asType<UIHTMLWidget>()->getCSSPosition() )
			  << ") "
			  << "header=(" << headerPos.x << "," << headerPos.y << " "
			  << header->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << header->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " offset=" << header->asType<UIWidget>()->getPixelsContentOffset().Left << ","
			  << header->asType<UIWidget>()->getPixelsContentOffset().Top << ","
			  << header->asType<UIWidget>()->getPixelsContentOffset().Right << ","
			  << header->asType<UIWidget>()->getPixelsContentOffset().Bottom << ") "
			  << "dropChoices=(" << dropChoicesPos.x << "," << dropChoicesPos.y << " "
			  << dropChoices->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << dropChoices->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " visible=" << dropChoices->asType<UIWidget>()->isVisible() << " display="
			  << CSSDisplayHelper::toString( dropChoices->asType<UIHTMLWidget>()->getDisplay() )
			  << ") "
			  << "selftextMd=(" << selftextMdPos.x << "," << selftextMdPos.y << " "
			  << selftextMd->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << selftextMd->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " font=" << fontSizeOf( selftextMd ) << ") "
			  << "selftextFirstP=(" << selftextFirstPPos.x << "," << selftextFirstPPos.y << " "
			  << selftextFirstP->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << selftextFirstP->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " font=" << fontSizeOf( selftextFirstP ) << ") "
			  << "postTitle=(" << postTitlePos.x << "," << postTitlePos.y << " "
			  << postTitle->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << postTitle->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " font=" << fontSizeOf( postTitle ) << ") "
			  << "postTagline=(" << postTaglinePos.x << "," << postTaglinePos.y << " "
			  << postTagline->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << postTagline->asType<UIWidget>()->getPixelsSize().getHeight()
			  << " font=" << fontSizeOf( postTagline ) << ") "
			  << "postExpando=(" << postExpandoPos.x << "," << postExpandoPos.y << " "
			  << postExpando->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << postExpando->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "postUsertext=(" << postUsertextPos.x << "," << postUsertextPos.y << " "
			  << postUsertext->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << postUsertext->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "commentArea=(" << commentAreaPos.x << "," << commentAreaPos.y << " "
			  << commentArea->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << commentArea->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "commentForm=(" << commentFormPos.x << "," << commentFormPos.y << " "
			  << commentForm->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << commentForm->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "commentEdit=(" << commentEditPos.x << "," << commentEditPos.y << " "
			  << commentEdit->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << commentEdit->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "commentTextarea=(" << commentTextareaPos.x << "," << commentTextareaPos.y << " "
			  << commentTextarea->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << commentTextarea->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "commentContentPolicy=(" << commentContentPolicyPos.x << ","
			  << commentContentPolicyPos.y << " "
			  << commentContentPolicy->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << commentContentPolicy->asType<UIWidget>()->getPixelsSize().getHeight() << ") "
			  << "commentHelpToggle=(" << commentHelpTogglePos.x << "," << commentHelpTogglePos.y
			  << " " << commentHelpToggle->asType<UIWidget>()->getPixelsSize().getWidth() << "x"
			  << commentHelpToggle->asType<UIWidget>()->getPixelsSize().getHeight() << ")"
			  << std::endl;

	const Float midcolCenter =
		midcolPos.x + midcol->asType<UIWidget>()->getPixelsSize().getWidth() / 2.f;
	const Float arrowCenter =
		arrowPos.x + arrow->asType<UIWidget>()->getPixelsSize().getWidth() / 2.f;
	EXPECT_NEAR( midcolCenter, arrowCenter, 1.f );
	EXPECT_NEAR( srListPos.y, srHeaderPos.y, 1.f );
	EXPECT_GE( srListPos.x,
			   srDropPos.x + srDrop->asType<UIWidget>()->getPixelsSize().getWidth() - 1.f );
	EXPECT_LE( srList->asType<UIWidget>()->getPixelsSize().getWidth(),
			   win->getWidth() - srListPos.x + 1.f );
	EXPECT_LE( srList->asType<UIWidget>()->getPixelsSize().getHeight(),
			   srHeader->asType<UIWidget>()->getPixelsSize().getHeight() + 1.f );
	EXPECT_EQ( srFlatList->asType<UIRichText>()->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_NEAR( srFlatListPos.y, srHeaderPos.y, 1.f );
	EXPECT_GE( headerBottomLeftPos.y,
			   srHeaderPos.y + srHeader->asType<UIWidget>()->getPixelsSize().getHeight() - 1.f );
	EXPECT_EQ( headerBottomRight->asType<UIHTMLWidget>()->getCSSPosition(), CSSPosition::Absolute );
	EXPECT_NEAR( headerBottomRightPos.x +
					 headerBottomRight->asType<UIWidget>()->getPixelsSize().getWidth(),
				 headerPos.x + header->asType<UIWidget>()->getPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( headerBottomRightPos.y +
					 headerBottomRight->asType<UIWidget>()->getPixelsSize().getHeight(),
				 headerPos.y + header->asType<UIWidget>()->getPixelsSize().getHeight() -
					 header->asType<UIWidget>()->getPixelsContentOffset().Bottom,
				 1.f );
	EXPECT_FALSE( dropChoices->asType<UIWidget>()->isVisible() );
	EXPECT_EQ( dropChoices->asType<UIHTMLWidget>()->getDisplay(), CSSDisplay::None );
	ASSERT_TRUE( flairCheckbox->isType( UI_TYPE_HTML_INPUT ) );
	ASSERT_TRUE( flairCheckbox->asType<UIHTMLInput>()->getChildWidget() != nullptr );
	EXPECT_TRUE(
		flairCheckbox->asType<UIHTMLInput>()->getChildWidget()->asType<UICheckBox>()->isChecked() );
	EXPECT_NEAR(
		commentHelpTogglePos.x + commentHelpToggle->asType<UIWidget>()->getPixelsSize().getWidth(),
		commentEditPos.x + commentEdit->asType<UIWidget>()->getPixelsSize().getWidth(), 1.f );
	EXPECT_LE( commentContentPolicyPos.x +
				   commentContentPolicy->asType<UIWidget>()->getPixelsSize().getWidth(),
			   commentHelpTogglePos.x -
				   commentHelpToggle->asType<UIWidget>()->getLayoutPixelsMargin().Left + 1.f );

	auto* arrowBackground = arrow->asType<UIWidget>()->getBackground();
	ASSERT_TRUE( arrowBackground != nullptr );
	auto* arrowBackgroundLayer = arrowBackground->getLayer( 0 );
	ASSERT_TRUE( arrowBackgroundLayer != nullptr );
	ASSERT_TRUE( arrowBackgroundLayer->getDrawable() != nullptr );
	EXPECT_EQ( arrowBackgroundLayer->getDrawable()->getPixelsSize().getWidth(), 140 );
	EXPECT_EQ( arrowBackgroundLayer->getDrawable()->getPixelsSize().getHeight(), 1751 );
	EXPECT_STDSTREQ( arrowBackgroundLayer->getPositionX(), "-42px" );
	EXPECT_STDSTREQ( arrowBackgroundLayer->getPositionY(), "-1678px" );
	EXPECT_NEAR( arrowBackgroundLayer->getOffset().x, -42.f, 0.1f );
	EXPECT_NEAR( arrowBackgroundLayer->getOffset().y, -1678.f, 0.1f );

	if ( !FileSystem::fileExists( "output" ) )
		FileSystem::makeDir( "output" );
	win->getFrontBufferImage().saveToFile( "output/eepp-reddit-old-thread-current.webp",
										   Image::SaveType::WEBP );

	Engine::destroySingleton();
}

UTEST( UIHTML, StrikeElementUsesDefaultLineThrough ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML(
		R"html(<body><p><strike id="strike">old</strike></p></body>)html" ) );
	SceneManager::instance()->update();

	auto* strike = sceneNode->getRoot()->find( "strike" )->asType<UITextSpan>();
	ASSERT_TRUE( strike != nullptr );
	EXPECT_TRUE( 0 != ( strike->getTextDecoration() & Text::StrikeThrough ) );

	Engine::destroySingleton();
}

UTEST( UIHTML, FontSizeAbsoluteKeywordsUseBrowserScale ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<div id="xsmall" style="font-size: x-small">x-small</div>
			<div id="small" style="font-size: small">small</div>
			<div id="medium" style="font-size: medium">medium</div>
			<div id="large" style="font-size: large">large</div>
			<div id="parent" style="font-size: 20px">
				<span id="smaller" style="font-size: smaller">smaller</span>
				<span id="larger" style="font-size: larger">larger</span>
			</div>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto fontSize = [sceneNode]( const std::string& id ) {
		auto* node = sceneNode->getRoot()->find( id );
		return node->isType( UI_TYPE_TEXTSPAN ) ? node->asType<UITextSpan>()->getFontSize()
												: node->asType<UIRichText>()->getFontSize();
	};

	EXPECT_EQ( fontSize( "xsmall" ), 10u );
	EXPECT_EQ( fontSize( "small" ), 13u );
	EXPECT_EQ( fontSize( "medium" ), 16u );
	EXPECT_EQ( fontSize( "large" ), 18u );
	EXPECT_EQ( fontSize( "smaller" ), 17u );
	EXPECT_EQ( fontSize( "larger" ), 24u );

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceNowrapDisablesSoftWrap ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	const std::string text = "alpha beta gamma delta epsilon zeta eta theta iota kappa lambda mu";

	sceneNode->loadLayoutFromString(
		HTMLFormatter::HTMLtoXML( String::format( R"html(
			<body>
				<div id="wrap" style="width: 120px; font-size: 16px">%s</div>
				<div id="nowrap" style="width: 120px; font-size: 16px; white-space: nowrap">
					%s
				</div>
			</body>
		)html",
												  text.c_str(), text.c_str() ) ) );
	SceneManager::instance()->update();

	auto* wrap = sceneNode->getRoot()->find( "wrap" )->asType<UIRichText>();
	auto* nowrap = sceneNode->getRoot()->find( "nowrap" )->asType<UIRichText>();
	ASSERT_TRUE( wrap != nullptr );
	ASSERT_TRUE( nowrap != nullptr );

	EXPECT_GT( wrap->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_EQ( nowrap->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_FALSE( nowrap->getLineWrap() );

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceNowrapKeepsInlineListOnOneLine ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<div id="bar" style="width: 120px; white-space: nowrap; overflow: hidden">
				<ul id="list" style="display: inline; list-style: none; margin: 0; padding: 0">
					<li style="display: inline; white-space: nowrap">alpha</li>
					<li style="display: inline; white-space: nowrap">beta</li>
					<li style="display: inline; white-space: nowrap">gamma</li>
					<li style="display: inline; white-space: nowrap">delta</li>
					<li style="display: inline; white-space: nowrap">epsilon</li>
					<li style="display: inline; white-space: nowrap">zeta</li>
					<li style="display: inline; white-space: nowrap">eta</li>
					<li style="display: inline; white-space: nowrap">theta</li>
				</ul>
			</div>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* bar = sceneNode->getRoot()->find( "bar" )->asType<UIRichText>();
	auto* list = sceneNode->getRoot()->find( "list" )->asType<UIRichText>();
	ASSERT_TRUE( bar != nullptr );
	ASSERT_TRUE( list != nullptr );

	EXPECT_EQ( bar->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_FALSE( bar->getLineWrap() );
	EXPECT_EQ( list->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_FALSE( list->getLineWrap() );

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceNowrapContinuesInlineContentAfterOverflow ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<div id="bar" style="width: 140px; white-space: nowrap; overflow: hidden">
				<ul id="first" style="display: inline; list-style: none; margin: 0; padding: 0">
					<li style="display: inline">home</li>
					<li style="display: inline"> - popular</li>
					<li style="display: inline"> - all</li>
					<li style="display: inline"> - friends</li>
				</ul>
				<span>|</span>
				<ul id="second" style="display: inline; list-style: none; margin: 0; padding: 0">
					<li style="display: inline">movies</li>
					<li style="display: inline"> - videos</li>
					<li style="display: inline"> - pcgaming</li>
					<li style="display: inline"> - gamedev</li>
					<li style="display: inline"> - science</li>
					<li style="display: inline"> - space</li>
				</ul>
			</div>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* bar = sceneNode->getRoot()->find( "bar" )->asType<UIRichText>();
	auto* first = sceneNode->getRoot()->find( "first" )->asType<UIRichText>();
	auto* second = sceneNode->getRoot()->find( "second" )->asType<UIRichText>();
	ASSERT_TRUE( bar != nullptr );
	ASSERT_TRUE( first != nullptr );
	ASSERT_TRUE( second != nullptr );

	EXPECT_FALSE( bar->getLineWrap() );
	EXPECT_FALSE( first->getLineWrap() );
	EXPECT_FALSE( second->getLineWrap() );
	EXPECT_EQ( bar->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_EQ( first->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_EQ( second->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_NEAR( first->convertToWorldSpace( { 0, 0 } ).y,
				 second->convertToWorldSpace( { 0, 0 } ).y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceCollapsePreCodePreservesIndentation ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<pre id="pre"><code id="code">if (x) {
    return 1;
}</code></pre>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* pre = sceneNode->getRoot()->find( "pre" )->asType<UIRichText>();
	auto* code = sceneNode->getRoot()->find( "code" );
	ASSERT_TRUE( pre != nullptr );
	ASSERT_TRUE( code != nullptr );

	EXPECT_FALSE( code->isType( UI_TYPE_CODEEDITOR ) );
	EXPECT_GE( pre->getRichTextPtr()->getLines().size(), (size_t)3 );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 0 ), "if (x) {" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 1 ), "    return 1;" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 2 ), "}" );

	Engine::destroySingleton();
}

UTEST( UIHTML, PreCodeSimpleFixtureKeepsCompactCodeLines ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/pre.code.html", html ) );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	SceneManager::instance()->update();

	UIRichText* pre = nullptr;
	Node* code = nullptr;
	sceneNode->getRoot()->forEachNode( [&pre, &code]( Node* node ) {
		if ( node->isWidget() && node->asType<UIWidget>()->getElementTag() == "pre" )
			pre = node->asType<UIRichText>();
		if ( node->isWidget() && node->asType<UIWidget>()->getElementTag() == "code" )
			code = node;
	} );
	ASSERT_TRUE( pre != nullptr );
	ASSERT_TRUE( code != nullptr );
	EXPECT_FALSE( code->isType( UI_TYPE_CODEEDITOR ) );
	auto* codeSpan = code->asType<UITextSpan>();
	ASSERT_TRUE( codeSpan != nullptr );

	const auto& lines = pre->getRichTextPtr()->getLines();
	ASSERT_GE( lines.size(), (size_t)3 );
	ASSERT_LE( lines.size(), (size_t)4 );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 0 ), "void main() {" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 1 ), "    printf(\"Hello World\");" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 2 ), "}" );
	if ( lines.size() == 4 )
		EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 3 ), "" );
	ASSERT_FALSE( lines[1].spans.empty() );
	ASSERT_TRUE( lines[1].spans[0].text != nullptr );
	ASSERT_FALSE( lines[1].spans[0].text->getString().empty() );
	EXPECT_TRUE( lines[1].spans[0].text->getString()[0] != '\n' );
	ASSERT_FALSE( lines[2].spans.empty() );
	ASSERT_TRUE( lines[2].spans[0].text != nullptr );
	ASSERT_FALSE( lines[2].spans[0].text->getString().empty() );
	EXPECT_TRUE( lines[2].spans[0].text->getString()[0] != '\n' );

	ASSERT_EQ( codeSpan->getHitBoxes().size(), (size_t)3 );
	Float expectedLineHeight = pre->getLineHeightPx();
	if ( expectedLineHeight <= 0.f ) {
		for ( const auto& span : lines[0].spans ) {
			if ( span.type == RichText::RenderSpan::Type::Text && span.text &&
				 !span.text->getString().empty() ) {
				const auto& fontStyle = span.text->getFontStyleConfig();
				ASSERT_TRUE( fontStyle.Font != nullptr );
				expectedLineHeight = fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );
				break;
			}
		}
	}
	ASSERT_GT( expectedLineHeight, 0.f );
	Float compactLineLimit = expectedLineHeight * 1.25f;
	for ( size_t i = 1; i < 3; ++i ) {
		Float lineDelta = lines[i].y - lines[i - 1].y;
		EXPECT_LE( lineDelta, compactLineLimit );
		EXPECT_LE( lines[i].height, compactLineLimit );

		const Rectf& prev = codeSpan->getHitBoxes()[i - 1];
		const Rectf& cur = codeSpan->getHitBoxes()[i];
		EXPECT_NEAR( cur.Top - prev.Top, lineDelta, 1.f );
		EXPECT_LE( cur.Top - prev.Top, compactLineLimit );
	}

	Engine::destroySingleton();
}

UTEST( UIHTML, PreCodeFixtureDiscardsFinalNewlineBeforeEndTag ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/pre.code.2.html", html ) );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	SceneManager::instance()->update();

	UIRichText* pre = nullptr;
	Node* code = nullptr;
	sceneNode->getRoot()->forEachNode( [&pre, &code]( Node* node ) {
		if ( node->isWidget() && node->asType<UIWidget>()->getElementTag() == "pre" )
			pre = node->asType<UIRichText>();
		if ( node->isWidget() && node->asType<UIWidget>()->getElementTag() == "code" )
			code = node;
	} );
	ASSERT_TRUE( pre != nullptr );
	ASSERT_TRUE( code != nullptr );
	EXPECT_FALSE( code->isType( UI_TYPE_CODEEDITOR ) );

	const auto& lines = pre->getRichTextPtr()->getLines();
	ASSERT_EQ( lines.size(), (size_t)3 );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 0 ), "int x = 42;" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 1 ), "const char* y = \"hello\";" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 2 ), "int z = foo(x, y);" );

	Engine::destroySingleton();
}

UTEST( UIHTML, PreCodeUsesCodeEditorOnlyForMarkdownAncestorOrGlobalOptIn ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( R"xml(
		<markdownview id="md">
			<pre id="md_pre"><code id="md_code" class="language-cpp">void main() {
    printf("Hello World");
}</code></pre>
		</markdownview>
	)xml" );
	SceneManager::instance()->update();

	auto* markdownCode = sceneNode->getRoot()->find( "md_code" );
	ASSERT_TRUE( markdownCode != nullptr );
	ASSERT_TRUE( markdownCode->isType( UI_TYPE_CODEEDITOR ) );
	EXPECT_TRUE( markdownCode->asType<UICodeEditor>()->getDocument().getText().find( "printf" ) !=
				 String::InvalidPos );

	Engine::destroySingleton();

	init_ui_test();
	sceneNode = SceneManager::instance()->getUISceneNode();
	UIRichText::setUseCodeEditorForPreCodeBlocks( true );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<pre id="pre"><code id="code">int value = 1;</code></pre>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* optInCode = sceneNode->getRoot()->find( "code" );
	ASSERT_TRUE( optInCode != nullptr );
	EXPECT_TRUE( optInCode->isType( UI_TYPE_CODEEDITOR ) );
	UIRichText::setUseCodeEditorForPreCodeBlocks( false );

	Engine::destroySingleton();
}

UTEST( UIHTML, PreCodeBlockFixtureKeepsCompactCodeLines ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/pre_code_block.html", html ) );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	SceneManager::instance()->update();

	UIRichText* pre = nullptr;
	sceneNode->getRoot()->forEachNode( [&pre]( Node* node ) {
		if ( pre == nullptr && node->isWidget() &&
			 node->asType<UIWidget>()->getElementTag() == "pre" )
			pre = node->asType<UIRichText>();
	} );
	ASSERT_TRUE( pre != nullptr );

	const auto& lines = pre->getRichTextPtr()->getLines();
	ASSERT_EQ( lines.size(), (size_t)19 );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 0 ), "let stepper () =" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 1 ),
					 "    // Execute a single instruction" );
	EXPECT_STRINGEQ( uiHtmlLineText( *pre->getRichTextPtr(), 18 ), "    mCycles " );

	Float expectedLineHeight = pre->getLineHeightPx();
	ASSERT_GT( expectedLineHeight, 0.f );
	Float compactLineLimit = expectedLineHeight * 1.25f;
	for ( size_t i = 1; i < lines.size(); ++i ) {
		EXPECT_LE( lines[i].y - lines[i - 1].y, compactLineLimit );
		EXPECT_LE( lines[i].height, compactLineLimit );
	}

	std::vector<UITextSpan*> codeLineSpans;
	for ( size_t i = 1; i <= 19; ++i ) {
		auto* lineSpan = sceneNode->getRoot()->find<UITextSpan>( String::format( "cb1-%zu", i ) );
		ASSERT_TRUE( lineSpan != nullptr );
		ASSERT_EQ( lineSpan->getHitBoxes().size(), (size_t)1 );
		EXPECT_LE( lineSpan->getHitBoxes()[0].getHeight(), compactLineLimit );
		codeLineSpans.push_back( lineSpan );
	}
	for ( size_t i = 1; i < codeLineSpans.size(); ++i ) {
		const Rectf& prev = codeLineSpans[i - 1]->getHitBoxes()[0];
		const Rectf& cur = codeLineSpans[i]->getHitBoxes()[0];
		Float prevTop = codeLineSpans[i - 1]->getPixelsPosition().y + prev.Top;
		Float curTop = codeLineSpans[i]->getPixelsPosition().y + cur.Top;
		EXPECT_NEAR( curTop - prevTop, lines[i].y - lines[i - 1].y, 1.f );
		EXPECT_LE( curTop - prevTop, compactLineLimit );
	}

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceCollapsePreLinePreservesBreaksOnly ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<div id="preline" style="white-space: pre-line">alpha   beta
    gamma</div>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* preline = sceneNode->getRoot()->find( "preline" )->asType<UIRichText>();
	ASSERT_TRUE( preline != nullptr );

	EXPECT_EQ( preline->getRichTextPtr()->getLines().size(), (size_t)2 );
	EXPECT_STRINGEQ( uiHtmlLineText( *preline->getRichTextPtr(), 0 ), "alpha beta" );
	EXPECT_STRINGEQ( uiHtmlLineText( *preline->getRichTextPtr(), 1 ), " gamma" );

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceCollapseBreakSpacesAffectsIntrinsicWidth ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<div id="normal" style="white-space: pre-wrap; font-size: 16px">     </div>
			<div id="breakspaces" style="white-space: break-spaces; font-size: 16px">     </div>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* normal = sceneNode->getRoot()->find( "normal" )->asType<UIRichText>();
	auto* breakSpaces = sceneNode->getRoot()->find( "breakspaces" )->asType<UIRichText>();
	ASSERT_TRUE( normal != nullptr );
	ASSERT_TRUE( breakSpaces != nullptr );

	EXPECT_LT( normal->getMinIntrinsicWidth(), breakSpaces->getMinIntrinsicWidth() );
	EXPECT_GT( breakSpaces->getMinIntrinsicWidth(), 0.f );
	EXPECT_STRINGEQ( uiHtmlRenderedText( *breakSpaces->getRichTextPtr() ), "     " );

	Engine::destroySingleton();
}

UTEST( UIHTML, WhiteSpaceCollapsePreservedTabsUseTabSize ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body>
			<pre id="tab2" style="font-family: monospace; font-size: 16px; tab-size: 2">a	b</pre>
			<pre id="tab8" style="font-family: monospace; font-size: 16px; tab-size: 8">a	b</pre>
			<pre id="tabpx" style="font-family: monospace; font-size: 16px; tab-size: 32px">a	b</pre>
		</body>
	)html" ) );
	SceneManager::instance()->update();

	auto* tab2 = sceneNode->getRoot()->find( "tab2" )->asType<UIRichText>();
	auto* tab8 = sceneNode->getRoot()->find( "tab8" )->asType<UIRichText>();
	auto* tabpx = sceneNode->getRoot()->find( "tabpx" )->asType<UIRichText>();
	ASSERT_TRUE( tab2 != nullptr );
	ASSERT_TRUE( tab8 != nullptr );
	ASSERT_TRUE( tabpx != nullptr );

	ASSERT_EQ( tab2->getRichTextPtr()->getLines().size(), (size_t)1 );
	ASSERT_EQ( tab8->getRichTextPtr()->getLines().size(), (size_t)1 );
	ASSERT_EQ( tabpx->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_STRINGEQ( uiHtmlLineText( *tab2->getRichTextPtr(), 0 ), "a\tb" );
	EXPECT_STRINGEQ( uiHtmlLineText( *tab8->getRichTextPtr(), 0 ), "a\tb" );
	EXPECT_STRINGEQ( uiHtmlLineText( *tabpx->getRichTextPtr(), 0 ), "a\tb" );
	EXPECT_EQ( tab2->getTabSize(), 2u );
	EXPECT_EQ( tab8->getTabSize(), 8u );
	EXPECT_GT( tabpx->getTabSize(), 2u );

	const Float tab2Width = tab2->getRichTextPtr()->getLines()[0].width;
	const Float tab8Width = tab8->getRichTextPtr()->getLines()[0].width;
	const Float tabpxWidth = tabpx->getRichTextPtr()->getLines()[0].width;
	EXPECT_GT( tab8Width, tab2Width );
	EXPECT_GT( tabpxWidth, tab2Width );

	ASSERT_FALSE( tab2->getRichTextPtr()->getLines()[0].spans.empty() );
	ASSERT_FALSE( tab8->getRichTextPtr()->getLines()[0].spans.empty() );
	ASSERT_FALSE( tabpx->getRichTextPtr()->getLines()[0].spans.empty() );
	ASSERT_TRUE( tab2->getRichTextPtr()->getLines()[0].spans[0].text != nullptr );
	ASSERT_TRUE( tab8->getRichTextPtr()->getLines()[0].spans[0].text != nullptr );
	ASSERT_TRUE( tabpx->getRichTextPtr()->getLines()[0].spans[0].text != nullptr );
	EXPECT_EQ( tab2->getRichTextPtr()->getLines()[0].spans[0].text->getTabWidth(), 2u );
	EXPECT_EQ( tab8->getRichTextPtr()->getLines()[0].spans[0].text->getTabWidth(), 8u );
	EXPECT_EQ( tabpx->getRichTextPtr()->getLines()[0].spans[0].text->getTabWidth(),
			   tabpx->getTabSize() );

	Engine::destroySingleton();
}

UTEST( UIRichText, anchorMargins ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "Anchor Margins Test", WindowStyle::Default,
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
	FileSystem::fileGet( "assets/html/anchor_margins.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );

	for ( auto anchor : anchors ) {
		auto a = anchor->asType<UIAnchorSpan>();
		EXPECT_EQ( anchor->getPixelsSize().getHeight(),
				   a->getFont()->getLineSpacing( a->getFontSize() ) );
	}

	compareImages( utest_state, utest_result, win, "eepp-ui-anchor-margins", "html" );

	Engine::destroySingleton();
}

UTEST( UIRichText, spanPadding ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "Span Padding Test", WindowStyle::Default, WindowBackend::Default,
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
	FileSystem::fileGet( "assets/html/span_padding.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-span-padding", "html" );

	Engine::destroySingleton();
}

UTEST( UIRichText, anchorPadding ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "Anchor Span Padding Test", WindowStyle::Default,
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
	FileSystem::fileGet( "assets/html/anchor_padding.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-anchor-padding", "html" );

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );
	ASSERT_GE( anchors.size(), (size_t)1 );
	auto downloadLink = anchors[0]->asType<UIWidget>();
	EXPECT_NEAR( downloadLink->getPixelsSize().getWidth(), 81.f, 3.f );
	EXPECT_NEAR( downloadLink->getPixelsSize().getHeight(), 28.f, 3.f );

	Engine::destroySingleton();
}

UTEST( UIRichText, anchorPaddingLineHeight ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "Anchor Padding LineHeight Test", WindowStyle::Default,
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
	FileSystem::fileGet( "assets/html/anchor_padding_lineheight.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-anchor-padding-lineheight", "html" );

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );
	ASSERT_GE( anchors.size(), (size_t)1 );
	auto downloadLink = anchors[0]->asType<UIWidget>();
	EXPECT_NEAR( downloadLink->getPixelsSize().getWidth(), 81.f, 3.f );
	EXPECT_NEAR( downloadLink->getPixelsSize().getHeight(), 28.f, 3.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBaselineAlignmentProperties ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Inline Baseline Alignment Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	sceneNode->loadLayoutFromString( R"html(
		<vbox layout_width="wrap_content" layout_height="wrap_content">
			<RichText id="baseline" font-size="20dp" layout_width="wrap_content" layout_height="wrap_content">
				A<span id="base_box" display="inline-block" layout_width="20dp" layout_height="20dp" vertical-align="baseline">X</span>
			</RichText>
			<RichText id="middle" font-size="20dp" layout_width="wrap_content" layout_height="wrap_content">
				A<span id="middle_box" display="inline-block" layout_width="20dp" layout_height="20dp" vertical-align="middle"><span id="middle_child">X</span></span>
			</RichText>
			<RichText id="alignment_middle" font-size="20dp" layout_width="wrap_content" layout_height="wrap_content">
				A<span id="alignment_box" display="inline-block" layout_width="20dp" layout_height="20dp" alignment-baseline="middle">X</span>
			</RichText>
		</vbox>
	)html" );
	sceneNode->updateDirtyLayouts();

	auto* baseline = sceneNode->getRoot()->find( "baseline" )->asType<UIRichText>();
	auto* middle = sceneNode->getRoot()->find( "middle" )->asType<UIRichText>();
	auto* alignmentMiddle = sceneNode->getRoot()->find( "alignment_middle" )->asType<UIRichText>();
	auto* baselineBox = sceneNode->getRoot()->find( "base_box" )->asType<UIHTMLWidget>();
	auto* middleBox = sceneNode->getRoot()->find( "middle_box" )->asType<UIHTMLWidget>();
	auto* middleChild = sceneNode->getRoot()->find( "middle_child" )->asType<UIHTMLWidget>();
	auto* alignmentBox = sceneNode->getRoot()->find( "alignment_box" )->asType<UIHTMLWidget>();
	ASSERT_TRUE( baseline != nullptr );
	ASSERT_TRUE( middle != nullptr );
	ASSERT_TRUE( alignmentMiddle != nullptr );
	ASSERT_TRUE( baselineBox != nullptr );
	ASSERT_TRUE( middleBox != nullptr );
	ASSERT_TRUE( middleChild != nullptr );
	ASSERT_TRUE( alignmentBox != nullptr );

	EXPECT_EQ( baselineBox->getBaselineAlign().type, CSSBaselineAlignment::Baseline );
	EXPECT_EQ( middleBox->getBaselineAlign().type, CSSBaselineAlignment::Middle );
	EXPECT_EQ( middleChild->getBaselineAlign().type, CSSBaselineAlignment::Baseline );
	EXPECT_EQ( alignmentBox->getBaselineAlign().type, CSSBaselineAlignment::Middle );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBlockVerticalAlignDoesNotInflateOwnTextLine ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Inline Block Vertical Align Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	sceneNode->loadLayoutFromString( R"html(
		<vbox layout_width="wrap_content" layout_height="wrap_content">
			<RichText id="baseline" font-size="16dp" layout_width="wrap_content" layout_height="wrap_content">
				A<span id="base_box" display="inline-block" font-size="10dp" vertical-align="baseline">self.cpp</span>
			</RichText>
			<RichText id="middle" font-size="16dp" layout_width="wrap_content" layout_height="wrap_content">
				A<span id="middle_box" display="inline-block" font-size="10dp" vertical-align="middle">self.cpp</span>
			</RichText>
		</vbox>
	)html" );
	sceneNode->updateDirtyLayouts();

	auto* baseline = sceneNode->getRoot()->find( "baseline" )->asType<UIRichText>();
	auto* middle = sceneNode->getRoot()->find( "middle" )->asType<UIRichText>();
	auto* baselineBox = sceneNode->getRoot()->find( "base_box" )->asType<UITextSpan>();
	auto* middleBox = sceneNode->getRoot()->find( "middle_box" )->asType<UITextSpan>();
	ASSERT_TRUE( baseline != nullptr );
	ASSERT_TRUE( middle != nullptr );
	ASSERT_TRUE( baselineBox != nullptr );
	ASSERT_TRUE( middleBox != nullptr );

	ASSERT_EQ( baselineBox->getRichTextPtr()->getLines().size(), (size_t)1 );
	ASSERT_EQ( middleBox->getRichTextPtr()->getLines().size(), (size_t)1 );
	EXPECT_NEAR( middleBox->getPixelsSize().getHeight(), baselineBox->getPixelsSize().getHeight(),
				 0.5f );
	EXPECT_NEAR( middleBox->getRichTextPtr()->getLines().front().height,
				 baselineBox->getRichTextPtr()->getLines().front().height, 0.5f );
	EXPECT_EQ( middleBox->getBaselineAlign().type, CSSBaselineAlignment::Middle );
	EXPECT_GE( middle->getRichTextPtr()->getLines().front().height,
			   baseline->getRichTextPtr()->getLines().front().height );

	Engine::destroySingleton();
}

UTEST( UIHTMLTable, complexLayout3 ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "HTML Tables Test 3", VisualTestWindowStyle,
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

UTEST( UIHTMLInput, sizeAttribute ) {
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
			<input id="i_chk_checked" type="checkbox" checked="checked" value="enabled" />
			<input id="i_radio_checked" type="radio" checked="checked" />
		</vbox>
	)html" );

	auto c1 = sceneNode->getRoot()->find( "i1" )->asType<UIHTMLInput>();
	auto c2 = sceneNode->getRoot()->find( "i2" )->asType<UIHTMLInput>();
	auto c3 = sceneNode->getRoot()->find( "i3" )->asType<UIHTMLInput>();
	auto cp = sceneNode->getRoot()->find( "i_pwd" )->asType<UIHTMLInput>();
	auto cm = sceneNode->getRoot()->find( "i_mode_pwd" )->asType<UIHTMLInput>();
	auto cc = sceneNode->getRoot()->find( "i_chk" )->asType<UIHTMLInput>();
	auto ccc = sceneNode->getRoot()->find( "i_chk_checked" )->asType<UIHTMLInput>();
	auto crc = sceneNode->getRoot()->find( "i_radio_checked" )->asType<UIHTMLInput>();

	ASSERT_TRUE( c1 != nullptr );
	ASSERT_TRUE( c2 != nullptr );
	ASSERT_TRUE( c3 != nullptr );
	ASSERT_TRUE( cp != nullptr );
	ASSERT_TRUE( cm != nullptr );
	ASSERT_TRUE( cc != nullptr );
	ASSERT_TRUE( ccc != nullptr );
	ASSERT_TRUE( crc != nullptr );

	auto i1 = c1->getChildWidget()->asType<UIHTMLTextInput>();
	auto i2 = c2->getChildWidget()->asType<UIHTMLTextInput>();
	auto i3 = c3->getChildWidget()->asType<UIHTMLTextInput>();

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
	EXPECT_TRUE( ccc->getChildWidget()->isType( UI_TYPE_CHECKBOX ) );
	EXPECT_TRUE( ccc->getChildWidget()->asType<UICheckBox>()->isChecked() );
	EXPECT_TRUE( ccc->getFormValue() == "enabled" );
	EXPECT_TRUE( crc->getChildWidget()->isType( UI_TYPE_RADIOBUTTON ) );
	EXPECT_TRUE( crc->getChildWidget()->asType<UIRadioButton>()->isActive() );
	EXPECT_TRUE( crc->getFormValue() == "on" );

	Engine::destroySingleton();
}

UTEST( UIHTML, DataProperties ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<html>
			<body>
				<div id="target"
					 data-role="hero"
					 data-tags="featured primary"
					 data-lang="en-US"
					 data-language="cpp"
					 data-id="user-42"
					 data-empty="">
				</div>
				<div id="missing-control"></div>
			</body>
		</html>
	)html" ) );

	auto* target = sceneNode->getRoot()->find( "target" )->asType<UIHTMLWidget>();
	ASSERT_TRUE( target != nullptr );

	EXPECT_TRUE( target->hasDataProperty( "data-role" ) );
	EXPECT_TRUE( target->hasDataProperty( "DATA-ROLE" ) );
	EXPECT_TRUE( target->hasDataProperty( "data-empty" ) );
	EXPECT_FALSE( target->hasDataProperty( "data-missing" ) );
	EXPECT_TRUE( target->getDataPropertyString( "data-role" ) == "hero" );
	EXPECT_TRUE( target->getDataPropertyString( "data-empty", "fallback" ) == "" );
	EXPECT_TRUE( target->getDataPropertyString( "data-missing", "fallback" ) == "fallback" );
	EXPECT_TRUE( target->getPropertyString( "data-role" ) == "hero" );

	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-role]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-role=\"hero\"]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-tags~=\"featured\"]" ).size(),
			   (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-lang|=\"en\"]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-id^=\"user-\"]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-id$=\"-42\"]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-id*=\"ser\"]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-empty]" ).size(), (size_t)1 );
	EXPECT_EQ( sceneNode->getRoot()->querySelectorAll( "[data-missing]" ).size(), (size_t)0 );

	EXPECT_TRUE( target->getDataPropertyString( "data-language" ) == "cpp" );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, closedByDefault ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d"><summary id="s">Label</summary><p id="p">Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* summary = sceneNode->getRoot()->find( "s" )->asType<UIHTMLSummary>();
	auto* content = sceneNode->getRoot()->find( "p" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( summary != nullptr );
	ASSERT_TRUE( content != nullptr );
	EXPECT_FALSE( details->isOpen() );
	EXPECT_TRUE( summary->isVisible() );
	EXPECT_FALSE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, summaryListStyleType ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d">
			<summary id="s1">Default</summary>
			<summary id="s2" style="list-style-type: disclosure-open;">Open marker</summary>
			<summary id="s3" style="list-style-type: decimal;">Decimal marker</summary>
		</details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	const auto* propDef = StyleSheetSpecification::instance()->getProperty( "list-style-type" );
	ASSERT_TRUE( propDef != nullptr );

	auto* s1 = sceneNode->getRoot()->find( "s1" )->asType<UIHTMLSummary>();
	auto* s2 = sceneNode->getRoot()->find( "s2" )->asType<UIHTMLSummary>();
	auto* s3 = sceneNode->getRoot()->find( "s3" )->asType<UIHTMLSummary>();
	ASSERT_TRUE( s1 != nullptr );
	ASSERT_TRUE( s2 != nullptr );
	ASSERT_TRUE( s3 != nullptr );

	EXPECT_TRUE( s1->getPropertyString( propDef ) == "disclosure-closed" );
	EXPECT_TRUE( s2->getPropertyString( propDef ) == "disclosure-open" );
	EXPECT_TRUE( s3->getPropertyString( propDef ) == "decimal" );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, summaryListStyleNoneClearsDefaultPadding ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
	<html>
		<body>
			<details>
				<summary id="default">Default marker</summary>
				<summary id="none" style="list-style-type: none;">No marker</summary>
				<summary id="explicit" style="list-style-type: none; padding-left: 7px;">
					Explicit padding
				</summary>
			</details>
		</body>
	</html>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* defaultSummary = sceneNode->getRoot()->find( "default" )->asType<UIHTMLSummary>();
	auto* noneSummary = sceneNode->getRoot()->find( "none" )->asType<UIHTMLSummary>();
	auto* explicitSummary = sceneNode->getRoot()->find( "explicit" )->asType<UIHTMLSummary>();
	ASSERT_TRUE( defaultSummary != nullptr );
	ASSERT_TRUE( noneSummary != nullptr );
	ASSERT_TRUE( explicitSummary != nullptr );

	EXPECT_GT( defaultSummary->getPixelsPadding().Left, 0.f );
	EXPECT_NEAR( noneSummary->getPixelsPadding().Left, 0.f, 0.5f );
	EXPECT_NEAR( explicitSummary->getPixelsPadding().Left, 7.f, 0.5f );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, inlineBlockSummaryListStyleNoneSize ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	std::string html;
	FileSystem::fileGet( "assets/html/lobsters_item.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->querySelector( ".caches" )->asType<UIHTMLDetails>();
	auto* summary =
		sceneNode->getRoot()->querySelector( ".caches summary" )->asType<UIHTMLSummary>();
	auto* author = sceneNode->getRoot()->querySelector( ".u-author" )->asType<UIWidget>();
	auto* time = sceneNode->getRoot()->querySelector( "time" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( summary != nullptr );
	ASSERT_TRUE( author != nullptr );
	ASSERT_TRUE( time != nullptr );

	EXPECT_EQ( details->getDisplay(), CSSDisplay::InlineBlock );
	EXPECT_EQ( summary->getListStyleType(), CSSListStyleType::None );
	EXPECT_NEAR( summary->getPixelsPadding().Left, 0.f, 0.5f );
	EXPECT_LE( details->getPixelsSize().getHeight(),
			   eemax( author->getPixelsSize().getHeight(), time->getPixelsSize().getHeight() ) +
				   1.f );
	EXPECT_NEAR( details->getPixelsPosition().y, author->getPixelsPosition().y, 2.f );
	EXPECT_NEAR( details->getPixelsPosition().y, time->getPixelsPosition().y, 2.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, openAttribute ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d" open><summary>Label</summary><p id="p">Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* content = sceneNode->getRoot()->find( "p" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( content != nullptr );
	EXPECT_TRUE( details->isOpen() );
	EXPECT_TRUE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, openAttributeExplicitFalse ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d" open="false"><summary>Label</summary><p id="p">Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* content = sceneNode->getRoot()->find( "p" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( content != nullptr );
	EXPECT_FALSE( details->isOpen() );
	EXPECT_FALSE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, toggleViaMouse ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d"><summary id="s">Label</summary><p id="p">Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* summary = sceneNode->getRoot()->find( "s" )->asType<UIHTMLSummary>();
	auto* content = sceneNode->getRoot()->find( "p" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( summary != nullptr );
	ASSERT_TRUE( content != nullptr );

	summary->onMouseClick( summary->getPixelsPosition().asInt(), EE_BUTTON_LMASK );
	EXPECT_TRUE( details->isOpen() );
	EXPECT_TRUE( content->isVisible() );
	summary->onMouseClick( summary->getPixelsPosition().asInt(), EE_BUTTON_LMASK );
	EXPECT_FALSE( details->isOpen() );
	EXPECT_FALSE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, toggleViaKeyboard ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d"><summary id="s">Label</summary><p id="p">Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* summary = sceneNode->getRoot()->find( "s" )->asType<UIHTMLSummary>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( summary != nullptr );

	KeyEvent enter( summary, Event::KeyDown, KEY_RETURN, SCANCODE_RETURN, 0, 0 );
	KeyEvent space( summary, Event::KeyDown, KEY_SPACE, SCANCODE_SPACE, 0, 0 );
	EXPECT_EQ( summary->onKeyDown( enter ), 1u );
	EXPECT_TRUE( details->isOpen() );
	EXPECT_EQ( summary->onKeyDown( space ), 1u );
	EXPECT_FALSE( details->isOpen() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, toggleEvent ) {
	init_ui_test();
	auto* details = UIHTMLDetails::New();
	details->setParent( SceneManager::instance()->getUISceneNode()->getRoot() );
	int toggleCount = 0;
	details->on( Event::OnToggle, [&toggleCount]( const Event* ) { toggleCount++; } );

	details->setOpen( true );
	details->setOpen( true );
	details->setOpen( false );
	EXPECT_EQ( toggleCount, 2 );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, autoSummary ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d"><p id="p">Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* content = sceneNode->getRoot()->find( "p" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( content != nullptr );
	auto* summary = details->findSummaryChild();
	ASSERT_TRUE( summary != nullptr );
	EXPECT_TRUE( summary->isVisible() );
	EXPECT_FALSE( content->isVisible() );
	EXPECT_TRUE( summary->toggleParentDetails() );
	EXPECT_TRUE( details->isOpen() );
	EXPECT_TRUE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, multipleSummaries ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d">
			<summary id="s1">One</summary>
			<summary id="s2">Two</summary>
			<p id="p">Content</p>
		</details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* s1 = sceneNode->getRoot()->find( "s1" )->asType<UIHTMLSummary>();
	auto* s2 = sceneNode->getRoot()->find( "s2" )->asType<UIHTMLSummary>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( s1 != nullptr );
	ASSERT_TRUE( s2 != nullptr );
	EXPECT_TRUE( s1->isVisible() );
	EXPECT_FALSE( s2->isVisible() );
	EXPECT_TRUE( s1->toggleParentDetails() );
	EXPECT_TRUE( details->isOpen() );
	EXPECT_TRUE( s2->isVisible() );
	EXPECT_FALSE( s2->toggleParentDetails() );
	EXPECT_TRUE( details->isOpen() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, nested ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="outer" open>
			<summary id="outer_s">Outer</summary>
			<details id="inner">
				<summary id="inner_s">Inner</summary>
				<p id="inner_p">Inner content</p>
			</details>
		</details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* outer = sceneNode->getRoot()->find( "outer" )->asType<UIHTMLDetails>();
	auto* inner = sceneNode->getRoot()->find( "inner" )->asType<UIHTMLDetails>();
	auto* innerSummary = sceneNode->getRoot()->find( "inner_s" )->asType<UIHTMLSummary>();
	ASSERT_TRUE( outer != nullptr );
	ASSERT_TRUE( inner != nullptr );
	ASSERT_TRUE( innerSummary != nullptr );
	EXPECT_TRUE( outer->isOpen() );
	EXPECT_FALSE( inner->isOpen() );
	EXPECT_TRUE( innerSummary->toggleParentDetails() );
	EXPECT_TRUE( outer->isOpen() );
	EXPECT_TRUE( inner->isOpen() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, hiddenChildPreserved ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<details id="d" open><summary>Label</summary><p id="p" hidden>Content</p></details>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* details = sceneNode->getRoot()->find( "d" )->asType<UIHTMLDetails>();
	auto* content = sceneNode->getRoot()->find( "p" )->asType<UIWidget>();
	ASSERT_TRUE( details != nullptr );
	ASSERT_TRUE( content != nullptr );
	EXPECT_FALSE( content->isVisible() );
	details->setOpen( false );
	details->setOpen( true );
	EXPECT_FALSE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, dynamicChildAddedWhileClosed ) {
	init_ui_test();
	auto* details = UIHTMLDetails::New();
	details->setParent( SceneManager::instance()->getUISceneNode()->getRoot() );
	auto* summary = UIHTMLSummary::New();
	summary->setParent( details );
	details->setOpen( false );
	auto* content = UIRichText::NewParagraph();
	content->setParent( details );
	details->updateLayout();

	EXPECT_TRUE( summary->isVisible() );
	EXPECT_FALSE( content->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIHTMLTextArea, rowsColsAttribute ) {
	init_ui_test();
	auto* scene = SceneManager::instance()->getUISceneNode();
	auto* c1_raw = scene->loadLayoutFromString( R"html(
		<vbox layout_width="wrap_content" layout_height="wrap_content">
			<textarea id="t1" rows="2" cols="20"></textarea>
			<textarea id="t2" rows="4" cols="40"></textarea>
		</vbox>
	)html" );
	ASSERT_TRUE( c1_raw != nullptr );
	auto* t1 = c1_raw->find( "t1" )->asType<UIHTMLTextArea>();
	auto* t2 = c1_raw->find( "t2" )->asType<UIHTMLTextArea>();
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

UTEST( UIHTML, FormControlsDefaultInlineBlock ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<div id="container" style="width: 600px;">
			<input id="i1" size="5">
			<input id="i2" size="5">
			<textarea id="t1" rows="2" cols="8"></textarea>
			<textarea id="t2" rows="2" cols="8"></textarea>
		</div>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* i1 = sceneNode->getRoot()->find( "i1" )->asType<UIHTMLInput>();
	auto* i2 = sceneNode->getRoot()->find( "i2" )->asType<UIHTMLInput>();
	auto* t1 = sceneNode->getRoot()->find( "t1" )->asType<UIHTMLTextArea>();
	auto* t2 = sceneNode->getRoot()->find( "t2" )->asType<UIHTMLTextArea>();

	ASSERT_TRUE( i1 != nullptr );
	ASSERT_TRUE( i2 != nullptr );
	ASSERT_TRUE( t1 != nullptr );
	ASSERT_TRUE( t2 != nullptr );

	EXPECT_EQ( i1->getDisplay(), CSSDisplay::InlineBlock );
	EXPECT_EQ( i2->getDisplay(), CSSDisplay::InlineBlock );

	EXPECT_EQ( i1->getPixelsPosition().y, i2->getPixelsPosition().y );
	EXPECT_LT( i1->getPixelsPosition().x, i2->getPixelsPosition().x );
	EXPECT_EQ( t1->getPixelsPosition().y, t2->getPixelsPosition().y );
	EXPECT_LT( t1->getPixelsPosition().x, t2->getPixelsPosition().x );

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

UTEST( UIHTMLBody, backgroundColorPropagation ) {
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
		R"(<html id="html_el">
			<body id="body_el" style="background-color: red; max-width: 960px;">
			</body>
		</html>)" );

	sceneNode->updateDirtyLayouts();

	auto html_el = sceneNode->getRoot()->find( "html_el" );
	auto body_el = sceneNode->getRoot()->find( "body_el" );

	ASSERT_TRUE( html_el != nullptr );
	ASSERT_TRUE( body_el != nullptr );

	// HTML element should have inherited the red background color, and body should be transparent
	EXPECT_TRUE( html_el->asType<UIWidget>()->getBackgroundColor() == Color::Red );
	EXPECT_TRUE( body_el->asType<UIWidget>()->getBackgroundColor() == Color::Transparent );

	Engine::destroySingleton();
}

UTEST( UIHTMLBody, maxWidthResizingBug ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "HTML Resize Bug",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );

	UI::CSS::StyleSheetParser parser;
	parser.loadFromFile( "assets/html/dwarmstrong/style.css" );
	sceneNode->setStyleSheet( parser.getStyleSheet() );

	std::string htmlContent;
	FileSystem::fileGet( "assets/html/dwarmstrong/dwarmstrong.html", htmlContent );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( htmlContent ) );

	sceneNode->getRoot()->setSize( 1024, 768 );
	sceneNode->updateDirtyLayouts();

	auto body_el = sceneNode->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( body_el != nullptr );
	Float widthAt1024 = body_el->getPixelsSize().getWidth();
	EXPECT_NEAR( widthAt1024, 960.f,
				 10.f ); // It should be around 960px (minus some margins if any)

	sceneNode->getRoot()->setSize( 2048, 768 );
	sceneNode->updateDirtyLayouts();
	Float widthAt2048 = body_el->getPixelsSize().getWidth();
	EXPECT_NEAR( widthAt2048, 960.f, 10.f ); // Body should stay 960px even when parent is huge

	sceneNode->getRoot()->setSize( 1024, 768 );
	sceneNode->updateDirtyLayouts();

	Float widthAfterResize = body_el->getPixelsSize().getWidth();
	EXPECT_NEAR( widthAt1024, widthAfterResize, 1.f );

	Engine::destroySingleton();
}

UTEST( UILayout, marginAuto ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Margin Auto Test",
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

	auto* container = sceneNode->loadLayoutFromString(
		R"(<vbox id="container">
			<widget id="child" style="margin: 0 auto;" />
		</vbox>)" );

	auto child = sceneNode->getRoot()->find( "child" );
	ASSERT_TRUE( child != nullptr );

	UIWidget* childWidget = child->asType<UIWidget>();
	UIWidget* contWidget = container->asType<UIWidget>();

	contWidget->setSize( 500, 500 );
	childWidget->setSize( 100, 100 );
	sceneNode->updateDirtyLayouts();

	Float expectedMarginX =
		( contWidget->getPixelsSize().getWidth() - childWidget->getPixelsSize().getWidth() ) / 2.f;

	// Margin left/right should be auto computed to expectedMarginX
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Left, expectedMarginX, 1.f );
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Right, expectedMarginX, 1.f );
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Top, 0.f, 1.f );
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Bottom, 0.f, 1.f );

	// Resize parent and see if margins re-evaluate automatically
	contWidget->setSize( 800, 800 );
	sceneNode->updateDirtyLayouts();

	expectedMarginX =
		( contWidget->getPixelsSize().getWidth() - childWidget->getPixelsSize().getWidth() ) / 2.f;

	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Left, expectedMarginX, 1.f );
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Right, expectedMarginX, 1.f );

	// Now test resize of child
	childWidget->setSize( 200, 100 );
	sceneNode->updateDirtyLayouts();

	expectedMarginX =
		( contWidget->getPixelsSize().getWidth() - childWidget->getPixelsSize().getWidth() ) / 2.f;

	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Left, expectedMarginX, 1.f );
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Right, expectedMarginX, 1.f );

	childWidget->setLayoutMarginAuto( false, false, true, true );
	sceneNode->updateDirtyLayouts();

	Float expectedMarginY =
		( contWidget->getPixelsSize().getHeight() - childWidget->getPixelsSize().getHeight() ) /
		2.f;

	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Top, expectedMarginY, 1.f );
	EXPECT_NEAR( childWidget->getLayoutPixelsMargin().Bottom, expectedMarginY, 1.f );
	EXPECT_FALSE( childWidget->hasLayoutMarginLeftAuto() );
	EXPECT_FALSE( childWidget->hasLayoutMarginRightAuto() );
	EXPECT_TRUE( childWidget->hasLayoutMarginTopAuto() );
	EXPECT_TRUE( childWidget->hasLayoutMarginBottomAuto() );

	Engine::destroySingleton();
}

UTEST( UILayout, listStyleTypeDecimal ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( R"html(
		<html>
			<ol>
				<li id="li1" style="list-style-type: decimal;">First item</li>
				<li id="li2" style="list-style-type: decimal;">Second item</li>
				<li id="li3" style="list-style-type: decimal;">Third item</li>
			</ol>
		</html>
	)html" );

	sceneNode->updateDirtyLayouts();

	const auto* propDef = StyleSheetSpecification::instance()->getProperty( "list-style-type" );
	ASSERT_TRUE( propDef != nullptr );

	auto* li1 = sceneNode->getRoot()->find( "li1" )->asType<UIRichText>();
	auto* li2 = sceneNode->getRoot()->find( "li2" )->asType<UIRichText>();
	auto* li3 = sceneNode->getRoot()->find( "li3" )->asType<UIRichText>();

	ASSERT_TRUE( li1 != nullptr );
	ASSERT_TRUE( li2 != nullptr );
	ASSERT_TRUE( li3 != nullptr );

	EXPECT_TRUE( li1->getPropertyString( propDef ) == "decimal" );
	EXPECT_TRUE( li2->getPropertyString( propDef ) == "decimal" );
	EXPECT_TRUE( li3->getPropertyString( propDef ) == "decimal" );

	Engine::destroySingleton();
}

UTEST( UILayout, listStyleTypeDisc ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( R"html(
		<html>
			<ul>
				<li id="li1" style="list-style-type: disc;">Bullet item</li>
			</ul>
		</html>
	)html" );

	sceneNode->updateDirtyLayouts();

	const auto* propDef = StyleSheetSpecification::instance()->getProperty( "list-style-type" );
	ASSERT_TRUE( propDef != nullptr );

	auto* li1 = sceneNode->getRoot()->find( "li1" )->asType<UIRichText>();
	ASSERT_TRUE( li1 != nullptr );

	EXPECT_TRUE( li1->getPropertyString( propDef ) == "disc" );

	Engine::destroySingleton();
}

UTEST( UILayout, listStyleTypeDisclosure ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( R"html(
		<html>
			<ul>
				<li id="li1" style="list-style-type: disclosure-closed;">Closed</li>
				<li id="li2" style="list-style-type: disclosure-open;">Open</li>
			</ul>
		</html>
	)html" );

	sceneNode->updateDirtyLayouts();

	const auto* propDef = StyleSheetSpecification::instance()->getProperty( "list-style-type" );
	ASSERT_TRUE( propDef != nullptr );

	auto* li1 = sceneNode->getRoot()->find( "li1" )->asType<UIRichText>();
	auto* li2 = sceneNode->getRoot()->find( "li2" )->asType<UIRichText>();
	ASSERT_TRUE( li1 != nullptr );
	ASSERT_TRUE( li2 != nullptr );

	EXPECT_TRUE( li1->getPropertyString( propDef ) == "disclosure-closed" );
	EXPECT_TRUE( li2->getPropertyString( propDef ) == "disclosure-open" );

	Engine::destroySingleton();
}

UTEST( UILayout, listStyleShorthand ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( R"html(
		<html>
			<ol>
				<li id="li1" style="list-style: decimal outside;">First</li>
				<li id="li2" style="list-style: lower-alpha inside;">Second</li>
				<li id="li3" style="list-style: none;">Third</li>
			</ol>
			<ul>
				<li id="li4" style="list-style: disc;">Bullet</li>
				<li id="li5" style="list-style: square outside;">Square</li>
				<li id="li6" style="list-style: circle;">Circle</li>
			</ul>
		</html>
	)html" );

	sceneNode->updateDirtyLayouts();

	const auto* typeDef = StyleSheetSpecification::instance()->getProperty( "list-style-type" );
	const auto* posDef = StyleSheetSpecification::instance()->getProperty( "list-style-position" );

	for ( const char* id : { "li1", "li2", "li3", "li4", "li5", "li6" } ) {
		auto* li = sceneNode->getRoot()->find( id )->asType<UIWidget>();
		ASSERT_TRUE( li != nullptr );
		EXPECT_TRUE( li->isType( UI_TYPE_HTML_LIST_ITEM ) );
	}

	EXPECT_TRUE( sceneNode->getRoot()->find( "li1" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "decimal" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "li1" )->asType<UIRichText>()->getPropertyString(
					 posDef ) == "outside" );

	EXPECT_TRUE( sceneNode->getRoot()->find( "li2" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "lower-alpha" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "li2" )->asType<UIRichText>()->getPropertyString(
					 posDef ) == "inside" );

	EXPECT_TRUE( sceneNode->getRoot()->find( "li3" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "none" );

	EXPECT_TRUE( sceneNode->getRoot()->find( "li4" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "disc" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "li5" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "square" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "li5" )->asType<UIRichText>()->getPropertyString(
					 posDef ) == "outside" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "li6" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "circle" );

	Engine::destroySingleton();
}

UTEST( UILayout, listStyleInheritanceFromUl ) {
	init_ui_test();
	auto* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->loadLayoutFromString( R"html(
		<html>
			<head>
				<style>
					ul.a { list-style-type: circle; }
					ul.b { list-style-type: disc; }
					ul.c { list-style-type: square; }
					ol.d { list-style-type: decimal; }
					ol.h { list-style-type: upper-roman; }
				</style>
			</head>
			<body>
				<ol class="h">
					<li id="h1">Coffee</li>
				</ol>
				<ul class="a">
					<li id="a1">Coffee</li>
				</ul>
				<ul class="b">
					<li id="b1">Coffee</li>
				</ul>
				<ul class="c">
					<li id="c1">Coffee</li>
				</ul>
				<ol class="d">
					<li id="d1">Coffee</li>
				</ol>
			</body>
		</html>
	)html" );

	sceneNode->updateDirtyLayouts();

	const auto* typeDef = StyleSheetSpecification::instance()->getProperty( "list-style-type" );

	EXPECT_TRUE( sceneNode->getRoot()->find( "h1" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "upper-roman" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "a1" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "circle" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "b1" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "disc" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "c1" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "square" );
	EXPECT_TRUE( sceneNode->getRoot()->find( "d1" )->asType<UIRichText>()->getPropertyString(
					 typeDef ) == "decimal" );

	Engine::destroySingleton();
}

UTEST( UIHTMLDetails, lobstersInlineBlockCachesWidth ) {
	Engine::instance()->createWindow( WindowSettings( 424, 184, "HTML Details Lobsters Test",
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
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<!doctype html>
		<html>
		<head></head>
		<body>
			<div id="inside">
				<ol class="stories list">
					<li id="story_4g74mw" class="story">
						<div class="story_liner h-entry">
							<div class="voters">
								<a class="upvoter" href="/login">109</a>
							</div>
							<div class="details">
								<span role="heading" aria-level="1" class="link h-cite u-repost-of">
									<a class="u-url" href="https://ratfactor.com/ascetic-computing">Ascetic Computing</a>
								</span>
								<span class="tags">
									<a class="tag tag_practices" href="/t/practices">practices</a>
								</span>
								<a class="domain" href="/domains/ratfactor.com">ratfactor.com</a>
								<div class="byline">
									<a tabindex="-1" aria-hidden="true" href="/~jbauer"><img class="avatar" alt="jbauer avatar" src="/avatars/jbauer-16.png" width="16" height="16"></a>
									<span> via </span>
									<a class="u-author h-card" href="/~jbauer">jbauer</a>
									<time>20 hours ago</time>
									<span aria-hidden="true"> | </span>
									<details class="caches" name="caches">
										<summary>caches</summary>
										<ul>
											<li><a href="https://web.archive.org/">Archive.org</a></li>
											<li><a href="https://ghostarchive.org/">Ghostarchive</a></li>
										</ul>
									</details>
									<span class="comments_label">
										<span aria-hidden="true"> | </span>
										<a role="heading" aria-level="2" href="/s/4g74mw/ascetic_computing">17 comments</a>
									</span>
								</div>
							</div>
						</div>
					</li>
				</ol>
			</div>
		</body>
		</html>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	sceneNode->combineStyleSheet( R"css(
		body, textarea, input, button {
			font-family: "helvetica neue", arial, sans-serif;
			line-height: 1.45em;
		}
		ol.stories {
			padding: 0;
			list-style: none;
			margin: 0;
		}
		div.voters {
			float: left;
			text-align: center;
			width: 40px;
		}
		li.story {
			clear: both;
		}
		ol.stories li.story div.story_liner {
			padding-top: 0.25em;
			padding-bottom: 0.25em;
			word-break: break-word;
		}
		li div.details {
			padding-top: 0.1em;
			margin-left: 32px;
		}
		li .link {
			font-weight: bold;
			vertical-align: middle;
		}
		li .link a {
			text-decoration: none;
		}
		li.story a.tag {
			vertical-align: middle;
		}
		li .tags {
			margin-right: 0.25em;
		}
		li .domain {
			font-style: italic;
			text-decoration: none;
			vertical-align: middle;
		}
		img.avatar {
			border-radius: 8px;
			height: 16px;
			margin-bottom: 2px;
			margin-right: 2px;
			vertical-align: middle;
			width: 16px;
		}
		li.story .byline {
			margin-top: 1px;
		}
		.caches {
			display: inline-block;
			position: relative;
		}
		.caches summary {
			list-style: none;
		}
		.caches ul {
			position: absolute;
			white-space: nowrap;
			list-style: none;
			padding: 0;
			z-index: 1;
		}
		.caches a {
			text-decoration: none;
			display: block;
			padding: 3px 7px;
		}
		@media only screen and (max-width: 480px) {
			div#inside {
				margin: 0.5rem;
			}
			ol.stories {
				margin: 0 0 0 -0.5rem;
				padding-left: 0;
			}
			div.voters {
				margin-left: 0.25em;
				margin-top: 0px;
				width: 30px;
			}
			ol.stories.list {
				margin-top: 0;
			}
			ol.stories.list li.story {
				display: table;
			}
			ol.stories.list li.story div.story_liner {
				display: table-cell;
				padding-top: 0.5em;
				padding-bottom: 0.75em;
				width: 100%;
			}
			li div.details {
				margin: 0 0 0 36px;
			}
		}
	)css" );
	sceneNode->updateDirtyLayouts();

	auto* caches = sceneNode->getRoot()->querySelector( ".caches" )->asType<UIHTMLDetails>();
	auto* comments = sceneNode->getRoot()->querySelector( ".comments_label" );
	auto* byline = sceneNode->getRoot()->querySelector( ".byline" );

	EXPECT_LT( caches->getPixelsSize().getWidth(), byline->getPixelsSize().getWidth() * 0.5f );
	EXPECT_NEAR( caches->getPixelsPosition().y, comments->getPixelsPosition().y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIBorder, renderingVariations ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "Border Rendering Test", VisualTestWindowStyle,
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
	FileSystem::fileGet( "assets/html/border_tests.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-border-rendering", "html", 12 );

	Engine::destroySingleton();
}

UTEST( UIBorder, renderingVariations2 ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "Border Rendering Test 2", VisualTestWindowStyle,
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
	FileSystem::fileGet( "assets/html/border_tests_2.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-border-rendering-2", "html", 12 );

	Engine::destroySingleton();
}

static UISceneNode* init_test_inline_block() {
	FontTrueType* font = nullptr;
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );
	FontTrueType* monoFont = FontTrueType::New( "monospace" );
	monoFont->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
	return sceneNode;
}

UTEST( UIHTML, BodyViewportMinimumHeightUsesSceneViewport ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Body Viewport Height Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setPixelsSize( 800, 3000 );
	sceneNode->setViewportPixelsSize( Sizef( 800, 600 ) );

	const std::string html = R"html(
<!DOCTYPE html>
<html>
<head><style>body { margin: 0; min-height: 100vh; }</style></head>
<body><div style="height: 10px;"></div></body>
</html>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto bodyNode = sceneNode->getRoot()->findByType( UI_TYPE_HTML_BODY );
	ASSERT_TRUE( bodyNode != nullptr );
	auto body = bodyNode->asType<UIHTMLBody>();
	body->applyProperty( StyleSheetProperty( "min-height", "100vh" ) );
	body->updateLayout();
	EXPECT_NEAR( bodyNode->getPixelsSize().getHeight(), 600.f, 1.f );
	EXPECT_EQ( sceneNode->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getPixelsSize().getHeight(), 3000 );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBlock ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Inline Block Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	const std::string html = R"html(
<!DOCTYPE html>
<html>
<head>
<style>
ul > li {
	display: inline-block;
	border: 1px solid red;
}
</style>
</head>
<body>
	<ul class="flat-list buttons">
		<li><a href="#">6 comments</a></li>
		<li><a class="post-sharing-button" href="#">share</a></li>
		<li><a href="#">save</a></li>
		<li><span><a href="#">hide</a></span></li>
		<li><a href="#">report</a></li>
		<li><a href="#">crosspost</a></li>
	</ul>
</body>
</html>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	auto ul = sceneNode->getRoot()->findByTag( "ul" );
	ASSERT_TRUE( ul != nullptr );

	// Force layout update
	sceneNode->update( Seconds( 1 ) );

	auto lis = ul->findAllByTag( "li" );
	EXPECT_EQ( lis.size(), (size_t)6 );

	for ( auto li : lis ) {
		EXPECT_EQ( li->asType<UIHTMLWidget>()->getDisplay(), CSSDisplay::InlineBlock );
		EXPECT_EQ( li->getLayoutWidthPolicy(), SizePolicy::WrapContent );
		EXPECT_GT( li->getPixelsSize().getWidth(), 0 );
		EXPECT_LT( li->getPixelsSize().getWidth(), ul->getPixelsSize().getWidth() );
		EXPECT_GT( li->getPixelsSize().getHeight(), 0 );
	}

	// Check if they are on the same line (inline-block)
	if ( lis.size() >= 2 ) {
		EXPECT_EQ( lis[0]->getPixelsPosition().y, lis[1]->getPixelsPosition().y );
		EXPECT_LT( lis[0]->getPixelsPosition().x, lis[1]->getPixelsPosition().x );
	}

	Engine::destroySingleton();
}

UTEST( UIHTML, BlockList ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Block List Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	const std::string html = R"html(
<ul id="block-list">
	<li style="height: 20px">Item 1</li>
	<li style="height: 20px">Item 2</li>
</ul>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	auto ul = sceneNode->getRoot()->findByTag( "ul" );
	ASSERT_TRUE( ul != nullptr );
	EXPECT_GT( ul->getPixelsSize().getWidth(), 0 );

	auto lis = ul->findAllByTag( "li" );
	EXPECT_EQ( lis.size(), (size_t)2 );

	for ( auto li : lis ) {
		EXPECT_EQ( li->asType<UIHTMLWidget>()->getDisplay(), CSSDisplay::ListItem );
		EXPECT_GT( li->getChildCount(), (size_t)0 );
		EXPECT_TRUE( li->asType<UIRichText>()->getRichTextPtr()->getFontStyleConfig().Font !=
					 nullptr );
		EXPECT_GT( li->asType<UIRichText>()->getRichTextPtr()->getFontStyleConfig().CharacterSize,
				   0 );
		EXPECT_GT( li->asType<UIRichText>()->getRichTextPtr()->getSize().getWidth(), 0 );
		EXPECT_GT( li->asType<UIRichText>()->getRichTextPtr()->getSize().getHeight(), 0 );
		EXPECT_GT( li->getPixelsSize().getWidth(), 0 );
		EXPECT_GT( li->getPixelsSize().getHeight(), 0 );
	}

	// They should be one above the other (block)
	EXPECT_LT( lis[0]->getPixelsPosition().y, lis[1]->getPixelsPosition().y );
	EXPECT_EQ( lis[0]->getPixelsPosition().x, lis[1]->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineList ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Inline List Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	const std::string html = R"html(
<ul style="display: block">
	<li style="display: inline">Item 1</li>
	<li style="display: inline">Item 2</li>
</ul>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	auto ul = sceneNode->getRoot()->findByTag( "ul" );
	ASSERT_TRUE( ul != nullptr );
	EXPECT_GT( ul->getPixelsSize().getWidth(), 0 );

	auto lis = ul->findAllByTag( "li" );
	EXPECT_EQ( lis.size(), (size_t)2 );

	for ( auto li : lis ) {
		EXPECT_EQ( li->asType<UIHTMLWidget>()->getDisplay(), CSSDisplay::Inline );
		EXPECT_EQ( li->getLayoutWidthPolicy(), SizePolicy::WrapContent );
		EXPECT_GT( li->getPixelsSize().getWidth(), 0 );
		EXPECT_LT( li->getPixelsSize().getWidth(), ul->getPixelsSize().getWidth() );
	}

	// They should be on the same line (inline)
	EXPECT_EQ( lis[0]->getPixelsPosition().y, lis[1]->getPixelsPosition().y );
	EXPECT_LT( lis[0]->getPixelsPosition().x, lis[1]->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBlockExplicitWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Inline Block Explicit Width Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	const std::string html = R"html(
<div style="width: 200px">
	<div id="d1" style="display: inline-block; width: 150px; height: 50px"></div>
	<div id="d2" style="display: inline-block; width: 150px; height: 50px"></div>
</div>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	auto d1 = sceneNode->getRoot()->find( "d1" )->asType<UIWidget>();
	auto d2 = sceneNode->getRoot()->find( "d2" )->asType<UIWidget>();
	ASSERT_TRUE( d1 != nullptr && d2 != nullptr );

	// They should NOT be on the same line because 150 + 150 > 200
	EXPECT_LT( d1->getPixelsPosition().y, d2->getPixelsPosition().y );
	EXPECT_EQ( d1->getPixelsPosition().x, d2->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBlockMixedContent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Inline Block Mixed Content Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	const std::string html = R"html(
<div>
	Some text
	<div id="ib" style="display: inline-block; width: 50px; height: 50px; background-color: red"></div>
	more text
</div>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	auto ib = sceneNode->getRoot()->find( "ib" )->asType<UIWidget>();
	ASSERT_TRUE( ib != nullptr );

	// The inline-block should have a non-zero position and be somewhat centered vertically if it
	// follows text flow
	EXPECT_GT( ib->getPixelsPosition().x, 0 );
	EXPECT_GT( ib->getPixelsSize().getWidth(), 0 );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBlockWrapIssue ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Inline Block Wrap Issue Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	std::string html;
	FileSystem::fileGet( "assets/html/inline_block_wrap.html", html );

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	auto h2 = sceneNode->getRoot()->find( "h2-wrap" );
	ASSERT_TRUE( h2 != nullptr );

	auto rt = h2->asType<UIRichText>()->getRichTextPtr();

	EXPECT_EQ( (size_t)1, rt->getLines().size() );

	Engine::destroySingleton();
}

UTEST( UIHTML, InlineBlockBrowserTest ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Inline Block Browser Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	std::string html;
	FileSystem::fileGet( "assets/html/is_inline_block.html", html );

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	auto ib = sceneNode->getRoot()->find( "ib" )->asType<UIWidget>();
	auto t1 = sceneNode->getRoot()->find( "t1" )->asType<UIWidget>();
	auto t2 = sceneNode->getRoot()->find( "t2" )->asType<UIWidget>();

	ASSERT_TRUE( ib != nullptr && t1 != nullptr && t2 != nullptr );

	// If it drops to the next line:
	EXPECT_GT( ib->getPixelsPosition().y, t1->getPixelsPosition().y );
	// And t2 should be AFTER ib (either horizontally or vertically)
	EXPECT_GE( t2->getPixelsPosition().y,
			   ib->getPixelsPosition().y + ib->getPixelsSize().getHeight() );
	if ( t2->getPixelsPosition().y == ib->getPixelsPosition().y ) {
		EXPECT_GE( t2->getPixelsPosition().x,
				   ib->getPixelsPosition().x + ib->getPixelsSize().getWidth() );
	}
	EXPECT_EQ( ib->getPixelsPosition().x, t2->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( UIHTML, HeightExpansion ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Height Expansion Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/ensoft/" );

	std::string html;
	FileSystem::fileGet( "assets/html/ensoft/ensoft.html", html );

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );

	// Wait a bit and update again to make sure layouts are computed
	sceneNode->updateDirtyLayouts();

	auto htmlNode = sceneNode->getRoot()->findByType( UI_TYPE_HTML_HTML );
	auto bodyNode = sceneNode->getRoot()->findByType( UI_TYPE_HTML_BODY );

	ASSERT_TRUE( htmlNode != nullptr );
	ASSERT_TRUE( bodyNode != nullptr );

	auto htmlWidget = htmlNode->asType<UIWidget>();
	auto bodyWidget = bodyNode->asType<UIWidget>();

	EXPECT_GT( htmlWidget->getSize().getHeight(), 0 );
	EXPECT_GT( bodyWidget->getSize().getHeight(), 0 );

	EXPECT_GE( htmlWidget->getSize().getHeight(), bodyWidget->getSize().getHeight() );

	auto barNode = sceneNode->getRoot()->find( "bar" );
	ASSERT_TRUE( barNode != nullptr );

	auto barWidget = barNode->asType<UIWidget>();
	auto barHTML = barNode->asType<UIHTMLWidget>();

	EXPECT_EQ( barHTML->getCSSPosition(), CSSPosition::Fixed );

	EXPECT_NEAR( barWidget->getPixelsSize().getWidth(), 250.f, 1.f );

	auto rootWidget = sceneNode->getRoot();
	EXPECT_GT( rootWidget->getPixelsSize().getHeight(), 0 );
	EXPECT_NEAR( barWidget->getPixelsSize().getHeight(), rootWidget->getPixelsSize().getHeight(),
				 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, HeightExpansion_FixedDoesNotExpand ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Height Expansion Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	const std::string html = R"html(
<!DOCTYPE html>
<html>
<body style="margin: 0; padding: 0;">
    <div style="height: 100px;"></div>
    <div style="position: fixed; top: 500px; height: 50px;"></div>
</body>
</html>
)html";

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto bodyNode = sceneNode->getRoot()->findByType( UI_TYPE_HTML_BODY );
	ASSERT_TRUE( bodyNode != nullptr );

	auto bodyWidget = bodyNode->asType<UIWidget>();

	// The height should be 100px, not 550px because the fixed div should be ignored.
	EXPECT_NEAR( bodyWidget->getPixelsSize().getHeight(), 100.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, BodyHeightMiscalculationFixture ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Body Height Miscalculation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/body_height_miscalculation.html", html );

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto bodyNode = sceneNode->getRoot()->findByType( UI_TYPE_HTML_BODY );
	ASSERT_TRUE( bodyNode != nullptr );

	auto bodyWidget = bodyNode->asType<UIWidget>();

	EXPECT_GT( bodyWidget->getPixelsSize().getHeight(), 3000.f );
	EXPECT_LT( bodyWidget->getPixelsSize().getHeight(), 6000.f );

	// Verify the nav layout: .wrap has justify-content: space-between,
	// so .brand should be at the left and .links at the right.
	auto nav = sceneNode->getRoot()->findByClass( "site-nav" );
	ASSERT_TRUE( nav != nullptr );
	auto navWidget = nav->asType<UIWidget>();

	auto wrap = navWidget->findByClass( "wrap" );
	ASSERT_TRUE( wrap != nullptr );
	auto wrapWidget = wrap->asType<UIWidget>();

	auto brand = wrapWidget->findByClass( "brand" );
	ASSERT_TRUE( brand != nullptr );
	auto brandWidget = brand->asType<UIWidget>();

	auto links = wrapWidget->findByClass( "links" );
	ASSERT_TRUE( links != nullptr );
	auto linksWidget = links->asType<UIWidget>();

	// With justify-content: space-between in a row-direction flex:
	// .brand should be at the left side of .wrap (near padding edge).
	EXPECT_LT( brandWidget->getPixelsPosition().x, wrapWidget->getPixelsSize().getWidth() * 0.25f );

	// .links should be at the right side of .wrap, NOT next to .brand.
	Float wrapW = wrapWidget->getPixelsSize().getWidth();
	Float linksW = linksWidget->getPixelsSize().getWidth();
	EXPECT_GT( linksWidget->getPixelsPosition().x, wrapW * 0.5f );

	// .links should NOT occupy all available width — it must be content-sized.
	EXPECT_GT( linksW, 0.f );
	EXPECT_LT( linksW, wrapW * 0.5f );

	Engine::destroySingleton();
}

UTEST( UIHTML, ContactFormLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Contact Form Layout Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/ensoft/" );

	std::string html;
	FileSystem::fileGet( "assets/html/ensoft/contact.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto form = sceneNode->getRoot()->find( "form-contact" );
	ASSERT_TRUE( form != nullptr );
	auto formWidget = form->asType<UIWidget>();
	EXPECT_GT( formWidget->getPixelsSize().getHeight(), 0 );

	auto ul = formWidget->findByTag( "ul" );
	ASSERT_TRUE( ul != nullptr );
	auto ulWidget = ul->asType<UIWidget>();
	Float ulHeight = ulWidget->getPixelsSize().getHeight();
	EXPECT_GT( ulHeight, 0 );

	auto lis = ulWidget->findAllByTag( "li" );
	EXPECT_EQ( lis.size(), (size_t)7 );

	Float totalLiHeight = 0;
	int visibleLiCount = 0;
	for ( auto li : lis ) {
		Float liH = li->getPixelsSize().getHeight();
		if ( liH > 0 )
			visibleLiCount++;
		totalLiHeight += liH;
	}
	EXPECT_EQ( visibleLiCount, 6 );
	EXPECT_GT( totalLiHeight, 0 );
	EXPECT_NEAR( ulHeight, totalLiHeight, 1.f );

	auto contactBox = sceneNode->getRoot()->find( "contact-box" );
	ASSERT_TRUE( contactBox != nullptr );
	auto cbWidget = contactBox->asType<UIWidget>();
	EXPECT_GT( cbWidget->getPixelsSize().getHeight(), 10 );

	auto content = sceneNode->getRoot()->find( "content" );
	ASSERT_TRUE( content != nullptr );
	auto contentWidget = content->asType<UIWidget>();
	EXPECT_GT( contentWidget->getPixelsSize().getHeight(), 60 );

	auto bodyNode = sceneNode->getRoot()->findByType( UI_TYPE_HTML_BODY );
	ASSERT_TRUE( bodyNode != nullptr );
	auto bodyWidget = bodyNode->asType<UIWidget>();
	EXPECT_GT( bodyWidget->getPixelsSize().getHeight(), 0 );

	Engine::destroySingleton();
}

UTEST( UIBackground, imageAtlasPositioning ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "Background Atlas Test", VisualTestWindowStyle,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/background_atlas.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	// Verify the atlas image was actually loaded — scan all nodes for a loaded drawable
	bool foundLoadedImage = false;
	sceneNode->getRoot()->forEachNode( [&foundLoadedImage]( Node* node ) {
		if ( foundLoadedImage || !node->isWidget() )
			return;
		auto* bg = node->asType<UIWidget>()->getBackground();
		if ( !bg )
			return;
		auto* layer = bg->getLayer( 0 );
		if ( layer && layer->getDrawable() ) {
			Sizef sz = layer->getDrawable()->getPixelsSize();
			if ( sz.getWidth() == 1024.f && sz.getHeight() == 512.f )
				foundLoadedImage = true;
		}
	} );
	ASSERT_TRUE( foundLoadedImage );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-background-atlas", "html", 4 );

	Engine::destroySingleton();
}

UTEST( UIBackground, inlineSpanColorRendersBehindBackgroundImage ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 320, 160, "Inline Background Color Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/ensoft/" );

	std::string html;
	FileSystem::fileGet( "assets/html/ensoft/background.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	auto anchors = sceneNode->getRoot()->querySelectorAll( "#rss a" );
	ASSERT_EQ( anchors.size(), (size_t)1 );
	auto* anchor = anchors.front()->asType<UITextSpan>();
	ASSERT_TRUE( anchor != nullptr );
	EXPECT_EQ( anchor->getFontBackgroundColor().getValue(),
			   Color::fromString( "#d0d0d0" ).getValue() );
	ASSERT_TRUE( anchor->getBackground() != nullptr );
	ASSERT_TRUE( anchor->getBackground()->hasDrawableLayers() );
	ASSERT_TRUE( anchor->getBackground()->getBackgroundDrawable().hasRadius() );

	bool foundRoundedLayeredFragment = false;
	sceneNode->getRoot()->forEachNode( [&]( Node* node ) {
		if ( foundRoundedLayeredFragment || !node->isType( UI_TYPE_RICHTEXT ) )
			return;
		for ( const auto& fragment :
			  node->asType<UIRichText>()->getRichText().getInlineFragments() ) {
			if ( fragment.type == RichText::InlineFragment::Type::Box &&
				 fragment.source.type == RichText::InlineSourceType::Widget &&
				 fragment.source.ptr == anchor ) {
				foundRoundedLayeredFragment = true;
				EXPECT_TRUE( fragment.backgroundColorDrawable != nullptr );
				EXPECT_TRUE( fragment.backgroundDrawable != nullptr );
				EXPECT_EQ( fragment.backgroundColor.getValue(),
						   Color::fromString( "#d0d0d0" ).getValue() );
				break;
			}
		}
	} );
	EXPECT_TRUE( foundRoundedLayeredFragment );

	const Rectf rect = anchor->getScreenRect();
	ASSERT_GT( rect.getWidth(), 12.f );
	ASSERT_GT( rect.getHeight(), 4.f );

	Image image = win->getFrontBufferImage();
	const Uint32 sampleX = static_cast<Uint32>( eefloor( rect.Right - 5.f ) );
	const Uint32 sampleY = static_cast<Uint32>( eefloor( rect.Top + rect.getHeight() * 0.5f ) );
	ASSERT_LT( sampleX, image.getWidth() );
	ASSERT_LT( sampleY, image.getHeight() );

	const Color pixel = image.getPixel( sampleX, sampleY );
	EXPECT_NEAR( pixel.r, 0xd0, 4 );
	EXPECT_NEAR( pixel.g, 0xd0, 4 );
	EXPECT_NEAR( pixel.b, 0xd0, 4 );

	Engine::destroySingleton();
}

UTEST( UIBackground, imageAtlasPositioningPixelDensity2 ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "Background Atlas Test PD2", VisualTestWindowStyle,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	EE::Graphics::PixelDensity::setPixelDensity( 2.0f );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/background_atlas.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	compareImages( utest_state, utest_result, win, "eepp-ui-background-atlas-pd2", "html", 4 );

	Engine::destroySingleton();
	EE::Graphics::PixelDensity::setPixelDensity( 1.0f );
}

UTEST( UIBackground, cssFileRelativeSpriteUrlAndNegativePosition ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="sprite" class="sprite"></div>
		</body>
	)html" ) );
	sceneNode->combineStyleSheet(
		R"css(
			.sprite {
				width: 15px;
				height: 14px;
				background-image: url(sprite-reddit.13AvZYXRW_4.png);
				background-position: -42px -1678px;
				background-repeat: no-repeat;
			}
		)css",
		true, String::hash( "reddit-sprite-test" ),
		URI( "file://" + Sys::getProcessPath() +
			 "assets/html/reddit_old_thread_files/reddit-sprite-test.css" ) );
	sceneNode->updateDirtyLayouts();

	auto* sprite = sceneNode->find<UIWidget>( "sprite" );
	ASSERT_TRUE( sprite != nullptr );
	auto* background = sprite->getBackground();
	ASSERT_TRUE( background != nullptr );
	auto* layer = background->getLayer( 0 );
	ASSERT_TRUE( layer != nullptr );
	ASSERT_TRUE( layer->getDrawable() != nullptr );

	EXPECT_EQ( layer->getDrawable()->getPixelsSize().getWidth(), 140 );
	EXPECT_EQ( layer->getDrawable()->getPixelsSize().getHeight(), 1751 );
	EXPECT_STDSTREQ( layer->getPositionX(), "-42px" );
	EXPECT_STDSTREQ( layer->getPositionY(), "-1678px" );
	EXPECT_NEAR( layer->getOffset().x, -42.f, 0.1f );
	EXPECT_NEAR( layer->getOffset().y, -1678.f, 0.1f );
	EXPECT_EQ( layer->getRepeatX(), UINodeDrawable::RepeatX::NoRepeat );
	EXPECT_EQ( layer->getRepeatY(), UINodeDrawable::RepeatY::NoRepeat );

	Engine::destroySingleton();
}

UTEST( UIBackground, InlineBlockImageSpans ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "inline-block image spans", VisualTestWindowStyle,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/inline_block.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );
	auto spans = sceneNode->getRoot()->querySelectorAll( "a > span" );

	EXPECT_GT( anchors.size(), (size_t)0 );
	EXPECT_GT( spans.size(), (size_t)0 );

	for ( auto anchor : anchors ) {
		EXPECT_GT( anchor->getPixelsSize().getWidth(), 0 );
		EXPECT_GT( anchor->getPixelsSize().getHeight(), 0 );
	}

	for ( auto span : spans ) {
		EXPECT_GT( span->getPixelsSize().getWidth(), 0 );
		EXPECT_GT( span->getPixelsSize().getHeight(), 0 );
	}

	compareImages( utest_state, utest_result, win, "eepp-ui-inline-block-image-spans", "html", 4 );

	Engine::destroySingleton();
}

UTEST( UIBackground, InlineBlockImageFixedSize ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "inline-block image fixed size", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/reddit_header_icons.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );

	EXPECT_GT( anchors.size(), (size_t)0 );

	for ( auto anchor : anchors ) {
		EXPECT_EQ( anchor->getPixelsSize().getWidth(), 15 );
		EXPECT_EQ( anchor->getPixelsSize().getHeight(), 12 );
	}

	Engine::destroySingleton();
}

UTEST( UIHTML, RedditHeaderPagenameBottomAlign ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "reddit header pagename bottom align", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/reddit_header.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->getInput()->update();
	SceneManager::instance()->update();

	auto* headerLeft = sceneNode->getRoot()->find( "header-bottom-left" )->asType<UIHTMLWidget>();
	auto* logo = sceneNode->getRoot()->find( "header-img" )->asType<UIHTMLWidget>();
	auto* page = sceneNode->getRoot()->querySelector( ".pagename" )->asType<UIHTMLWidget>();
	auto* jumpToContent = sceneNode->getRoot()->find( "jumpToContent" )->asType<UIHTMLWidget>();
	ASSERT_TRUE( headerLeft != nullptr );
	ASSERT_TRUE( logo != nullptr );
	ASSERT_TRUE( page != nullptr );
	ASSERT_TRUE( jumpToContent != nullptr );

	EXPECT_EQ( logo->getBaselineAlign().type, CSSBaselineAlignment::Bottom );
	EXPECT_EQ( page->getBaselineAlign().type, CSSBaselineAlignment::Bottom );
	EXPECT_NEAR( headerLeft->getPixelsSize().getHeight(),
				 page->getPixelsPosition().y + page->getPixelsSize().getHeight(), 0.5f );
	EXPECT_EQ( CSSPosition::Absolute, jumpToContent->getCSSPosition() );
	EXPECT_NEAR( -865.f, jumpToContent->convertToWorldSpace( { 0, 0 } ).x, 1.f );
	EXPECT_NEAR( 25.f, jumpToContent->convertToWorldSpace( { 0, 0 } ).y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, AnchorsSizing ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "anchors sizing", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/lobsters_simple.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );

	EXPECT_GT( anchors.size(), (size_t)0 );

	for ( auto anchor : anchors ) {
		auto a = anchor->asType<UIAnchorSpan>();
		if ( a->getDisplay() == CSSDisplay::None )
			continue;
		EXPECT_GT( a->getPixelsSize().getHeight(), 0 );
		if ( !a->getText().empty() && a->getFontStyleConfig().Font ) {
			String text = UIRichText::collapseInternalWhitespace( a->getText() );
			text.trim();
			if ( !text.empty() )
				EXPECT_GE( a->getPixelsSize().getWidth(),
						   Text::getTextWidth( text, a->getFontStyleConfig() ) );
		}
	}

	Engine::destroySingleton();
}

static UISceneNode* createWinAndLoadHTML( std::string winName, std::string htmlPath ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, winName, WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
						false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	if ( font == nullptr || !font->loaded() )
		return nullptr;
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string html;
	FileSystem::fileGet( htmlPath, html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	return sceneNode;
}

UTEST( UIHTML, blockFlow ) {
	auto sceneNode = createWinAndLoadHTML( "HTML Block Flow", "assets/html/block_flow.html" );
	ASSERT_TRUE( sceneNode != nullptr );
	auto sections = sceneNode->getRoot()->findAllByClass( "language-section" );

	ASSERT_EQ( sections.size(), (size_t)6 );

	// Each section is display block so we expect a single section per line
	// if sections position are not equal it means that some sections are floating
	Float ref = sections[0]->getPixelsPosition().x;
	for ( auto section : sections )
		EXPECT_EQ( section->getPixelsPosition().x, ref );

	Engine::destroySingleton();
}

UTEST( UIHTML, blockFlowFloat ) {
	auto sceneNode =
		createWinAndLoadHTML( "HTML Block Flow", "assets/html/block_flow_float_left.html" );
	ASSERT_TRUE( sceneNode != nullptr );
	auto sections = sceneNode->getRoot()->findAllByClass( "language-section" );

	ASSERT_EQ( sections.size(), (size_t)6 );

	// Each section is display block with float: left and width 48% so we expect two sections
	// per line, and each odd index should be to the right
	Float refLeft = sections[0]->getPixelsPosition().x;
	Float refRight = sections[1]->getPixelsPosition().x;
	for ( size_t idx = 0; idx < sections.size(); idx++ ) {
		Float expected = idx % 2 == 0 ? refLeft : refRight;
		EXPECT_EQ( sections[idx]->getPixelsPosition().x, expected );
	}
	Engine::destroySingleton();
}

UTEST( FontTrueType, glyphScaleZeroDimensionsNoCrash ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Glyph Scale Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	for ( unsigned int size : { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u } ) {
		for ( Uint32 ch : { 'A', 'Z', 'a', 'z', '0', '9', '!', '@', '#', '$' } ) {
			const Glyph& glyph = font->getGlyph( ch, size, false, false, 0.f );
			(void)glyph;
		}
	}

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	sceneNode->loadLayoutFromString( R"html(
		<vbox layout_width="match_parent" layout_height="match_parent">
			<TextView id="tiny" text="Tiny text" font_size="1" />
			<TextView id="small" text="Small text" font_size="3" />
			<TextView id="normal" text="Normal text" font_size="14" />
		</vbox>
	)html" );

	sceneNode->updateDirtyLayouts();

	auto tiny = sceneNode->getRoot()->find( "tiny" );
	auto small = sceneNode->getRoot()->find( "small" );
	auto normal = sceneNode->getRoot()->find( "normal" );

	ASSERT_TRUE( tiny != nullptr );
	ASSERT_TRUE( small != nullptr );
	ASSERT_TRUE( normal != nullptr );

	EXPECT_GT( normal->getPixelsSize().getWidth(), 0 );
	EXPECT_GT( normal->getPixelsSize().getHeight(), 0 );

	Engine::destroySingleton();
}

UTEST( UIHTML, LiFloatLeft ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 653, "li float left", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UI::UISceneNode* sceneNode = init_test_inline_block();

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/float_li.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	win->setClearColor( Color::White );

	win->getInput()->update();
	SceneManager::instance()->update();

	win->clear();
	SceneManager::instance()->draw();
	win->display();

	auto livec = sceneNode->getRoot()->findAllByTag( "li" );

	ASSERT_GT( livec.size(), (size_t)0 );

	auto refY = livec[0]->getPixelsPosition().y;

	for ( size_t i = 1; i < livec.size(); i++ )
		EXPECT_NEAR( refY, livec[i]->getPixelsPosition().y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexCenterWebViewLikeLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1280, 720, "Flex Center WebView",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/flex_center.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto postWrapper = sceneNode->getRoot()->findByClass( "post-wrapper" );
	auto post = sceneNode->getRoot()->findByClass( "post" );
	ASSERT_TRUE( postWrapper != nullptr );
	ASSERT_TRUE( post != nullptr );

	auto postWidget = post->asType<UIWidget>();

	// In a real browser a div with width:100% inside a column flex container
	// with align-items:center still spans the full cross axis because 100%
	// resolves against the container.  The item should therefore sit flush with
	// the container's left edge (plus any container padding).
	EXPECT_NEAR( postWidget->getPixelsPosition().x, 0.f, 2.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexCenterNoTextNodeDisplacement ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Flex Center Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/flex_center.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto postWrapper = sceneNode->getRoot()->findByClass( "post-wrapper" );
	ASSERT_TRUE( postWrapper != nullptr );
	auto post = postWrapper->findByClass( "post" );
	ASSERT_TRUE( post != nullptr );

	// The post-wrapper is a column flex container.  Whitespace text nodes
	// between the tags must not be treated as flex items, otherwise the
	// .post child would be displaced downward.
	auto postWrapperWidget = postWrapper->asType<UIWidget>();
	auto postWidget = post->asType<UIWidget>();

	Float postX = postWidget->getPixelsPosition().x;
	Float postY = postWidget->getPixelsPosition().y;
	Float wrapperTopPadding = postWrapperWidget->getPixelsPadding().Top;
	Float postMarginTop = postWidget->getLayoutPixelsMargin().Top;
	Float wrapperW = postWrapperWidget->getPixelsSize().getWidth();
	Float postW = postWidget->getPixelsSize().getWidth();

	// In a column flex container the first (and only) real flex item should
	// sit right after the container's top padding plus its own margin-top.
	EXPECT_NEAR( postY, wrapperTopPadding + postMarginTop, 2.f );

	// The .post div has width: 100% so it should span the full cross axis.
	// With align-items: center there is no free space, so x should be at the
	// container's left padding (not displaced toward the right edge).
	EXPECT_NEAR( postX, 0.f, 2.f );
	EXPECT_GT( postW, wrapperW * 0.5f );

	Engine::destroySingleton();
}

UTEST( UIHTML, BlockSizeInfDoesNotHang ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Block Size Inf Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/block_size_inf.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	// If we got here without hanging, the test passes.
	auto body = sceneNode->getRoot()->findByTag( "body" );
	ASSERT_TRUE( body != nullptr );
	auto bodyWidget = body->asType<UIWidget>();
	EXPECT_GT( bodyWidget->getPixelsSize().getHeight(), 0.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, KittyHomeSmallDoesNotHang ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Kitty Home Small Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/kitty_home_small.html", html );
	ASSERT_FALSE( html.empty() );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto body = sceneNode->getRoot()->findByTag( "body" );
	ASSERT_TRUE( body != nullptr );
	auto bodyWidget = body->asType<UIWidget>();
	EXPECT_GT( bodyWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( bodyWidget->getPixelsSize().getHeight(), 0.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexFormLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 653, "Flex Form Layout Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/flex_form.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto form = sceneNode->getRoot()->findByClass( "newsletter-form" );
	ASSERT_TRUE( form != nullptr );
	auto formWidget = form->asType<UIWidget>();

	// The form has display: flex with no explicit height; it must derive
	// its height from the tallest flex item (the input or button).
	EXPECT_GT( formWidget->getPixelsSize().getHeight(), 0.f );

	// Find the email input and subscribe button within the form.
	auto input = formWidget->findByTag( "input" );
	auto button = formWidget->findByTag( "button" );
	ASSERT_TRUE( input != nullptr );
	ASSERT_TRUE( button != nullptr );
	auto inputWidget = input->asType<UIWidget>();
	auto buttonWidget = button->asType<UIWidget>();

	// In a row flex container the button should sit to the right of the input.
	Float inputRight = inputWidget->getPixelsPosition().x + inputWidget->getPixelsSize().getWidth();
	EXPECT_GT( buttonWidget->getPixelsPosition().x, inputRight - 1.f );

	auto* buttonRichText = buttonWidget->isType( UI_TYPE_RICHTEXT )
							   ? buttonWidget->asType<UIRichText>()->getRichTextPtr()
							   : nullptr;
	ASSERT_TRUE( buttonRichText != nullptr );
	buttonRichText->updateLayout();
	EXPECT_EQ( buttonRichText->getLines().size(), 1u );

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexMediaQueriesLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Flex Media Queries Layout Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/flex_mediaqueries.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto body = sceneNode->getRoot()->findByTag( "body" );
	ASSERT_TRUE( body != nullptr );
	auto bodyWidget = body->asType<UIWidget>();

	// Header
	auto header = bodyWidget->findByClass( "site-header" );
	ASSERT_TRUE( header != nullptr );
	auto headerWidget = header->asType<UIWidget>();

	// Header logo (contains SVG + "Causality" text)
	auto logo = headerWidget->findByClass( "header-logo" );
	ASSERT_TRUE( logo != nullptr );
	auto logoWidget = logo->asType<UIWidget>();

	// Header wordmark (the "Causality" span)
	auto wordmark = headerWidget->findByClass( "header-wordmark" );
	ASSERT_TRUE( wordmark != nullptr );
	auto wordmarkWidget = wordmark->asType<UIWidget>();

	// Site nav
	auto nav = headerWidget->findByClass( "site-nav" );
	ASSERT_TRUE( nav != nullptr );
	auto navWidget = nav->asType<UIWidget>();

	// Header should have visible height (padding + content + padding)
	EXPECT_GT( headerWidget->getPixelsSize().getHeight(), 80.f );

	// Header should not exceed content width significantly
	EXPECT_LE( headerWidget->getPixelsSize().getWidth(), 1024.f );

	// The wordmark "Causality" should have positive width and height (visible text)
	EXPECT_GT( wordmarkWidget->getPixelsSize().getWidth(), 10.f );
	EXPECT_GT( wordmarkWidget->getPixelsSize().getHeight(), 5.f );

	// The logo (flex item in header) should not extend outside the header
	EXPECT_LE( logoWidget->getPixelsPosition().x + logoWidget->getPixelsSize().getWidth(),
			   headerWidget->getPixelsPosition().x + headerWidget->getPixelsSize().getWidth() +
				   1.f );

	// Site nav should not overflow the header
	EXPECT_LE( navWidget->getPixelsPosition().x + navWidget->getPixelsSize().getWidth(),
			   headerWidget->getPixelsPosition().x + headerWidget->getPixelsSize().getWidth() +
				   1.f );

	// Essay nav
	auto essayNav = bodyWidget->findByClass( "essay-nav" );
	ASSERT_TRUE( essayNav != nullptr );
	auto essayNavWidget = essayNav->asType<UIWidget>();

	// The essay-nav link (<a>)
	auto essayNavLink = essayNavWidget->findByClass( "essay-nav-prev" );
	ASSERT_TRUE( essayNavLink != nullptr );
	auto essayNavLinkWidget = essayNavLink->asType<UIWidget>();

	// essay-nav should contain its link (link should not overflow in local coords)
	Float linkLocalX = essayNavLinkWidget->getPixelsPosition().x;
	Float linkLocalY = essayNavLinkWidget->getPixelsPosition().y;
	Float linkLocalW = essayNavLinkWidget->getPixelsSize().getWidth();
	Float linkLocalH = essayNavLinkWidget->getPixelsSize().getHeight();
	Float navW = essayNavWidget->getPixelsSize().getWidth();
	Float navH = essayNavWidget->getPixelsSize().getHeight();

	EXPECT_GE( linkLocalX, -1.f );
	EXPECT_LE( linkLocalX + linkLocalW, navW + 1.f );
	EXPECT_GE( linkLocalY, -1.f );
	EXPECT_LE( linkLocalY + linkLocalH, navH + 1.f );

	// essay-nav should have visible size
	EXPECT_GT( essayNavWidget->getPixelsSize().getHeight(), 10.f );
	EXPECT_GT( essayNavWidget->getPixelsSize().getWidth(), 10.f );

	// The essay-nav link contains two spans: label and title
	// They should stack vertically (flex-direction: column on the <a>)
	// so the link height should be at least the sum of both span heights
	auto essayLabel = essayNavLinkWidget->findByClass( "essay-nav-label" );
	auto essayTitle = essayNavLinkWidget->findByClass( "essay-nav-title" );
	if ( essayLabel && essayTitle ) {
		auto labelWidget = essayLabel->asType<UIWidget>();
		auto titleWidget = essayTitle->asType<UIWidget>();
		EXPECT_GT( essayNavLinkWidget->getPixelsSize().getHeight(),
				   labelWidget->getPixelsSize().getHeight() +
					   titleWidget->getPixelsSize().getHeight() - 1.f );

		auto* labelRichText = essayLabel->isType( UI_TYPE_TEXTSPAN )
								  ? essayLabel->asType<UITextSpan>()->getRichTextPtr()
								  : nullptr;
		auto* titleRichText = essayTitle->isType( UI_TYPE_TEXTSPAN )
								  ? essayTitle->asType<UITextSpan>()->getRichTextPtr()
								  : nullptr;
		ASSERT_TRUE( labelRichText != nullptr );
		ASSERT_TRUE( titleRichText != nullptr );
		labelRichText->updateLayout();
		titleRichText->updateLayout();
		EXPECT_EQ( labelRichText->getLines().size(), 1u );
		EXPECT_EQ( titleRichText->getLines().size(), 1u );
	}

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexAnchorInFlexNavVisible ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Flex Anchor in Flex Nav Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/flex_mediaquery.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto body = sceneNode->getRoot()->findByTag( "body" );
	ASSERT_TRUE( body != nullptr );
	auto bodyWidget = body->asType<UIWidget>();

	auto nav = bodyWidget->findByClass( "site-nav" );
	ASSERT_TRUE( nav != nullptr );
	auto navWidget = nav->asType<UIWidget>();

	// The nav is a flex container; its <a> children are blockified flex items.
	// Each <a> should have non-zero width and height (text must be visible).
	for ( Uint32 i = 0; i < navWidget->getChildCount(); ++i ) {
		auto c = navWidget->getChildAt( i );
		auto cw = c->asType<UIWidget>();
		if ( !cw || cw->getElementTag() != "a" )
			continue;
		EXPECT_GT( cw->getPixelsSize().getWidth(), 5.f );
		EXPECT_GT( cw->getPixelsSize().getHeight(), 5.f );
	}

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexTextSpanWrapContentUsesItemWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1280, 720, "Flex Text Span Width Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/newsblur_home_prices_small.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto priceAmount = sceneNode->getRoot()->findByClass( "NB-pricing-tier-price-amount" );
	ASSERT_TRUE( priceAmount != nullptr );
	ASSERT_TRUE( priceAmount->isType( UI_TYPE_TEXTSPAN ) );

	auto* amountSpan = priceAmount->asType<UITextSpan>();
	auto* amountRichText = amountSpan->getRichTextPtr();
	ASSERT_TRUE( amountRichText != nullptr );
	amountRichText->updateLayout();

	const auto& lines = amountRichText->getLines();
	ASSERT_EQ( lines.size(), 1u );
	ASSERT_EQ( lines.front().spans.size(), 1u );
	ASSERT_EQ( lines.front().spans.front().type, RichText::RenderSpan::Type::Text );
	ASSERT_TRUE( lines.front().spans.front().text != nullptr );
	EXPECT_STRINGEQ( lines.front().spans.front().text->getString(), "$36" );
	EXPECT_NEAR( lines.front().spans.front().position.x, 0.f, 1.f );
	EXPECT_NEAR( amountSpan->getPixelsSize().getWidth(), lines.front().spans.front().size.x, 2.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, FlexLiItemsWrapContentWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Flex LI Width Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/flex_li_width.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto menuContainer = sceneNode->getRoot()->findByClass( "menu-container" );
	ASSERT_TRUE( menuContainer != nullptr );
	auto menuWidget = menuContainer->asType<UIWidget>();

	// The menu container is a flex row; its <li> children should wrap to
	// their content width, not span the full container width.
	Float containerWidth = menuWidget->getPixelsSize().getWidth();
	Float containerPadding =
		menuWidget->getPixelsPadding().Left + menuWidget->getPixelsPadding().Right;
	Float contentWidth = containerWidth - containerPadding;

	Float totalLiWidth = 0.f;
	Uint32 liCount = 0;
	for ( Uint32 i = 0; i < menuWidget->getChildCount(); ++i ) {
		auto c = menuWidget->getChildAt( i );
		if ( !c->isWidget() )
			continue;
		auto cw = c->asType<UIWidget>();
		if ( cw->getElementTag() != "li" )
			continue;

		// Each <li> should be narrower than the container content width
		EXPECT_LT( cw->getPixelsSize().getWidth(), contentWidth );

		// Accumulate total width (including margin)
		Rectf margin = cw->getLayoutPixelsMargin();
		totalLiWidth += cw->getPixelsSize().getWidth() + margin.Left + margin.Right;
		liCount++;
	}

	ASSERT_GT( liCount, (Uint32)0 );
	// With 9 items and horizontal margins, the total should still fit without
	// each item spanning the full container.
	EXPECT_LT( totalLiWidth, contentWidth * (Float)liCount );

	Engine::destroySingleton();
}

UTEST( UIHTML, ImagePercentageWidthRespectsParentMaxWidth ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "img pct width respects parent max-width", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/image_width.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	win->getInput()->update();
	SceneManager::instance()->update();
	sceneNode->updateDirtyLayouts();

	auto articles = sceneNode->getRoot()->findAllByTag( "article" );
	ASSERT_GT( articles.size(), (size_t)0 );
	auto* articleWidget = articles[0]->asType<UIWidget>();
	ASSERT_TRUE( articleWidget != nullptr );

	auto images = sceneNode->getRoot()->findAllByTag( "img" );
	ASSERT_GT( images.size(), (size_t)0 );
	auto* imgWidget = images[0]->asType<UIWidget>();
	ASSERT_TRUE( imgWidget != nullptr );

	EXPECT_GT( articleWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( articleWidget->getPixelsSize().getHeight(), 0.f );
	EXPECT_LE( articleWidget->getPixelsSize().getWidth(), 474.f );

	EXPECT_GT( imgWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( imgWidget->getPixelsSize().getHeight(), 0.f );
	EXPECT_LE( imgWidget->getPixelsSize().getWidth(), articleWidget->getPixelsSize().getWidth() );

	auto anchors = sceneNode->getRoot()->findAllByTag( "a" );
	ASSERT_GT( anchors.size(), (size_t)0 );
	auto* anchorWidget = anchors[0]->asType<UIWidget>();
	ASSERT_TRUE( anchorWidget != nullptr );
	EXPECT_GT( anchorWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( anchorWidget->getPixelsSize().getHeight(), 0.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, TextureReplaceInvalidatesRichTextAncestors ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "texture replace img relayout", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( win->getWidth(), win->getHeight() );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	std::string html = R"html(
		<!doctype html>
		<html>
		<body>
			<article id="article" style="display:block; width: 480px;">
				<p>Before image</p>
				<img id="late-img">
				<p>After image</p>
			</article>
		</body>
		</html>
	)html";
	documentScene->setURI( "file://delayed-image-resize.html" );
	documentScene->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ),
										 webView->getDocumentContainer(),
										 String::hash( "delayed-image-resize" ) );

	win->getInput()->update();
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();

	auto* article = documentScene->getRoot()->find( "article" )->asType<UIRichText>();
	auto* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	auto* doc = webView->getDocumentContainer();
	auto images = documentScene->getRoot()->findAllByTag( "img" );
	ASSERT_TRUE( article != nullptr );
	ASSERT_TRUE( body != nullptr );
	ASSERT_TRUE( doc != nullptr );
	ASSERT_EQ( images.size(), (size_t)1 );
	auto* img = images[0]->asType<UIHTMLImage>();
	ASSERT_TRUE( img != nullptr );

	Texture* texture =
		TextureFactory::instance()->createEmptyTexture( 1, 1, 4, Color::Transparent );
	ASSERT_TRUE( texture != nullptr );
	img->setDrawable( texture );
	documentScene->updateDirtyLayouts();

	Float articleInitialHeight = article->getPixelsSize().getHeight();
	Float bodyInitialHeight = body->getPixelsSize().getHeight();
	Float docInitialHeight = doc->getPixelsSize().getHeight();

	Image loadedImage( 320, 180, 4, Color::White );
	texture->replace( &loadedImage );
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();

	EXPECT_GT( article->getPixelsSize().getHeight(), articleInitialHeight + 150.f );
	EXPECT_GT( body->getPixelsSize().getHeight(), bodyInitialHeight + 150.f );
	EXPECT_GT( doc->getPixelsSize().getHeight(), docInitialHeight + 150.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, HtmlContainsTableBodyHeight ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "html contains table body height", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( win->getWidth(), win->getHeight() );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	std::string html;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/hn_frontpage.html", html ) );
	for ( std::string::size_type pos = 0;
		  ( pos = html.find( "<link rel=\"stylesheet\"", pos ) ) != std::string::npos; ) {
		auto end = html.find( "/>", pos );
		ASSERT_TRUE( end != std::string::npos );
		html.erase( pos, end + 2 - pos );
	}

	documentScene->setURI( "file://html-table-body-height.html" );
	documentScene->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ),
										 webView->getDocumentContainer(),
										 String::hash( "html-table-body-height" ) );

	win->getInput()->update();
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();

	std::string css;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/base.css", css ) );
	css += "\n";
	std::string newsCss;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/news.css", newsCss ) );
	css += newsCss;
	documentScene->runOnMainThread( [documentScene, css = std::move( css )] {
		documentScene->combineStyleSheet( css, true,
										  String::hash( "html-table-body-height-late-css" ) );
	} );
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();

	auto* htmlNode = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	auto* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	auto* table = documentScene->getRoot()->find( "hnmain" )->asType<UIWidget>();
	ASSERT_TRUE( htmlNode != nullptr );
	ASSERT_TRUE( body != nullptr );
	ASSERT_TRUE( table != nullptr );

	EXPECT_GE( htmlNode->getPixelsSize().getHeight(), body->getPixelsSize().getHeight() );
	EXPECT_GE( body->getPixelsSize().getHeight() + 4.f, table->getPixelsSize().getHeight() );
	EXPECT_GT( table->getPixelsSize().getHeight(), 500.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, BodyDocumentContentMinHeightCanShrink ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "body content min height shrinks", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( win->getWidth(), win->getHeight() );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	std::string html = R"html(
		<!doctype html>
		<html>
		<body style="margin: 0;">
			<div id="spacer" style="display: block; width: 100px; height: 900px;"></div>
		</body>
		</html>
	)html";

	documentScene->setURI( "file://body-content-min-height-shrink.html" );
	documentScene->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ),
										 webView->getDocumentContainer(),
										 String::hash( "body-content-min-height-shrink" ) );
	webView->refreshDocumentLayout();

	win->getInput()->update();
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();

	auto* htmlNode = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	auto* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	auto* spacer = documentScene->getRoot()->find( "spacer" )->asType<UIWidget>();
	ASSERT_TRUE( htmlNode != nullptr );
	ASSERT_TRUE( body != nullptr );
	ASSERT_TRUE( spacer != nullptr );

	const Float tallBodyHeight = body->getPixelsSize().getHeight();
	EXPECT_GT( spacer->getPixelsSize().getHeight(), 850.f );
	EXPECT_GT( tallBodyHeight, 850.f );

	spacer->setStyleSheetProperty( StyleSheetProperty( "height", "120px" ) );
	win->getInput()->update();
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();
	win->getInput()->update();
	SceneManager::instance()->update();
	documentScene->updateDirtyLayouts();

	EXPECT_LT( spacer->getPixelsSize().getHeight(), 180.f );
	EXPECT_LT( body->getPixelsSize().getHeight(), tallBodyHeight - 250.f );
	EXPECT_LT( htmlNode->getPixelsSize().getHeight(), tallBodyHeight - 250.f );
	EXPECT_GE( body->getPixelsSize().getHeight() + 1.f, webView->getPixelsSize().getHeight() );
	EXPECT_GE( htmlNode->getPixelsSize().getHeight() + 1.f, webView->getPixelsSize().getHeight() );

	Engine::destroySingleton();
}

UTEST( UIHTML, DeferredCSSKeepsTableHeightStableAfterViewportResize ) {
	std::shared_ptr<ThreadPool> threadPool(
		ThreadPool::createShared( eemax<int>( 4, Sys::getCPUCount() ) ) );
	Http::setThreadPool( threadPool );
	SystemFontResolver::setEnabled( true );
	UIApplication app(
		WindowSettings{ 1280, 720, "deferred css table height stable", WindowStyle::Default,
						WindowBackend::Default, 32 },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5 ),
		ContextSettings( false, ContextSettings::FrameRateLimitScreenRefreshRate ) );

	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = app.getWindow();
	ASSERT_TRUE( win != nullptr );

	win->setPosition( 0, 0 );

	UISceneNode* ui = app.getUI();
	ASSERT_TRUE( ui != nullptr );
	ui->setThreadPool( threadPool );
	ui->setColorSchemePreference( ColorSchemeExtPreference::Light );

	ui->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	ui->loadLayoutFromString( R"xml(
	<vbox layout_width="match_parent" layout_height="match_parent">
		<WebView id="webview" layout_width="match_parent" layout_height="0" layout_weight="1" />
	</vbox>
	)xml",
							  nullptr, app.getStyleSheetDefaultMarker() );

	UIWebView* webView = ui->find( "webview" )->asType<UIWebView>();
	webView->setStyleSheetDefaultMarker( app.getStyleSheetDefaultMarker() );
	webView->loadURI( "assets/html/hn_empty_thread.html" );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	auto* bigboxTd = documentScene->getRoot()->querySelector( "#bigbox > td" );
	auto* bigboxTable = documentScene->getRoot()->querySelector( "#bigbox > td > table" );
	ASSERT_TRUE( bigboxTd != nullptr );
	ASSERT_TRUE( bigboxTable != nullptr );

	auto* titleCell = documentScene->getRoot()->querySelector( ".title" );
	ASSERT_TRUE( titleCell != nullptr );
	ASSERT_TRUE( titleCell->isType( UI_TYPE_RICHTEXT ) );

	const Color expectedTitleColor( "#828282" );
	for ( int i = 0;
		  i < 120 && titleCell->asType<UIRichText>()->getFontColor() != expectedTitleColor; ++i ) {
		win->getInput()->update();
		SceneManager::instance()->update();
		Sys::sleep( Milliseconds( 1 ) );
	}

	ASSERT_TRUE( titleCell->asType<UIRichText>()->getFontColor() == expectedTitleColor );
	win->getInput()->update();
	SceneManager::instance()->update();

	const Float initialTdHeight = bigboxTd->getPixelsSize().getHeight();
	const Float initialTableHeight = bigboxTable->getPixelsSize().getHeight();

	EXPECT_GT( initialTdHeight, 0.f );
	EXPECT_GT( initialTableHeight, 0.f );

	win->setSize( win->getWidth(), win->getHeight() + 90 );
	win->getInput()->update();
	SceneManager::instance()->update();

	win->setSize( win->getWidth(), win->getHeight() - 90 );
	win->getInput()->update();
	SceneManager::instance()->update();

	// These tests are currently unstable, most likely due to a bug in UIHTMLTextArea
	// EXPECT_NEAR( initialTdHeight, bigboxTd->getPixelsSize().getHeight(), 0.1f );
	// EXPECT_NEAR( initialTableHeight, bigboxTable->getPixelsSize().getHeight(), 0.1f );
}

UTEST( UIHTML, ImageCSSWidthOverridesHTMLWidthAttribute ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "img css width overrides html width attr", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/image_width_2.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	win->getInput()->update();
	SceneManager::instance()->update();
	sceneNode->updateDirtyLayouts();

	auto images = sceneNode->getRoot()->findAllByTag( "img" );
	ASSERT_GT( images.size(), (size_t)0 );
	auto* imgWidget = images[0]->asType<UIWidget>();
	ASSERT_TRUE( imgWidget != nullptr );

	auto mains = sceneNode->getRoot()->findAllByTag( "main" );
	ASSERT_GT( mains.size(), (size_t)0 );
	auto* mainWidget = mains[0]->asType<UIWidget>();
	ASSERT_TRUE( mainWidget != nullptr );

	Float imgWidth = imgWidget->getPixelsSize().getWidth();
	Float imgHeight = imgWidget->getPixelsSize().getHeight();
	Float mainWidth = mainWidget->getPixelsSize().getWidth();

	EXPECT_GT( imgWidth, 0.f );
	EXPECT_GT( imgHeight, 0.f );

	// CSS width: 100% should override HTML width="1500"
	// Image should be no wider than its container
	EXPECT_LE( imgWidth, mainWidth );

	// Image should NOT be 1500 (the HTML attribute value)
	EXPECT_LT( imgWidth, 1500.f );

	Engine::destroySingleton();
}

UTEST( UIHTML, ImageMaxWidthConstrainsWebpWithHeightAuto ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "img max-width constrains webp with height auto",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_inline_block();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string html;
	FileSystem::fileGet( "assets/html/image_width_3.html", html );
	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );

	win->getInput()->update();
	SceneManager::instance()->update();
	sceneNode->updateDirtyLayouts();

	auto articles = sceneNode->getRoot()->findAllByTag( "article" );
	ASSERT_GT( articles.size(), (size_t)0 );
	auto* articleWidget = articles[0]->asType<UIWidget>();
	ASSERT_TRUE( articleWidget != nullptr );

	auto images = sceneNode->getRoot()->findAllByTag( "img" );
	ASSERT_GT( images.size(), (size_t)0 );
	auto* imgWidget = images[0]->asType<UIWidget>();
	ASSERT_TRUE( imgWidget != nullptr );

	EXPECT_GT( articleWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( articleWidget->getPixelsSize().getHeight(), 0.f );
	EXPECT_LE( articleWidget->getPixelsSize().getWidth(), 1140.f );

	Float imgWidth = imgWidget->getPixelsSize().getWidth();
	Float imgHeight = imgWidget->getPixelsSize().getHeight();

	EXPECT_GT( imgWidth, 0.f );
	EXPECT_GT( imgHeight, 0.f );
	EXPECT_LE( imgWidth, articleWidget->getPixelsSize().getWidth() );

	// max-width: 100% should prevent the image from using the HTML width="2560"
	EXPECT_LT( imgWidth, 2560.f );

	// height should be proportional to width (aspect ratio: 1436/2560)
	Float expectedRatio = 1436.f / 2560.f;
	Float actualRatio = imgHeight / imgWidth;
	EXPECT_GT( actualRatio, expectedRatio * 0.9f );
	EXPECT_LT( actualRatio, expectedRatio * 1.1f );

	Engine::destroySingleton();
}
