#include "utest.hpp"
#include <eepp/scene/node.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::Scene;

UTEST( CSSInheritance, HtmlXmlLoadingInheritance ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Inheritance Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash() ), 1 );

	std::string xml = R"(
<html>
	<head>
		<style>
		body {
		  background-color: white;
		  color: #FF0000;
		}
		</style>
	</head>
<body>
	<div id="testdiv">This is not color black</div>
</body>
</html>
    )";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	// Check if the div inherited the color black (#FF0000) from body
	if ( Color( "#FF0000" ) != div->getFontColor() ) {
		printf( "div color is: %s\n", div->getFontColor().toHexString().c_str() );
	}
	EXPECT_TRUE( Color( "#FF0000" ) == div->getFontColor() );
}

UTEST( CSSInheritance, ComputedFontSize ) {
	for ( Float scale : { 1.f, 1.5f, 2.f, 2.5f } ) {
		UTEST_PRINT_STEP( String::format( "SCALE %.1f", scale ).c_str() );
		UIApplication app( WindowSettings( 800, 600, "eepp - CSS Inheritance Test",
										   WindowStyle::Default, WindowBackend::Default, 32 ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), scale ) );

		std::string xml = R"(
<html>
	<head>
		<style>
		body {
		  font-size: 16px;
		}
		h1 {
		  font-size: 2em;
		}
		</style>
	</head>
<body>
	<h1 id="testh1">test text</h1>
</body>
</html>
    )";

		UIWidget* root = app.getUI()->loadLayoutFromString( xml );
		EXPECT_TRUE( root != nullptr );

		UIRichText* h1 = root->querySelector( "#testh1" )->asType<UIRichText>();
		EXPECT_TRUE( h1 != nullptr );

		EXPECT_NEAR( 32u * scale, h1->getFontSize(), 1.f );

		Node* child = h1->getFirstChild();
		EXPECT_TRUE( child != nullptr );
		EXPECT_TRUE( child->isWidget() );

		UIWidget* childWidget = child->asType<UIWidget>();
		std::string pxStr = childWidget->getPropertyString(
			StyleSheetSpecification::instance()->getProperty( PropertyId::FontSize ) );
		EXPECT_NEAR( 32u * scale, childWidget->asType<UITextSpan>()->getFontSize(), 1.f );
	}
}

UTEST( CSSInheritance, ComputedFontSizePercentageAndRem ) {
	for ( Float scale : { 1.f, 1.5f, 2.f, 2.5f } ) {
		UTEST_PRINT_STEP( String::format( "SCALE %.1f", scale ).c_str() );
		UIApplication app( WindowSettings( 800, 600, "eepp - CSS Inheritance Test 2",
										   WindowStyle::Default, WindowBackend::Default, 32 ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), scale ) );

		std::string xml = R"(
	<html>
	<head>
		<style>
		html {
		  font-size: 20px;
		}
		body {
		  font-size: 10px;
		}
		#parent {
		  font-size: 1.5rem; /* 1.5 * 20 = 30px */
		}
		.child {
		  font-size: 200%; /* 200% of 30px = 60px */
		}
		.generic {
		  font-size: 0.5em; /* 0.5 * 60px = 30px */
		}
		</style>
	</head>
	<body>
	<div id="parent">
		<div class="child">
			<div class="generic">
				<span id="targetspan">target</span>
			</div>
		</div>
	</div>
	</body>
	</html>
	)";

		UIWidget* root = app.getUI()->loadLayoutFromString( xml );
		EXPECT_TRUE( root != nullptr );

		UIWidget* targetSpan = root->querySelector( "#targetspan" );
		EXPECT_TRUE( targetSpan != nullptr );

		EXPECT_NEAR( 18u * scale, targetSpan->asType<UITextSpan>()->getFontSize(), 1.f );
	}
}
