#include "utest.h"
#include <eepp/scene/node.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::Scene;

UTEST( CSSInheritance, HtmlXmlLoadingInheritance ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Inheritance Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash() ) );

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
    if ( Color("#FF0000") != div->getFontColor() ) {
        printf("div color is: %s\n", div->getFontColor().toHexString().c_str());
    }
    EXPECT_TRUE( Color("#FF0000") == div->getFontColor() );
}
