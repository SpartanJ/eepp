#include "utest.h"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uihtmlimage.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisvg.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Tools;

static UISceneNode* createScene() {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "SVG Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	if ( !font->loaded() ) {
		Engine::destroySingleton();
		return nullptr;
	}
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	return sceneNode;
}

static void destroyScene( UISceneNode* sceneNode ) {
	eeDelete( sceneNode );
	Engine::destroySingleton();
}

static UISvg* findSvgWidget( UIWidget* root ) {
	return root->findByType<UISvg>( UI_TYPE_SVG );
}

UTEST( UISvg, basicInlineSvg ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<svg width="100" height="100"><circle cx="50" cy="50" r="40" fill="red"/></svg>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_TRUE( svgWidget->isType( UI_TYPE_SVG ) );
	EXPECT_FALSE( svgWidget->getSvgXml().empty() );
	EXPECT_GT( svgWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( svgWidget->getPixelsSize().getHeight(), 0.f );

	destroyScene( sceneNode );
}

UTEST( UISvg, svgWithViewBox ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<svg viewBox="0 0 200 200" width="100" height="100"><rect width="100" height="100" fill="blue"/></svg>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_FALSE( svgWidget->getSvgXml().empty() );
	EXPECT_GT( svgWidget->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( svgWidget->getPixelsSize().getHeight(), 0.f );

	destroyScene( sceneNode );
}

UTEST( UISvg, svgWithXmlns ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"><rect width="100" height="100" fill="green"/></svg>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_EQ( svgWidget->getType(), UI_TYPE_SVG );

	destroyScene( sceneNode );
}

UTEST( UISvg, svgChildrenNotCreatedAsUiWidgets ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<div>
<svg width="100" height="100">
  <circle cx="50" cy="50" r="40" fill="red"/>
  <rect width="30" height="30" fill="blue"/>
  <path d="M10 10 L20 20" stroke="black"/>
</svg>
</div>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_TRUE( svgWidget->loadsItsChildren() );
	EXPECT_TRUE( svgWidget->isType( UI_TYPE_SVG ) );

	// SVG's child count should be 0 because we set UI_LOADS_ITS_CHILDREN
	// which prevents the parent from creating widgets for SVG's internal elements
	EXPECT_EQ( svgWidget->getChildCount(), 0u );

	destroyScene( sceneNode );
}

UTEST( UISvg, multipleSvgElements ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<svg width="50" height="50"><circle cx="25" cy="25" r="20" fill="red"/></svg>
<svg width="50" height="50"><rect width="50" height="50" fill="blue"/></svg>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidgets = rootWidget->findAllByTag( "svg" );
	EXPECT_EQ( svgWidgets.size(), 2u );

	for ( auto* w : svgWidgets ) {
		EXPECT_TRUE( w->isType( UI_TYPE_SVG ) );
		EXPECT_FALSE( static_cast<UISvg*>( w )->getSvgXml().empty() );
	}

	destroyScene( sceneNode );
}

UTEST( UISvg, svgWithNoDimensions ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<svg><circle cx="50" cy="50" r="40" fill="red"/></svg>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_FALSE( svgWidget->getSvgXml().empty() );
	EXPECT_TRUE( svgWidget->isType( UI_TYPE_SVG ) );

	destroyScene( sceneNode );
}

UTEST( UISvg, svgInsideBlockElement ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<div>
  <p>Before SVG</p>
  <svg width="100" height="100"><circle cx="50" cy="50" r="40" fill="red"/></svg>
  <p>After SVG</p>
</div>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_TRUE( svgWidget->isType( UI_TYPE_SVG ) );
	EXPECT_FALSE( svgWidget->getSvgXml().empty() );

	destroyScene( sceneNode );
}

UTEST( UISvg, percentageHeightResolvesAfterContainingBlockLayout ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<button id="button" style="height:34.5px;padding:2px 6px 3px">
  <svg id="icon" viewBox="0 0 26 26" style="width:100%;height:100%">
    <path d="M11 2a9 9 0 105.641 16.01l3.652 3.697 1.414-1.414-3.697-3.653A9 9 0 0011 2z"/>
  </svg>
</button>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );
	sceneNode->updateDirtyLayouts();

	auto* button = rootWidget->querySelector( "#button" );
	auto* svgWidget = rootWidget->querySelector( "#icon" )->asType<UISvg>();
	ASSERT_TRUE( button != nullptr );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_EQ( svgWidget->getLayoutHeightPolicy(), SizePolicy::Fixed );
	EXPECT_GT( svgWidget->getPixelsSize().getHeight(), 0.f );
	EXPECT_NEAR( svgWidget->getPixelsSize().getHeight(), 34.5f, 1.f );

	button->setStyleSheetProperty( StyleSheetProperty( "height", "50px" ) );
	sceneNode->updateDirtyLayouts();
	EXPECT_NEAR( svgWidget->getPixelsSize().getHeight(), 50.f, 1.f );

	destroyScene( sceneNode );
}

UTEST( UISvg, svgWithMemoryAsset ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<svg width="100" height="100">
  <circle cx="50" cy="50" r="20" fill="red" stroke="black" stroke-width="3"/>
  <rect x="30" y="30" width="40" height="40" fill="blue" opacity="0.5"/>
</svg>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );
	EXPECT_TRUE( svgWidget->isType( UI_TYPE_SVG ) );

	std::string xml = svgWidget->getSvgXml();
	EXPECT_FALSE( xml.empty() );
	EXPECT_TRUE( xml.find( "<circle" ) != std::string::npos );
	EXPECT_TRUE( xml.find( "<rect" ) != std::string::npos );

	destroyScene( sceneNode );
}

// --- UIHTMLImage tests ---

UTEST( UIHTMLImage, imgElementCreatesHtmlImage ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<img src="nonexistent.png" alt="Test Image"/>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto imgWidget = rootWidget->findByType<UIHTMLImage>( UI_TYPE_HTML_IMAGE );
	ASSERT_TRUE( imgWidget != nullptr );
	EXPECT_TRUE( imgWidget->isType( UI_TYPE_HTML_IMAGE ) );
	EXPECT_STREQ( imgWidget->getElementTag().c_str(), "img" );

	destroyScene( sceneNode );
}

UTEST( UIHTMLImage, altAttributeCaptured ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<img src="broken.png" alt="My broken image"/>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto imgWidget = rootWidget->findByType<UIHTMLImage>( UI_TYPE_HTML_IMAGE );
	ASSERT_TRUE( imgWidget != nullptr );
	EXPECT_STREQ( imgWidget->getAlt().c_str(), "My broken image" );

	destroyScene( sceneNode );
}

UTEST( UIHTMLImage, noAltAttributeEmpty ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<img src="some.png"/>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto imgWidget = rootWidget->findByType<UIHTMLImage>( UI_TYPE_HTML_IMAGE );
	ASSERT_TRUE( imgWidget != nullptr );
	EXPECT_TRUE( imgWidget->getAlt().empty() );

	destroyScene( sceneNode );
}

UTEST( UIHTMLImage, multipleImgElements ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<img alt="First"/>
<img alt="Second"/>
<img alt="Third"/>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto imgWidgets = rootWidget->findAllByTag( "img" );
	EXPECT_EQ( imgWidgets.size(), 3u );

	for ( auto* w : imgWidgets )
		EXPECT_TRUE( w->isType( UI_TYPE_HTML_IMAGE ) );

	destroyScene( sceneNode );
}

UTEST( UIHTMLImage, imgAndSvgTogether ) {
	auto sceneNode = createScene();
	ASSERT_TRUE( sceneNode != nullptr );

	std::string html = R"html(<!doctype html>
<html>
<body>
<p>Before</p>
<img id="img1" alt="An image"/>
<svg width="50" height="50"><circle cx="25" cy="25" r="20" fill="red"/></svg>
<img id="img2" alt="Another image"/>
</body>
</html>)html";

	auto rootWidget = sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( html ) );
	ASSERT_TRUE( rootWidget != nullptr );

	sceneNode->update( Time::Zero );

	auto svgWidget = findSvgWidget( rootWidget );
	ASSERT_TRUE( svgWidget != nullptr );

	auto imgWidget1 = rootWidget->find<UIHTMLImage>( "img1" );
	ASSERT_TRUE( imgWidget1 != nullptr );
	EXPECT_STREQ( imgWidget1->getAlt().c_str(), "An image" );

	auto imgWidget2 = rootWidget->find<UIHTMLImage>( "img2" );
	ASSERT_TRUE( imgWidget2 != nullptr );
	EXPECT_STREQ( imgWidget2->getAlt().c_str(), "Another image" );

	destroyScene( sceneNode );
}
