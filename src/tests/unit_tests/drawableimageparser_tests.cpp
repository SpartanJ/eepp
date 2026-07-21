#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/lineargradientdrawable.hpp>
#include <eepp/ui/radialgradientdrawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::CSS;

static UISceneNode* initDrawableParserTest() {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
	return sceneNode;
}

static UINode* createTestNode( UISceneNode* sceneNode ) {
	UIWidget* widget = UIWidget::New();
	widget->setParent( sceneNode->getRoot() );
	return widget;
}

UTEST( DrawableImageParser, TwoStopsWithPosition ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(#f5eedd 0%, #ebe0c2 100%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	ASSERT_TRUE( ownIt );
	ASSERT_EQ( drawable->getDrawableType(), Drawable::LINEARGRADIENT );

	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)2 );
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_EQ( stops[1].value, 100.f );
	Color expect05 = Color::fromString( "#f5eedd" );
	Color expectEbe = Color::fromString( "#ebe0c2" );
	EXPECT_TRUE( stops[0].color == expect05 );
	EXPECT_TRUE( stops[1].color == expectEbe );
	EXPECT_EQ( grad->getAngle(), 180.f ); /* default: to bottom */

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, DirectionToRight ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(to right, red, blue)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_EQ( grad->getAngle(), 90.f );
	EXPECT_EQ( grad->getColorStops().size(), (size_t)2 );
	EXPECT_TRUE( grad->getColorStops()[0].color == Color::fromString( "red" ) );
	EXPECT_TRUE( grad->getColorStops()[1].color == Color::fromString( "blue" ) );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, AngleDegrees ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(45deg, red 0%, blue 100%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_EQ( grad->getAngle(), 45.f );
	EXPECT_EQ( grad->getColorStops().size(), (size_t)2 );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, AngleTurn ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(0.25turn, red, blue)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_EQ( grad->getAngle(), 90.f ); /* 0.25 * 360 = 90 */
	EXPECT_EQ( grad->getColorStops().size(), (size_t)2 );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, ThreeStops ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(red 0%, green 50%, blue 100%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)3 );
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_TRUE( stops[0].color == Color::fromString( "red" ) );
	EXPECT_TRUE( stops[1].color == Color::fromString( "green" ) );
	EXPECT_TRUE( stops[2].color == Color::fromString( "blue" ) );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, ColorHint ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(red 0%, 25%, blue 100%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	const auto& stops = grad->getColorStops();

	/* Hint expands to 16 sampled sub-stops + 2 color stops = 18 total
	 * (stop 0 + 15 internal samples + stop 1 = 17, but setColorStops may
	 * re-clamp, so we just check the boundaries and the hint position). */
	ASSERT_GE( stops.size(), (size_t)17 );
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_TRUE( stops[0].color == Color::fromString( "red" ) );
	EXPECT_EQ( stops[stops.size() - 1].value, 100.f );
	EXPECT_TRUE( stops[stops.size() - 1].color == Color::fromString( "blue" ) );

	/* At the hint position 25%, t = 0.25^0.5 = 0.5, so the color should
	 * be ~50% red, 50% blue (rgb ~127, 0, 127). */
	bool foundMidpoint = false;
	for ( const auto& s : stops ) {
		if ( std::abs( s.value - 25.f ) < 2.f ) {
			EXPECT_GT( s.color.r, (Uint8)100 );
			EXPECT_GT( s.color.b, (Uint8)100 );
			foundMidpoint = true;
			break;
		}
	}
	EXPECT_TRUE( foundMidpoint );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, StopsWithoutPositions ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "linear-gradient(red, green, blue)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)3 );
	/* Should be evenly distributed: 0%, 50%, 100% */
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_NEAR( stops[1].value, 50.f, 0.01f );
	EXPECT_EQ( stops[2].value, 100.f );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, EmptyAngleFallsThrough ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	/* No direction specified, should default to 180 (to bottom) */
	Drawable* drawable =
		parser.createDrawable( "linear-gradient(red, blue)", Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_EQ( grad->getAngle(), 180.f );
	EXPECT_EQ( grad->getColorStops().size(), (size_t)2 );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RepeatingTwoStops ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "repeating-linear-gradient(red, blue)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	ASSERT_TRUE( drawable->getDrawableType() == Drawable::REPEATINGLINEARGRADIENT );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_TRUE( grad->isRepeating() );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)2 );
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_EQ( stops[1].value, 100.f );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RepeatingWithAngle ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable(
		"repeating-linear-gradient(45deg, #f00, #0f0, #00f)", Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_EQ( grad->getAngle(), 45.f );
	EXPECT_TRUE( grad->isRepeating() );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)3 );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RepeatingWithPositions ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "repeating-linear-gradient(red 10%, blue 40%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_TRUE( grad->isRepeating() );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)2 );
	EXPECT_EQ( stops[0].value, 10.f );
	EXPECT_EQ( stops[1].value, 40.f );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RadialTwoStops ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable =
		parser.createDrawable( "radial-gradient(red, blue)", Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	ASSERT_TRUE( drawable->getDrawableType() == Drawable::RADIALGRADIENT );
	auto* grad = static_cast<RadialGradientDrawable*>( drawable );
	EXPECT_FALSE( grad->isRepeating() );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)2 );
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_EQ( stops[1].value, 1.f );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RadialCircleKeyword ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "radial-gradient(circle, #f00, #00f)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<RadialGradientDrawable*>( drawable );
	EXPECT_EQ( grad->getShape(), RadialGradientDrawable::CIRCLE );
	EXPECT_EQ( grad->getColorStops().size(), (size_t)2 );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RadialWithPositions ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "radial-gradient(red 10%, blue 80%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<RadialGradientDrawable*>( drawable );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)2 );
	EXPECT_EQ( stops[0].value, 10.f );
	EXPECT_EQ( stops[1].value, 80.f );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, RepeatingRadial ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable( "repeating-radial-gradient(red 10%, blue 40%)",
												Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	ASSERT_TRUE( drawable->getDrawableType() == Drawable::REPEATINGRADIALGRADIENT );
	auto* grad = static_cast<RadialGradientDrawable*>( drawable );
	EXPECT_TRUE( grad->isRepeating() );
	const auto& stops = grad->getColorStops();
	ASSERT_EQ( stops.size(), (size_t)2 );
	EXPECT_EQ( stops[0].value, 10.f );
	EXPECT_EQ( stops[1].value, 40.f );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}

UTEST( DrawableImageParser, TwoLengthStopSyntax ) {
	Engine::instance()->createWindow( WindowSettings( 512, 512, "DrawableImageParser test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	UISceneNode* sceneNode = initDrawableParserTest();
	UINode* node = createTestNode( sceneNode );

	auto& parser = StyleSheetSpecification::instance()->getDrawableImageParser();
	bool ownIt = false;
	Drawable* drawable = parser.createDrawable(
		"repeating-linear-gradient(red 0 10px, blue 10px 20px)", Sizef( 100, 100 ), ownIt, node );

	ASSERT_TRUE( drawable != NULL );
	auto* grad = static_cast<LinearGradientDrawable*>( drawable );
	EXPECT_TRUE( grad->isRepeating() );
	const auto& stops = grad->getColorStops();
	/* red 0 10px → two stops: raw pos 0 and raw pos 10. blue 10px 20px →
	 * two stops: raw pos 10 and raw pos 20. All px positions are stored
	 * as raw pixel values (unit != NORMALIZED=true) and normalized at draw time. */
	ASSERT_EQ( stops.size(), (size_t)4 );
	EXPECT_EQ( stops[0].value, 0.f );
	EXPECT_EQ( stops[0].unit, EE::UI::CSS::StyleSheetLength::Percentage );
	EXPECT_EQ( stops[1].value, 10.f );
	EXPECT_EQ( stops[1].unit, EE::UI::CSS::StyleSheetLength::Px );
	EXPECT_EQ( stops[2].value, 10.f );
	EXPECT_EQ( stops[2].unit, EE::UI::CSS::StyleSheetLength::Px );
	EXPECT_EQ( stops[3].value, 20.f );
	EXPECT_EQ( stops[3].unit, EE::UI::CSS::StyleSheetLength::Px );

	eeSAFE_DELETE( drawable );
	Engine::destroySingleton();
}
