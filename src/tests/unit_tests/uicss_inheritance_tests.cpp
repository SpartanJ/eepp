#include "utest.hpp"
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetpropertyanimation.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::Scene;
using namespace EE::Graphics;

UTEST( CSSParser, UnknownPropertiesKeepDistinctNameHashes ) {
	StyleSheetProperty first( "data-first", "one" );
	StyleSheetProperty second( "data-second", "two" );

	EXPECT_TRUE( first.getPropertyDefinition() == nullptr );
	EXPECT_TRUE( second.getPropertyDefinition() == nullptr );
	EXPECT_EQ( first.getId(), String::hash( "data-first" ) );
	EXPECT_EQ( second.getId(), String::hash( "data-second" ) );
	EXPECT_NE( first.getId(), second.getId() );
	EXPECT_TRUE( first.getPropertyId() == PropertyId::Invalid );
	EXPECT_TRUE( first.getShorthandId() == ShorthandId::Invalid );
}

UTEST( CSSParser, CommentsPreserveDeclarationContext ) {
	StyleSheetPropertiesParser parser( R"css(
		color /* before colon */ : red;
		margin: 1px /* between values */ 2px;
		font-family: "A/* literal */B";
		/* between declarations */ width: 140px;
		line-height: 40px !important /* final declaration without semicolon */
	)css" );
	const auto& properties = parser.getProperties();

	auto lineHeight = properties.find(
		StyleSheetSpecification::instance()->getProperty( PropertyId::LineHeight )->getId() );
	auto color = properties.find(
		StyleSheetSpecification::instance()->getProperty( PropertyId::Color )->getId() );
	auto marginTop = properties.find(
		StyleSheetSpecification::instance()->getProperty( PropertyId::MarginTop )->getId() );
	auto marginRight = properties.find(
		StyleSheetSpecification::instance()->getProperty( PropertyId::MarginRight )->getId() );
	auto fontFamily = properties.find(
		StyleSheetSpecification::instance()->getProperty( PropertyId::FontFamily )->getId() );
	auto width = properties.find(
		StyleSheetSpecification::instance()->getProperty( PropertyId::Width )->getId() );

	ASSERT_TRUE( lineHeight != properties.end() );
	ASSERT_TRUE( color != properties.end() );
	ASSERT_TRUE( marginTop != properties.end() );
	ASSERT_TRUE( marginRight != properties.end() );
	ASSERT_TRUE( fontFamily != properties.end() );
	ASSERT_TRUE( width != properties.end() );
	EXPECT_TRUE( lineHeight->second.getValue() == "40px" );
	EXPECT_TRUE( color->second.getValue() == "red" );
	EXPECT_TRUE( marginTop->second.getValue() == "1px" );
	EXPECT_TRUE( marginRight->second.getValue() == "2px" );
	EXPECT_TRUE( fontFamily->second.getValue().find( "/* literal */" ) != std::string::npos );
	EXPECT_TRUE( width->second.getValue() == "140px" );
}

UTEST( CSSParser, StatementAtRuleDoesNotConsumeFollowingRule ) {
	StyleSheetParser parser;
	ASSERT_TRUE( parser.loadFromString( std::string_view{ R"css(
		@charset "UTF-8";
		:root { --color-bg: #0c0c0c; }
	)css" } ) );

	auto rootStyle = parser.getStyleSheet().getStyleFromSelector( ":root" );
	ASSERT_TRUE( rootStyle != nullptr );
	EXPECT_TRUE( rootStyle->getVariableByName( "--color-bg" ).getValue() == "#0c0c0c" );
}

UTEST( CSSInheritance, HtmlXmlLoadingInheritance ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Inheritance Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
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
    )html";

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

		std::string xml = R"html(
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
    )html";

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
		EXPECT_FALSE( pxStr.empty() );
		EXPECT_NEAR( 32u * scale,
					 childWidget->lengthFromValue( StyleSheetProperty( "font-size", pxStr ) ),
					 1.f );
	}
}

UTEST( CSSInheritance, ComputedFontSizePercentageAndRem ) {
	for ( Float scale : { 1.f, 1.5f, 2.f, 2.5f } ) {
		UTEST_PRINT_STEP( String::format( "SCALE %.1f", scale ).c_str() );
		UIApplication app( WindowSettings( 800, 600, "eepp - CSS Inheritance Test 2",
										   WindowStyle::Default, WindowBackend::Default, 32 ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), scale ) );

		std::string xml = R"html(
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
	)html";

		UIWidget* root = app.getUI()->loadLayoutFromString( xml );
		EXPECT_TRUE( root != nullptr );

		UIWidget* targetSpan = root->querySelector( "#targetspan" );
		EXPECT_TRUE( targetSpan != nullptr );

		EXPECT_NEAR( 30u * scale, targetSpan->asType<UITextSpan>()->getFontSize(), 1.f );
	}
}

UTEST( CSSInheritance, ExplicitColorInherit ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Color Inherit Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			color: #FF0000;
		}
		#child {
			color: inherit;
		}
		</style>
	</head>
<body>
	<div>color set on body<div id="child">should be red via inherit</div></div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* child = root->querySelector( "#child" )->asType<UIRichText>();
	EXPECT_TRUE( child != nullptr );

	EXPECT_TRUE( Color( "#FF0000" ) == child->getFontColor() );
}

UTEST( CSSInheritance, ExplicitFontSizeInherit ) {
	for ( Float scale : { 1.f, 1.5f, 2.f } ) {
		UTEST_PRINT_STEP( String::format( "SCALE %.1f", scale ).c_str() );
		UIApplication app( WindowSettings( 800, 600, "eepp - CSS FontSize Inherit Test",
										   WindowStyle::Default, WindowBackend::Default, 32 ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), scale ) );

		std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			font-size: 16px;
		}
		#parent {
			font-size: 24px;
		}
		#child {
			font-size: inherit;
		}
		</style>
	</head>
<body>
	<div id="parent">parent<div id="child">child with inherit</div></div>
</body>
</html>
    )html";

		UIWidget* root = app.getUI()->loadLayoutFromString( xml );
		EXPECT_TRUE( root != nullptr );

		UIRichText* child = root->querySelector( "#child" )->asType<UIRichText>();
		EXPECT_TRUE( child != nullptr );
		EXPECT_NEAR( 24u * scale, child->getFontSize(), 1.f );
	}
}

UTEST( CSSInheritance, ExplicitFontSizeInheritEm ) {
	for ( Float scale : { 1.f, 1.5f, 2.f } ) {
		UTEST_PRINT_STEP( String::format( "SCALE %.1f", scale ).c_str() );
		UIApplication app( WindowSettings( 800, 600, "eepp - CSS FontSize Inherit Em Test",
										   WindowStyle::Default, WindowBackend::Default, 32 ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), scale ) );

		std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			font-size: 24px;
		}
		#parent {
			font-size: 1.5em;
		}
		#child {
			font-size: inherit;
		}
		</style>
	</head>
<body>
	<div id="parent">parent (1.5em = 36px)<div id="child">inherit should be 36px</div></div>
</body>
</html>
    )html";

		UIWidget* root = app.getUI()->loadLayoutFromString( xml );
		EXPECT_TRUE( root != nullptr );

		UIRichText* child = root->querySelector( "#child" )->asType<UIRichText>();
		EXPECT_TRUE( child != nullptr );
		// 1.5 * 24 = 36px; inherit resolves to parent's computed 36px, not 1.5em
		EXPECT_NEAR( 36u * scale, child->getFontSize(), 1.f );
	}
}

UTEST( CSSInheritance, ExplicitFontFamilyInherit ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS FontFamily Inherit Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		#parent {
			font-family: monospace;
		}
		#child {
			font-family: inherit;
		}
		</style>
	</head>
<body>
	<div id="parent"><div id="child">child with font-family: inherit</div></div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* parent = root->querySelector( "#parent" )->asType<UIRichText>();
	EXPECT_TRUE( parent != nullptr );
	UIRichText* child = root->querySelector( "#child" )->asType<UIRichText>();
	EXPECT_TRUE( child != nullptr );

	if ( parent->getFont() && child->getFont() ) {
		EXPECT_TRUE( parent->getFont() == child->getFont() );
	}
}

UTEST( CSSInheritance, ExplicitBackgroundColorInherit ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS BGColor Inherit Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			background-color: #00FF00;
		}
		#child {
			background-color: inherit;
		}
		</style>
	</head>
<body>
	<div id="child">child with background-color: inherit</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* child = root->querySelector( "#child" )->asType<UIRichText>();
	EXPECT_TRUE( child != nullptr );

	EXPECT_TRUE( Color( "#00FF00" ) == child->getBackgroundColor() );
}

UTEST( CSSUnits, ExFallback ) {
	StyleSheetLength len( "100ex" );
	Float result = len.asPixels( 0, Sizef::Zero, 96, 16, 16 );
	EXPECT_EQ( Math::round( 100.f * 16.f * 0.5f ), result );
}

UTEST( CSSUnits, ChFallback ) {
	StyleSheetLength len( "100ch" );
	Float result = len.asPixels( 0, Sizef::Zero, 96, 16, 16 );
	EXPECT_EQ( Math::round( 100.f * 16.f * 0.5f ), result );
}

UTEST( CSSUnits, ExChWithFont ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Units Ex/Ch Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	Graphics::Font* font = app.getUI()->getUIThemeManager()->getDefaultFont();
	EXPECT_TRUE( font != nullptr );

	Float elFontSize = 24.f;

	Float resultEx = StyleSheetLength( "1ex" ).asPixels( 0, Sizef::Zero, 96, elFontSize, 16, font );
	Float resultCh = StyleSheetLength( "1ch" ).asPixels( 0, Sizef::Zero, 96, elFontSize, 16, font );
	Float resultEm = StyleSheetLength( "1em" ).asPixels( 0, Sizef::Zero, 96, elFontSize, 16, font );
	Float fallback = Math::round( elFontSize * 0.5f );

	EXPECT_GT( resultEx, 0.f );
	EXPECT_GT( resultCh, 0.f );
	EXPECT_EQ( elFontSize, resultEm );
	EXPECT_LE( resultEx, resultEm );
	EXPECT_LE( resultCh, resultEm );
	EXPECT_NE( fallback, resultEx );
	EXPECT_NE( fallback, resultCh );
}

UTEST( CSSVariables, VariableReferencesSimple ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Var Ref Test 1", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			--primary: #FF0000;
			--text-color: var(--primary);
			color: var(--text-color);
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	EXPECT_TRUE( Color( "#FF0000" ) == div->getFontColor() );
}

UTEST( CSSVariables, StyleSheetPropertyNeedsValueSubstitution ) {
	EXPECT_FALSE( StyleSheetProperty( "width", "42dp" ).needsValueSubstitution() );
	EXPECT_TRUE( StyleSheetProperty( "color", "var(--text)" ).needsValueSubstitution() );
	EXPECT_TRUE( StyleSheetProperty( "color", "light-dark(#000, #fff)" ).needsValueSubstitution() );
	EXPECT_FALSE( StyleSheetProperty( "width", "calc(100% - 2dp)" ).needsValueSubstitution() );
	EXPECT_FALSE(
		StyleSheetProperty( "width", "min(100%, max(20dp, 10vw))" ).needsValueSubstitution() );
	EXPECT_FALSE( StyleSheetProperty( "width", "clamp(10dp, calc(50% - 1dp), 100dp)" )
					  .needsValueSubstitution() );
}

UTEST( CSSVariables, StyleAttributeVarOnRichTextAndTextSpan ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Style Attribute Var Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<!doctype html>
<html>
	<head>
		<style>
		:root{--astro-code-background:#1a1714;--astro-code-foreground:#F0EEEC;--astro-code-token-keyword:#F59E0B;--astro-code-token-function:#FDE68A}*,*:before,*:after{margin:0;padding:0;box-sizing:border-box}
		body{background:var(--astro-code-background);color:var(--astro-code-foreground)}
		</style>
	</head>
<body>
	<pre id="code" style="background-color: var(--astro-code-background); color: var(--astro-code-foreground);">
		<code><span class="line"><span id="keyword" style="color:var(--astro-code-token-keyword)">async</span><span id="function" style="color:var(--astro-code-token-function)"> loadDashboard</span><span id="foreground" style="color:var(--astro-code-foreground)">()</span></span></code>
	</pre>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( xml ) );
	EXPECT_TRUE( root != nullptr );

	UIRichText* code = root->querySelector( "#code" )->asType<UIRichText>();
	EXPECT_TRUE( code != nullptr );

	UITextSpan* keyword = root->querySelector( "#keyword" )->asType<UITextSpan>();
	EXPECT_TRUE( keyword != nullptr );
	UITextSpan* function = root->querySelector( "#function" )->asType<UITextSpan>();
	EXPECT_TRUE( function != nullptr );
	UITextSpan* foreground = root->querySelector( "#foreground" )->asType<UITextSpan>();
	EXPECT_TRUE( foreground != nullptr );
	UIRichText* body = root->querySelector( "body" )->asType<UIRichText>();
	EXPECT_TRUE( body != nullptr );
	UIRichText* html = root->querySelector( "html" )->asType<UIRichText>();
	EXPECT_TRUE( html != nullptr );

	EXPECT_TRUE( Color( "#1a1714" ) == html->getBackgroundColor() );
	EXPECT_TRUE( Color( "#F0EEEC" ) == body->getFontColor() );
	EXPECT_TRUE( Color( "#1a1714" ) == code->getBackgroundColor() );
	EXPECT_TRUE( Color( "#F0EEEC" ) == code->getFontColor() );
	EXPECT_TRUE( Color( "#F59E0B" ) == keyword->getFontColor() );
	EXPECT_TRUE( Color( "#FDE68A" ) == function->getFontColor() );
	EXPECT_TRUE( Color( "#F0EEEC" ) == foreground->getFontColor() );
}

UTEST( CSSVariables, StyleAttributeDeclaresAndInheritsCustomProperty ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Inline Custom Property Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<body>
		<div id="parent" style="--inline-color: #13579B; color: var(--inline-color);">
			<span id="child" style="color: var(--inline-color)">Child</span>
		</div>
	</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* parent = root->querySelector( "#parent" )->asType<UIRichText>();
	EXPECT_TRUE( parent != nullptr );

	UITextSpan* child = root->querySelector( "#child" )->asType<UITextSpan>();
	EXPECT_TRUE( child != nullptr );

	EXPECT_TRUE( Color( "#13579B" ) == parent->getFontColor() );
	EXPECT_TRUE( Color( "#13579B" ) == child->getFontColor() );
}

UTEST( CSSVariables, InheritedPropertyResolvesVarFromParent ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Inherited Var Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIWidget* root = app.getUI()->loadLayoutFromString( R"html(
<html>
	<head>
		<style>
			:root { --inherited-color: #0EA5E9; }
			#parent { color: var(--inherited-color); }
			#explicit { color: inherit; }
		</style>
	</head>
	<body>
		<div id="parent">
			<span id="implicit">Implicit</span>
			<span id="explicit">Explicit</span>
			<div id="middle"><span id="deep">Deep</span></div>
		</div>
	</body>
</html>
    )html" );
	EXPECT_TRUE( root != nullptr );

	UITextSpan* implicit = root->querySelector( "#implicit" )->asType<UITextSpan>();
	EXPECT_TRUE( implicit != nullptr );
	UITextSpan* explicitInherit = root->querySelector( "#explicit" )->asType<UITextSpan>();
	EXPECT_TRUE( explicitInherit != nullptr );
	UIRichText* middle = root->querySelector( "#middle" )->asType<UIRichText>();
	EXPECT_TRUE( middle != nullptr );
	UITextSpan* deep = root->querySelector( "#deep" )->asType<UITextSpan>();
	EXPECT_TRUE( deep != nullptr );

	EXPECT_TRUE( Color( "#0EA5E9" ) == implicit->getFontColor() );
	EXPECT_TRUE( Color( "#0EA5E9" ) == explicitInherit->getFontColor() );
	EXPECT_TRUE( Color( "#0EA5E9" ) == middle->getFontColor() );
	EXPECT_TRUE( Color( "#0EA5E9" ) == deep->getFontColor() );
}

UTEST( CSSVariables, ParentStateChildOpacityReverts ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Parent State Child Opacity Test",
						WindowStyle::Default, WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIWidget* root = app.getUI()->loadLayoutFromString( R"html(
<vbox id="root">
	<style>
		#parent > #child {
			opacity: 0;
		}
		#parent:selected > #child {
			opacity: 1;
		}
	</style>
	<div id="parent">
		<div id="child"></div>
	</div>
</vbox>
    )html" );
	EXPECT_TRUE( root != nullptr );

	UIWidget* parent = root->querySelector( "#parent" );
	EXPECT_TRUE( parent != nullptr );
	UIWidget* child = root->querySelector( "#child" );
	EXPECT_TRUE( child != nullptr );

	EXPECT_EQ( child->getAlpha(), 0.f );
	parent->pushState( UIState::StateSelected );
	EXPECT_EQ( child->getAlpha(), 255.f );
	parent->popState( UIState::StateSelected );
	EXPECT_EQ( child->getAlpha(), 0.f );
}

UTEST( CSSVariables, ParentStateChildOpacityTransitionReverts ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Parent State Child Opacity Transition Test",
						WindowStyle::Default, WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UISceneNode* sceneNode = app.getUI();
	UIWidget* root = sceneNode->loadLayoutFromString( R"html(
<vbox id="root">
	<style>
		#parent > #child {
			opacity: 0;
			transition: all 0.15s;
		}
		#parent:selected > #child {
			opacity: 1;
		}
	</style>
	<div id="parent">
		<div id="child"></div>
	</div>
</vbox>
    )html" );
	EXPECT_TRUE( root != nullptr );

	UIWidget* parent = root->querySelector( "#parent" );
	EXPECT_TRUE( parent != nullptr );
	UIWidget* child = root->querySelector( "#child" );
	EXPECT_TRUE( child != nullptr );

	EXPECT_EQ( child->getAlpha(), 0.f );
	parent->pushState( UIState::StateSelected );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( child->getAlpha(), 255.f );
	parent->popState( UIState::StateSelected );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( child->getAlpha(), 0.f );
}

UTEST( CSSVariables, ParentHoverChildOpacityReverts ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Parent Hover Child Opacity Test",
						WindowStyle::Default, WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UISceneNode* sceneNode = app.getUI();
	UIWidget* root = sceneNode->loadLayoutFromString( R"html(
<vbox id="root">
	<style>
		#parent {
			width: 100dp;
			height: 40dp;
		}
		#child {
			width: 20dp;
			height: 20dp;
		}
		#parent > #child {
			opacity: 0;
			transition: all 0.15s;
		}
		#parent:hover > #child {
			opacity: 1;
		}
	</style>
	<div id="parent">
		<div id="child"></div>
	</div>
</vbox>
    )html" );
	EXPECT_TRUE( root != nullptr );

	UIWidget* parent = root->querySelector( "#parent" );
	EXPECT_TRUE( parent != nullptr );
	UIWidget* child = root->querySelector( "#child" );
	EXPECT_TRUE( child != nullptr );

	sceneNode->update( Time::Zero );
	EXPECT_EQ( child->getAlpha(), 0.f );

	Vector2f hoverPos = parent->convertToWorldSpace( { 1.f, 1.f } );
	EXPECT_EQ( sceneNode->overFind( hoverPos ), child );
	sceneNode->getEventDispatcher()->getInput()->setMousePos( hoverPos.asInt() );
	sceneNode->update( Time::Zero );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( child->getAlpha(), 255.f );

	Vector2f leavePos = parent->convertToWorldSpace(
		{ parent->getPixelsSize().getWidth() + 10.f, parent->getPixelsSize().getHeight() + 10.f } );
	sceneNode->getEventDispatcher()->getInput()->setMousePos( leavePos.asInt() );
	sceneNode->update( Time::Zero );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( child->getAlpha(), 0.f );
}

UTEST( CSSVariables, TabWidgetCloseOpacityRevertsOnHoverLeave ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS TabWidget Close Opacity Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UISceneNode* sceneNode = app.getUI();
	UIWidget* container = UIWidget::New();
	container->addClass( "tab_widget_cont" );
	container->setPixelsSize( 400, 120 );
	container->setParent( sceneNode->getRoot() );

	sceneNode->setStyleSheet( R"css(
		:root {
			--tab-close: #909396;
			--tab-close-hover: #863d47;
		}
		Tab::close {
			width: 10dp;
			height: 10dp;
			foreground-image: url("data:image/svg,<svg width='16' height='16' viewBox='0 0 16 16'><path fill='#ffffff' d='M2 2h12v12H2z' /></svg>");
			foreground-tint: var(--tab-close);
			foreground-size: 10dp 10dp;
			margin-right: 4dp;
			transition: all 0.15s;
		}
		Tab::close:hover {
			foreground-tint: var(--tab-close-hover);
		}
		TabWidget {
			tab-height: 24dp;
		}
		Tab {
			width: 120dp;
			height: 24dp;
		}
		Tab::Text {
			height: 24dp;
		}
		.tab_widget_cont TabWidget {
			max-tab-width: 200dp;
		}
		.tab_widget_cont Tab > Tab::Text {
			text-overflow: ellipsis;
		}
		.tab_widget_cont Tab > Tab::close {
			opacity: 0;
		}
		.tab_widget_cont Tab:selected > Tab::close,
		.tab_widget_cont Tab:hover > Tab::close {
			opacity: 1;
		}
	)css" );

	UITabWidget* tabWidget = UITabWidget::New();
	tabWidget->setPixelsSize( 300, 80 );
	tabWidget->setTabsClosable( true );
	tabWidget->setParent( container );

	UIWidget* ownedWidget0 = UIWidget::New();
	ownedWidget0->setPixelsSize( 300, 60 );
	UITab* tab = tabWidget->add( "Test", ownedWidget0 );
	EXPECT_TRUE( tab != nullptr );
	UIWidget* close = tab->getCloseButton();
	EXPECT_TRUE( close != nullptr );
	EXPECT_TRUE( close->isVisible() );

	UIWidget* ownedWidget1 = UIWidget::New();
	ownedWidget1->setPixelsSize( 300, 60 );
	UITab* selectedTab = tabWidget->add( "Selected", ownedWidget1 );
	EXPECT_TRUE( selectedTab != nullptr );
	tabWidget->setTabSelected( selectedTab );

	sceneNode->update( Time::Zero );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( close->getAlpha(), 0.f );

	Vector2f hoverPos =
		tab->convertToWorldSpace( { 4.f, tab->getPixelsSize().getHeight() * 0.5f } );
	Node* hitNode = sceneNode->overFind( hoverPos );
	EXPECT_NE( hitNode, close );
	EXPECT_EQ( hitNode, tab );
	sceneNode->getEventDispatcher()->getInput()->setMousePos( hoverPos.asInt() );
	sceneNode->update( Time::Zero );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( close->getAlpha(), 255.f );

	Vector2f leavePos = tab->convertToWorldSpace(
		{ tab->getPixelsSize().getWidth() + 10.f, tab->getPixelsSize().getHeight() + 10.f } );
	sceneNode->getEventDispatcher()->getInput()->setMousePos( leavePos.asInt() );
	sceneNode->update( Time::Zero );
	sceneNode->update( Seconds( 0.2f ) );
	EXPECT_EQ( close->getAlpha(), 0.f );
}

UTEST( CSSVariables, DropDownForegroundTintVarTransitionKeepsImageStable ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS DropDown Tint Var Transition Test",
						WindowStyle::Default, WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UISceneNode* sceneNode = app.getUI();
	sceneNode->setStyleSheet( R"css(
		:root {
			--icon: #b6bbc2;
			--icon-active: #ffffff;
		}
		DropDownList {
			width: 160dp;
			height: 24dp;
			transition: all 0.125s;
			foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M12 15.6315L20.9679 10.8838L20.0321 9.11619L12 13.3685L3.96788 9.11619L3.0321 10.8838L12 15.6315Z'></path></svg>");
			foreground-position-x: right 6dp;
			foreground-position-y: center 1dp;
			foreground-size: 12dp 16dp;
			foreground-tint: var(--icon);
		}
		DropDownList:hover {
			foreground-tint: var(--icon-active);
		}
	)css" );

	UIDropDownList* dropdown = UIDropDownList::New();
	dropdown->setPixelsSize( 160, 24 );
	dropdown->setParent( sceneNode->getRoot() );

	const std::string iconColor = Color( "#b6bbc2" ).toHexString();
	const std::string activeIconColor = Color( "#ffffff" ).toHexString();

	sceneNode->update( Time::Zero );
	EXPECT_TRUE( dropdown->getForeground() != nullptr );
	std::string currentTint = dropdown->getForegroundTint( 0 ).toHexString();
	EXPECT_STREQ( currentTint.c_str(), iconColor.c_str() );
	const std::string foregroundImage = dropdown->getForeground()->getLayer( 0 )->getDrawableRef();
	EXPECT_FALSE( foregroundImage.empty() );

	Vector2f hoverPos = dropdown->convertToWorldSpace( { 4.f, 4.f } );
	EXPECT_EQ( sceneNode->overFind( hoverPos ), dropdown );
	sceneNode->getEventDispatcher()->getInput()->setMousePos( hoverPos.asInt() );
	sceneNode->update( Time::Zero );

	const PropertyDefinition* foregroundTint =
		StyleSheetSpecification::instance()->getProperty( PropertyId::ForegroundTint );
	auto transitions = dropdown->getActionsByTag( foregroundTint->getId() );
	EXPECT_EQ( transitions.size(), 1UL );
	auto* transition = static_cast<StyleSheetPropertyAnimation*>( transitions.front() );
	EXPECT_FALSE( String::startsWith( transition->getStartValue(), "var(" ) );
	EXPECT_FALSE( String::startsWith( transition->getEndValue(), "var(" ) );
	EXPECT_STREQ( transition->getStartValue().c_str(), iconColor.c_str() );
	EXPECT_STREQ( transition->getEndValue().c_str(), activeIconColor.c_str() );
	std::string currentForegroundImage = dropdown->getForeground()->getLayer( 0 )->getDrawableRef();
	EXPECT_STREQ( currentForegroundImage.c_str(), foregroundImage.c_str() );

	sceneNode->update( Seconds( 0.2f ) );
	currentTint = dropdown->getForegroundTint( 0 ).toHexString();
	EXPECT_STREQ( currentTint.c_str(), activeIconColor.c_str() );
	currentForegroundImage = dropdown->getForeground()->getLayer( 0 )->getDrawableRef();
	EXPECT_STREQ( currentForegroundImage.c_str(), foregroundImage.c_str() );

	Vector2f leavePos =
		dropdown->convertToWorldSpace( { dropdown->getPixelsSize().getWidth() + 10.f,
										 dropdown->getPixelsSize().getHeight() + 10.f } );
	sceneNode->getEventDispatcher()->getInput()->setMousePos( leavePos.asInt() );
	sceneNode->update( Time::Zero );
	sceneNode->update( Seconds( 0.2f ) );
	currentTint = dropdown->getForegroundTint( 0 ).toHexString();
	EXPECT_STREQ( currentTint.c_str(), iconColor.c_str() );
	currentForegroundImage = dropdown->getForeground()->getLayer( 0 )->getDrawableRef();
	EXPECT_STREQ( currentForegroundImage.c_str(), foregroundImage.c_str() );
}

UTEST( CSSVariables, StyleAttributeVarResolvesAfterLateStylesheet ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Late Style Attribute Var Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIWidget* root = app.getUI()->loadLayoutFromString( R"html(
<html>
	<body>
		<pre id="code" style="background-color: var(--late-bg); color: var(--late-fg);">
			<code><span id="keyword" style="color:var(--late-keyword)">async</span></code>
		</pre>
	</body>
</html>
    )html" );
	EXPECT_TRUE( root != nullptr );

	UIRichText* code = root->querySelector( "#code" )->asType<UIRichText>();
	EXPECT_TRUE( code != nullptr );
	UITextSpan* keyword = root->querySelector( "#keyword" )->asType<UITextSpan>();
	EXPECT_TRUE( keyword != nullptr );

	app.getUI()->combineStyleSheet( R"css(
		:root {
			--late-bg: #1a1714;
			--late-fg: #F0EEEC;
			--late-keyword: #F59E0B;
		}
	)css" );

	EXPECT_TRUE( Color( "#1a1714" ) == code->getBackgroundColor() );
	EXPECT_TRUE( Color( "#F0EEEC" ) == code->getFontColor() );
	EXPECT_TRUE( Color( "#F59E0B" ) == keyword->getFontColor() );
}

UTEST( CSSVariables, StyleAttributeVarUpdatesAfterLaterStylesheetOverride ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Style Attribute Var Override Test",
						WindowStyle::Default, WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIWidget* root = app.getUI()->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( R"html(
<html>
	<head>
		<style>:root{--inline-color:#13579B}</style>
	</head>
	<body>
		<span id="child" style="color: var(--inline-color)">Child</span>
	</body>
</html>
    )html" ) );
	EXPECT_TRUE( root != nullptr );

	UITextSpan* child = root->querySelector( "#child" )->asType<UITextSpan>();
	EXPECT_TRUE( child != nullptr );
	EXPECT_TRUE( Color( "#13579B" ) == child->getFontColor() );

	app.getUI()->combineStyleSheet( ":root{--inline-color:#F59E0B}" );

	EXPECT_TRUE( Color( "#F59E0B" ) == child->getFontColor() );
}

UTEST( CSSVariables, VariableReferencesChain ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Var Ref Test 2", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			--a: #00FF00;
			--b: var(--a);
			--c: var(--b);
			color: var(--c);
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	EXPECT_TRUE( Color( "#00FF00" ) == div->getFontColor() );
}

UTEST( CSSVariables, VariableReferencesWithPadding ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Var Ref Test 3", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		#testdiv {
			--base: 20px;
			--spacing: var(--base);
			padding: var(--spacing);
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIWidget* div = root->querySelector( "#testdiv" );
	EXPECT_TRUE( div != nullptr );

	Float padding = PixelDensity::dpToPx( div->getPadding().Left );
	EXPECT_NEAR( 20.f, padding, 1.f );
}

UTEST( CSSVariables, VariableReferencesMultiple ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Var Ref Test 4", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		#testdiv {
			--color1: #0000FF;
			--color2: var(--color1);
			--color3: var(--color2);
			color: var(--color3);
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	EXPECT_TRUE( Color( "#0000FF" ) == div->getFontColor() );
}

UTEST( CSSVariables, VariableReferencesCircular ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - CSS Var Ref Test 5", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		body {
			--x: var(--y);
			--y: var(--x);
			color: var(--x, #FF0000);
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );
}

UTEST( CSSVariables, VarInRgbMixedSyntax ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Var In Rgb Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	// Test rgb(var(--ch) / var(--alpha)) where the variable holds
	// comma-separated channel values. After substitution this becomes
	// rgb(255, 0, 0 / 0.5) — mixed comma + slash syntax.
	std::string xml = R"html(
<html>
	<head>
		<style>
		#testdiv {
			--ch: 255, 0, 0;
			--alpha: 0.5;
			color: rgb(var(--ch) / var(--alpha));
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	Color result = div->getFontColor();
	EXPECT_EQ( 255, result.r );
	EXPECT_EQ( 0, result.g );
	EXPECT_EQ( 0, result.b );
	EXPECT_EQ( 128, result.a );
}

UTEST( CSSVariables, VarInRgbMixedSyntaxBackground ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Var In Rgb Bg Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	std::string xml = R"html(
<html>
	<head>
		<style>
		#testdiv {
			--ch: 0, 128, 255;
			--alpha: 0.75;
			background-color: rgb(var(--ch) / var(--alpha));
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	Color result = div->getBackgroundColor();
	EXPECT_EQ( 0, result.r );
	EXPECT_EQ( 128, result.g );
	EXPECT_EQ( 255, result.b );
	EXPECT_EQ( 191, result.a );
}

UTEST( CSSVariables, VarInRgbModernSpaceSyntax ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Var In Rgb Space Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	// Space-separated channel values: after substitution the result is
	// rgb(255 128 0 / 0.8) which maps to the modern syntax path.
	std::string xml = R"html(
<html>
	<head>
		<style>
		#testdiv {
			--ch: 255 128 0;
			--alpha: 0.8;
			background-color: rgb(var(--ch) / var(--alpha));
		}
		</style>
	</head>
<body>
	<div id="testdiv">Test text</div>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	UIRichText* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );

	Color result = div->getBackgroundColor();
	EXPECT_EQ( 255, result.r );
	EXPECT_EQ( 128, result.g );
	EXPECT_EQ( 0, result.b );
	EXPECT_EQ( 204, result.a );
}

UTEST( CSSVariables, VarInRgbAnchorColor ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Var In Rgb Anchor Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	// Test that anchor (<a>) elements resolve color with rgb(var()) syntax.
	// This verifies that anchors (which start in Link state) go through the
	// same var resolution pipeline as other elements.
	std::string xml = R"html(
<html>
	<head>
		<style>
		:root {
			--ch: 255, 0, 128;
			--alpha: 0.6;
		}
		a {
			color: rgb(var(--ch) / var(--alpha));
		}
		</style>
	</head>
<body>
	<a id="testlink" href="/">Test link</a>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	auto* link = root->querySelector( "#testlink" );
	EXPECT_TRUE( link != nullptr );

	// Anchor should be a UITextSpan (or UIAnchorSpan which extends it)
	EXPECT_TRUE( link->isType( UI_TYPE_TEXTSPAN ) );

	Color result = link->asType<UITextSpan>()->getFontColor();
	EXPECT_EQ( 255, result.r );
	EXPECT_EQ( 0, result.g );
	EXPECT_EQ( 128, result.b );
	EXPECT_EQ( 153, result.a ); // 0.6 * 255 = 153
}

UTEST( CSSVariables, VarInRgbViaIntermediateVar ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Var In Rgb Indirect Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	// Test variable that references another variable which contains
	// rgb(var()) - requiring recursive var resolution.
	std::string xml = R"html(
<html>
	<head>
		<style>
		:root {
			--r: 0;
			--g: 128;
			--b: 255;
			--a: 0.75;
			--link-color: rgb(var(--r) var(--g) var(--b) / var(--a));
		}
		a {
			color: var(--link-color);
		}
		</style>
	</head>
<body>
	<a id="testlink" href="/">Indirect test link</a>
</body>
</html>
    )html";

	UIWidget* root = app.getUI()->loadLayoutFromString( xml );
	EXPECT_TRUE( root != nullptr );

	auto* link = root->querySelector( "#testlink" );
	EXPECT_TRUE( link != nullptr );

	Color result = link->asType<UITextSpan>()->getFontColor();
	EXPECT_EQ( 0, result.r );
	EXPECT_EQ( 128, result.g );
	EXPECT_EQ( 255, result.b );
	EXPECT_EQ( 191, result.a ); // 0.75 * 255 = 191
}

UTEST( CSSMediaQuery, NegatedFeatureExpression ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Negated Media Feature Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	MediaFeatures features;
	features.prefersColorScheme = "dark";
	features.prefersContrast = "no-preference";
	auto darkNormal =
		MediaQuery::parse( "(prefers-color-scheme: dark) and (not (prefers-contrast: more))" );
	ASSERT_TRUE( darkNormal != nullptr );
	EXPECT_TRUE( darkNormal->check( features ) );

	features.prefersContrast = "more";
	EXPECT_FALSE( darkNormal->check( features ) );

	auto lightHigh =
		MediaQuery::parse( "(not (prefers-color-scheme: dark)) and (prefers-contrast: more)" );
	ASSERT_TRUE( lightHigh != nullptr );
	features.prefersColorScheme = "light";
	EXPECT_TRUE( lightHigh->check( features ) );
	features.prefersColorScheme = "dark";
	EXPECT_FALSE( lightHigh->check( features ) );
}

UTEST( CSSVariables, LightDarkBasic ) {
	// Light scheme: picks first parameter of light-dark()
	UIApplication lightApp(
		WindowSettings( 800, 600, "eepp - LightDark Basic Light", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	lightApp.getUI()->setColorSchemePreference( ColorSchemePreference::Light );
	UIWidget* lightRoot = lightApp.getUI()->loadLayoutFromString( R"html(
<html><head><style>
	#testdiv { color: light-dark(#FF0000, #00FF00); }
</style></head><body>
	<div id="testdiv">Test</div>
</body></html>
    )html" );

	auto* lightDiv = lightRoot->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( lightDiv != nullptr );
	Color lightResult = lightDiv->getFontColor();
	EXPECT_EQ( 255, lightResult.r );
	EXPECT_EQ( 0, lightResult.g );
	EXPECT_EQ( 0, lightResult.b );

	// Dark scheme: picks second parameter
	UIApplication darkApp(
		WindowSettings( 800, 600, "eepp - LightDark Basic Dark", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	darkApp.getUI()->setColorSchemePreference( ColorSchemePreference::Dark );
	UIWidget* darkRoot = darkApp.getUI()->loadLayoutFromString( R"html(
<html><head><style>
	#testdiv { color: light-dark(#FF0000, #00FF00); }
</style></head><body>
	<div id="testdiv">Test</div>
</body></html>
    )html" );

	auto* darkDiv = darkRoot->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( darkDiv != nullptr );
	Color darkResult = darkDiv->getFontColor();
	EXPECT_EQ( 0, darkResult.r );
	EXPECT_EQ( 255, darkResult.g );
	EXPECT_EQ( 0, darkResult.b );
}

UTEST( CSSVariables, LightDarkWithVars ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - LightDark Var Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	app.getUI()->setColorSchemePreference( ColorSchemePreference::Light );
	UIWidget* root = app.getUI()->loadLayoutFromString( R"html(
<html><head><style>
	:root {
		--light: #0000FF;
		--dark: #FF8800;
	}
	#testdiv {
		color: light-dark(var(--light), var(--dark));
	}
</style></head><body>
	<div id="testdiv">Test</div>
</body></html>
    )html" );

	auto* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );
	Color result = div->getFontColor();
	EXPECT_EQ( 0, result.r );
	EXPECT_EQ( 0, result.g );
	EXPECT_EQ( 255, result.b );
}

UTEST( CSSVariables, LightDarkWithVarRgb ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - LightDark Rgb Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	app.getUI()->setColorSchemePreference( ColorSchemePreference::Light );
	UIWidget* root = app.getUI()->loadLayoutFromString( R"html(
<html><head><style>
	:root {
		--l-ch: 255, 0, 0;
		--d-ch: 0, 128, 0;
	}
	#testdiv {
		color: light-dark(rgb(var(--l-ch) / 1), rgb(var(--d-ch) / 1));
	}
</style></head><body>
	<div id="testdiv">Test</div>
</body></html>
    )html" );

	auto* div = root->querySelector( "#testdiv" )->asType<UIRichText>();
	EXPECT_TRUE( div != nullptr );
	Color result = div->getFontColor();
	EXPECT_EQ( 255, result.r );
	EXPECT_EQ( 0, result.g );
	EXPECT_EQ( 0, result.b );
}

UTEST( CSSFunctions, ClampResolvesPixels ) {
	auto len = StyleSheetLength::fromString( "clamp(10px, 50px, 100px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Clamp, len.getUnit() );
	EXPECT_EQ( 3u, (size_t)len.getArgs().size() );
	EXPECT_STREQ( "10px", len.getArgs()[0].c_str() );
	EXPECT_STREQ( "50px", len.getArgs()[1].c_str() );
	EXPECT_STREQ( "100px", len.getArgs()[2].c_str() );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSLength, ScalarParsing ) {
	auto expectLength = [&]( const std::string& value, Float expectedValue,
							 StyleSheetLength::Unit expectedUnit ) {
		const auto length = StyleSheetLength::fromString( value );
		EXPECT_NEAR( expectedValue, length.getValue(), 0.0001f );
		EXPECT_EQ( expectedUnit, length.getUnit() );
	};

	expectLength( "12px", 12, StyleSheetLength::Unit::Px );
	expectLength( "-1.5em", -1.5f, StyleSheetLength::Unit::Em );
	expectLength( "+24dp", 24, StyleSheetLength::Unit::Dp );
	expectLength( ".75rem", 0.75f, StyleSheetLength::Unit::Rem );
	expectLength( "1e3px", 1000, StyleSheetLength::Unit::Px );
	expectLength( " 50% ", 50, StyleSheetLength::Unit::Percentage );
	expectLength( " center ", 50, StyleSheetLength::Unit::Percentage );
	expectLength( "left", 0, StyleSheetLength::Unit::Percentage );
	expectLength( "bottom", 100, StyleSheetLength::Unit::Percentage );
	expectLength( "10", 10, StyleSheetLength::Unit::Dp );
	expectLength( "10unknown", 10, StyleSheetLength::Unit::Dp );
	const auto invalid = StyleSheetLength::fromString( "invalid", 7 );
	EXPECT_EQ( 7, invalid.getValue() );
	EXPECT_EQ( StyleSheetLength::Unit::Px, invalid.getUnit() );

	const auto pxAsDp = StyleSheetLength::fromString( "12px", 0, true );
	EXPECT_EQ( StyleSheetLength::Unit::Dp, pxAsDp.getUnit() );
}

UTEST( CSSFunctions, ClampHonorsMinimum ) {
	auto len = StyleSheetLength::fromString( "clamp(40px, 30px, 100px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 40, resolved );
}

UTEST( CSSFunctions, ClampHonorsMaximum ) {
	auto len = StyleSheetLength::fromString( "clamp(10px, 150px, 100px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 100, resolved );
}

UTEST( CSSFunctions, ClampWithPercentage ) {
	auto len = StyleSheetLength::fromString( "clamp(10px, 50%, 100px)" );
	Float resolved = len.asPixels( 200, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 100, resolved );
}

UTEST( CSSFunctions, MinBasic ) {
	auto len = StyleSheetLength::fromString( "min(100px, 50px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Min, len.getUnit() );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, MinThreeArgs ) {
	auto len = StyleSheetLength::fromString( "min(100px, 50px, 75px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, MaxBasic ) {
	auto len = StyleSheetLength::fromString( "max(10px, 50px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Max, len.getUnit() );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, CaseInsensitive ) {
	auto len = StyleSheetLength::fromString( "CLAMP(10px, 50px, 100px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Clamp, len.getUnit() );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, WhitespaceTolerance ) {
	auto len = StyleSheetLength::fromString( "clamp( 10px , 50% , 100px )" );
	EXPECT_EQ( 3u, (size_t)len.getArgs().size() );
	EXPECT_STREQ( "10px", len.getArgs()[0].c_str() );
	EXPECT_STREQ( "50%", len.getArgs()[1].c_str() );
	EXPECT_STREQ( "100px", len.getArgs()[2].c_str() );
}

UTEST( CSSFunctions, NestedClampInMin ) {
	auto len = StyleSheetLength::fromString( "min(clamp(10px, 50px, 100px), 200px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Min, len.getUnit() );
	EXPECT_EQ( 2u, (size_t)len.getArgs().size() );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, IsLengthRecognizesFunctions ) {
	EXPECT_TRUE( StyleSheetLength::isLength( "clamp(10px, 50%, 100px)" ) );
	EXPECT_TRUE( StyleSheetLength::isLength( "min(10px, 50px)" ) );
	EXPECT_TRUE( StyleSheetLength::isLength( "max(10px, 50px)" ) );
	EXPECT_TRUE( StyleSheetLength::isLength( "calc(10px + 20px)" ) );
}

UTEST( CSSFunctions, CalcBasicAddition ) {
	auto len = StyleSheetLength::fromString( "calc(10px + 20px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Calc, len.getUnit() );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 30, resolved );
}

UTEST( CSSFunctions, CalcSubtraction ) {
	auto len = StyleSheetLength::fromString( "calc(100px - 30px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 70, resolved );
}

UTEST( CSSFunctions, CalcMultiplication ) {
	auto len = StyleSheetLength::fromString( "calc(10px * 5)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, CalcDivision ) {
	auto len = StyleSheetLength::fromString( "calc(100px / 4)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 25, resolved );
}

UTEST( CSSFunctions, CalcOperatorPrecedence ) {
	auto len = StyleSheetLength::fromString( "calc(10px + 20px * 3)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 70, resolved );
}

UTEST( CSSFunctions, CalcParentheses ) {
	auto len = StyleSheetLength::fromString( "calc((10px + 20px) * 3)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 90, resolved );
}

UTEST( CSSFunctions, CalcWithPercentage ) {
	auto len = StyleSheetLength::fromString( "calc(50% + 10px)" );
	Float resolved = len.asPixels( 200, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 110, resolved );
}

UTEST( CSSFunctions, CalcWithEm ) {
	auto len = StyleSheetLength::fromString( "calc(2em + 10px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 16, 16, nullptr );
	EXPECT_EQ( 42, resolved );
}

UTEST( CSSFunctions, CalcNegativeValues ) {
	auto len = StyleSheetLength::fromString( "calc(10px + -5px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 5, resolved );
}

UTEST( CSSFunctions, CalcUnaryMinus ) {
	auto len = StyleSheetLength::fromString( "calc(-10px + 30px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 20, resolved );
}

UTEST( CSSFunctions, CalcNestedInClamp ) {
	auto len = StyleSheetLength::fromString( "clamp(10px, calc(50% + 20px), 200px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Clamp, len.getUnit() );
	Float resolved = len.asPixels( 100, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 70, resolved );
}

UTEST( CSSFunctions, CalcNestedInMin ) {
	auto len = StyleSheetLength::fromString( "min(calc(100px - 20px), 50px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, CalcComplexExpression ) {
	auto len = StyleSheetLength::fromString( "calc((100px + 20px) / 2 - 10px)" );
	Float resolved = len.asPixels( 0, Sizef::Zero, 96, 12, 12, nullptr );
	EXPECT_EQ( 50, resolved );
}

UTEST( CSSFunctions, NotAFunctionReturnsDefault ) {
	auto len = StyleSheetLength::fromString( "notaclamp(10px, 50px, 100px)" );
	EXPECT_EQ( StyleSheetLength::Unit::Px, len.getUnit() );
	EXPECT_EQ( 0, len.getValue() );
}
