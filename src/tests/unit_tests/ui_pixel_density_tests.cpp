#include "utest.hpp"

#include <atomic>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/network/http.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uihtmlimage.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uisvg.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiviewpager.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwidgettable.hpp>
#include <eepp/window/input.hpp>
#include <limits>
#include <thread>
#include <type_traits>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;
using EE::Window::PixelDensityMode;

namespace {

const char DensityTestSvg[] = "<svg xmlns='http://www.w3.org/2000/svg' width='10' height='6'>"
							  "<rect width='10' height='6' fill='red'/></svg>";

struct RestoreDensity {
	Float previous{ PixelDensity::getPixelDensity() };

	~RestoreDensity() { PixelDensity::setPixelDensity( previous ); }
};

struct DensityEnvironment {
	explicit DensityEnvironment( const char* variable = "EEPP_PIXEL_DENSITY" ) :
		variable( variable ), previous( Sys::getEnv( variable ) ) {}
	const char* variable;
	std::string previous;

	~DensityEnvironment() { Sys::setEnv( variable, previous.c_str() ); }
};

struct DensityApplication {
	RestoreDensity density;
	UIApplication app{
		WindowSettings( 1200, 1000, "Pixel Density Regression", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 2.f ) };
};

Float getPhysicalAutoBackingPPI( EE::Window::Window* window ) {
	const Float panelPPI = window->getPhysicalDisplayDPI();
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( panelPPI > 0.f ) {
		DensityEnvironment rounding( "EEPP_PIXEL_DENSITY_ROUNDING" );
		Sys::setEnv( "EEPP_PIXEL_DENSITY_ROUNDING", "0" );
		const Float reference = window->getPixelDensityReferencePPI();
		window->setPixelDensityReferencePPI( 1.f );
		const Float backingPPI = window->getPixelDensity();
		window->setPixelDensityReferencePPI( reference );
		return backingPPI;
	}
#endif
	return panelPPI;
}

template <typename Base> class PasswordInputProbe : public Base {
  public:
	PasswordInputProbe() : Base() {}

	Text& visibleText() { return this->getVisibleTextCache(); }
};

class PixelSetterWidget : public UIWidget {
  public:
	PixelSetterWidget() : UIWidget( "pixel-setter-test" ) {}
	using UIWidget::setInternalPixelsHeight;
	using UIWidget::setInternalPixelsWidth;

	int paddingChanges{ 0 };
	int marginChanges{ 0 };
	int densityChanges{ 0 };

  protected:
	void onPixelDensityChange( Float oldDensity, Float newDensity ) {
		++densityChanges;
		UIWidget::onPixelDensityChange( oldDensity, newDensity );
	}

	void onPaddingChange() {
		++paddingChanges;
		UIWidget::onPaddingChange();
	}

	void onMarginChange() {
		++marginChanges;
		UIWidget::onMarginChange();
	}
};

class DensityListBox : public UIListBox {
  public:
	void recalculateRowHeight() {
		mRowHeight = 0;
		setRowHeight();
	}
};

class DensityEditor : public UICodeEditor {
  public:
	Float verticalThumb() const { return mVScrollBar->getValue(); }
};

class DensityScene : public UISceneNode {
  public:
	explicit DensityScene( EE::Window::Window* window ) : UISceneNode( window ) {}

	void setTestDPI( Float dpi ) { mDPI = dpi; }
};

class DensityReflowProbe : public UIWidget {
  public:
	DensityReflowProbe() : UIWidget( "density-reflow-probe" ) {}

	std::function<void( UIDensityReflow& )> capture;

  protected:
	virtual void onBeforePixelDensityReflow( UIDensityReflow& reflow ) {
		UIWidget::onBeforePixelDensityReflow( reflow );
		if ( capture )
			capture( reflow );
	}
};

class DensityCreationWidget : public UIWidget {
  public:
	DensityCreationWidget() : UIWidget( "density-creation" ) {}

	UIWidget* victim{ nullptr };
	Node* reparentTo{ nullptr };
	UITextView* created{ nullptr };
	UIWindow* createdWindow{ nullptr };

  protected:
	void onPixelDensityChange( Float oldDensity, Float newDensity ) {
		UIWidget::onPixelDensityChange( oldDensity, newDensity );
		if ( !created ) {
			if ( victim && reparentTo ) {
				victim->setParent( reparentTo );
				victim = nullptr;
			} else {
				eeSAFE_DELETE( victim );
			}
			created = UITextView::New();
			created->setParent( this );
			createdWindow = UIWindow::New();
			createdWindow->setParent( this );
			createdWindow->setSize( 160, 100 );
		}
	}
};

class DensityImage : public UIImage {
  public:
	DensityImage() : UIImage() {}

	Sizef destinationSize() const { return mDestSize; }
};

class DensityWebView : public UIWebView {
  public:
	DensityWebView() : UIWebView() {}
	using UIWebView::getDocumentViewportPixelsSize;
};

class DensityWidgetTable : public UIWidgetTable {
  public:
	void refreshSkinPadding() {
		autoPadding();
		containerResize();
	}
};

} // namespace

UTEST( PixelDensityContext, NestedScopesRestoreFallbackAndConversions ) {
	RestoreDensity restore;
	PixelDensity::setPixelDensity( 1.25f );
	EXPECT_EQ( PixelDensity::dpToPx( 8.f ), 10.f );
	{
		auto outer = PixelDensity::makeCurrent( 2.f );
		EXPECT_EQ( PixelDensity::dpToPx( 8.f ), 16.f );
		EXPECT_EQ( PixelDensity::pxToDp( 16.f ), 8.f );
		EXPECT_TRUE( PixelDensity::dpToPx( Sizef( 8, 12 ) ) == Sizef( 16, 24 ) );
		EXPECT_EQ( PixelDensity::toPxFromString( "8dp" ), 16.f );
		// Changing the fallback must not replace the active thread-local override.
		PixelDensity::setPixelDensity( 1.5f );
		EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
		{
			auto inner = PixelDensity::makeCurrent( 1.75f );
			EXPECT_EQ( PixelDensity::dpToPx( 8.f ), 14.f );
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
		{
			auto integerZero = PixelDensity::makeCurrent( 0 );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 1.5f );
		}
		for ( Float density : { 0.f, -1.f, std::numeric_limits<Float>::infinity(),
								std::numeric_limits<Float>::quiet_NaN() } ) {
			auto fallback = PixelDensity::makeCurrent( density );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 1.5f );
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
	}
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.5f );
}

UTEST( PixelDensityContext, MovingGuardTransfersIdempotentRestoration ) {
	static_assert( !std::is_copy_constructible_v<PixelDensity::Context> );
	static_assert( !std::is_move_assignable_v<PixelDensity::Context> );
	// An active guard also links to the enclosing guard so destruction can snapshot both.
	static_assert( sizeof( PixelDensity::Context ) <= 2 * sizeof( void* ) + 2 * sizeof( Float ) );
	RestoreDensity restore;
	PixelDensity::setPixelDensity( 1.f );
	auto outer = PixelDensity::makeCurrent( 1.25f );
	{
		auto original = PixelDensity::makeCurrent( 2.f );
		{
			auto moved = std::move( original );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
			moved.restore();
			moved.restore();
			EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
	}
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
	outer.restore();
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
}

UTEST( PixelDensityContext, WorkerScopesDoNotLeakAcrossThreads ) {
	RestoreDensity restore;
	PixelDensity::setPixelDensity( 1.25f );
	auto mainContext = PixelDensity::makeCurrent( 2.f );
	Float workerInitial{ 0.f }, workerScoped{ 0.f }, workerRestored{ 0.f };
	std::thread worker( [&] {
		workerInitial = PixelDensity::getPixelDensity();
		{
			auto workerContext = PixelDensity::makeCurrent( 1.75f );
			workerScoped = PixelDensity::dpToPx( 8.f );
		}
		workerRestored = PixelDensity::getPixelDensity();
	} );
	worker.join();
	EXPECT_EQ( workerInitial, 1.25f );
	EXPECT_EQ( workerScoped, 14.f );
	EXPECT_EQ( workerRestored, 1.25f );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
}

UTEST( PixelDensityContext, LiveSourcesTrackChangesAndRestoreNestedSnapshots ) {
	RestoreDensity restore;
	PixelDensity::setPixelDensity( 1.f );
	Float density = 1.25f;
	{
		auto binding = PixelDensity::bindCurrent( &density );
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
		density = 2.f;
		EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
		{
			auto snapshot = PixelDensity::makeCurrent( 1.75f );
			density = 1.5f;
			EXPECT_EQ( PixelDensity::getPixelDensity(), 1.75f );
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.5f );
	}
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
}

UTEST( UISceneDensity, DestroyedSceneBindingsKeepTheirLastDensity ) {
	DensityApplication fixture;
	auto* scene = UISceneNode::New( fixture.app.getWindow() );
	scene->setPixelDensity( 1.5f );
	const Float* source = &scene->getPixelDensity();
	auto context = scene->makeCurrent();
	{
		auto suspended = PixelDensity::makeCurrent( 1.25f );
		auto active = PixelDensity::bindCurrent( source );
		scene->setPixelDensity( 1.75f );
		eeDelete( scene );
		EXPECT_NE( &PixelDensity::getPixelDensity(), source );
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.75f );
		EXPECT_EQ( PixelDensity::dpToPx( 8.f ), 14.f );
	}
	EXPECT_NE( &PixelDensity::getPixelDensity(), source );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.75f );
}

UTEST( PerWindowDensity, DestroyedWindowBindingsKeepTheirLastDensity ) {
	DensityApplication fixture;
	auto* engine = Engine::instance();
	auto* window =
		engine->createWindow( WindowSettings( 320, 240, "Destroyed density binding",
											  WindowStyle::Default | WindowStyle::Hidden,
											  WindowBackend::Default, 32, {}, 1.5f, false, true ),
							  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( window != nullptr );
	engine->setCurrentWindow( fixture.app.getWindow() );
	auto context = engine->makeWindowCurrent( window );
	const Float* source = &window->getPixelDensity();
	window->setPixelDensityOverride( 1.75f );
	engine->destroyWindow( window );
	EXPECT_NE( &PixelDensity::getPixelDensity(), source );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.75f );
}

UTEST( PerWindowDensity, IndependentWindowsAndNestedSceneContexts ) {
	RestoreDensity restore;
	DensityEnvironment environment;
	Sys::setEnv( "EEPP_PIXEL_DENSITY", "" );
	UIApplication app(
		WindowSettings( 320, 240, "Density A", WindowStyle::Default, WindowBackend::Default, 32, {},
						1.f, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
								 std::nullopt, false ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	ASSERT_TRUE( app.getUI() != nullptr );
	auto* sceneA = app.getUI();
	auto* sceneB =
		app.createWindow( WindowSettings( 320, 240, "Density B", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 2.f, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( sceneB != nullptr );
	EXPECT_EQ( sceneA->getPixelDensity(), 1.f );
	EXPECT_EQ( sceneB->getPixelDensity(), 2.f );
	EXPECT_EQ( sceneA->getUIThemeManager()->getDefaultFontSize(), 12.f );
	EXPECT_EQ( sceneB->getUIThemeManager()->getDefaultFontSize(), 24.f );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
	auto* engine = Engine::instance();
	{
		auto contextA = sceneA->makeCurrent();
		auto* widgetA = UIWidget::New();
		widgetA->setSize( 40, 20 );
		EXPECT_EQ( PixelDensity::dpToPx( 10.f ), 10.f );
		{
			auto contextB = sceneB->makeCurrent();
			auto* widgetB = UIWidget::New();
			widgetB->setSize( 40, 20 );
			EXPECT_EQ( widgetB->getUISceneNode(), sceneB );
			EXPECT_TRUE( widgetB->getSize() == widgetA->getSize() );
			EXPECT_TRUE( widgetB->getPixelsSize() == Sizef( 80, 40 ) );
			EXPECT_TRUE( widgetA->getPixelsSize() == Sizef( 40, 20 ) );
			EXPECT_EQ( PixelDensity::dpToPx( 10.f ), 20.f );
			EXPECT_EQ( engine->getCurrentWindow(), sceneB->getWindow() );
			EXPECT_EQ( sceneA->getMediaFeatures().pixelDensity, 1.f );
			EXPECT_EQ( sceneB->getMediaFeatures().pixelDensity, 2.f );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
			{
				SceneNode plainScene( app.getWindow() );
				auto plainContext = plainScene.makeCurrent();
				EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
				EXPECT_TRUE( SceneManager::instance()->getUISceneNode() == nullptr );
			}
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
			EXPECT_EQ( SceneManager::instance()->getUISceneNode(), sceneB );
			{
				auto fallbackContext = engine->makeWindowCurrent( nullptr );
				EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
				EXPECT_TRUE( engine->getCurrentWindow() == nullptr );
			}
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
			EXPECT_EQ( engine->getCurrentWindow(), sceneB->getWindow() );
			{
				// A scene constructed inside B must initialize A's metrics before its body runs.
				auto extraScene =
					std::unique_ptr<UISceneNode>( UISceneNode::New( app.getWindow() ) );
				EXPECT_EQ( extraScene->getUIThemeManager()->getDefaultFontSize(), 12.f );
				EXPECT_EQ( SceneManager::instance()->getUISceneNode(), sceneB );
			}
			{
				auto unrelatedDensity = PixelDensity::makeCurrent( 3.f );
				auto implicitScene = std::unique_ptr<UISceneNode>( UISceneNode::New() );
				EXPECT_EQ( implicitScene->getPixelDensity(), 2.f );
				EXPECT_EQ( implicitScene->getUIThemeManager()->getDefaultFontSize(), 24.f );
				EXPECT_EQ( PixelDensity::getPixelDensity(), 3.f );
			}
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
		EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );
		EXPECT_EQ( SceneManager::instance()->getUISceneNode(), sceneA );
		sceneB->update( Time::Zero );
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
		EXPECT_TRUE( widgetA->getPixelsSize() == Sizef( 40, 20 ) );
	}
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
	{
		auto original = engine->makeWindowCurrent( sceneB->getWindow() );
		{
			auto moved = std::move( original );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
		EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );
	}
	{
		auto sceneContext = sceneB->makeCurrent();
		{
			auto moved = std::move( sceneContext );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
		}
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
		EXPECT_EQ( SceneManager::instance()->getUISceneNode(), sceneA );
	}
}

UTEST( PerWindowDensity, ApplicationOverrideAppliesToEveryWindow ) {
	RestoreDensity restore;
	DensityEnvironment environment;
	Sys::setEnv( "EEPP_PIXEL_DENSITY", "1.75" );
	UIApplication app( WindowSettings( 320, 240, "Application Density", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 2.f, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.25f, false ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	EXPECT_EQ( app.getWindow()->getPixelDensity(), 1.25f );
	EXPECT_EQ( app.getWindow()->getPixelDensityOverride(), 1.25f );
	EXPECT_TRUE( app.getWindow()->hasPixelDensityOverride() );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
	auto* secondary = app.createWindow(
		WindowSettings( 320, 240, "Secondary Application Density", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 2.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondary != nullptr );
	EXPECT_EQ( secondary->getPixelDensity(), 1.25f );
	EXPECT_EQ( secondary->getWindow()->getPixelDensityOverride(), 1.25f );
	EXPECT_EQ( secondary->getWindow()->getWindowInfo()->WindowConfig.PixelDensity, 2.f );
	EXPECT_EQ( secondary->getUIThemeManager()->getDefaultFontSize(), 15.f );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
}

UTEST( PerWindowDensity, WindowEnvironmentAndAutomaticPrecedence ) {
	RestoreDensity restore;
	DensityEnvironment environment;
	DensityEnvironment roundingEnvironment( "EEPP_PIXEL_DENSITY_ROUNDING" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY", "1.75" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_ROUNDING", "0" );
	UIApplication app(
		WindowSettings( 320, 240, "Window Density", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1.25f, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
								 std::nullopt, false ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	EXPECT_EQ( app.getWindow()->getPixelDensity(), 1.25f );
	auto* fromEnvironment =
		app.createWindow( WindowSettings( 320, 240, "Environment Density", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 0.f, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( fromEnvironment != nullptr );
	EXPECT_EQ( fromEnvironment->getPixelDensity(), 1.75f );
	EXPECT_EQ( fromEnvironment->getWindow()->getPixelDensityOverride(), 1.75f );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
	for ( const char* invalid : { "", "invalid", "0", "-1", "inf", "nan" } ) {
		Sys::setEnv( "EEPP_PIXEL_DENSITY", invalid );
		auto* automatic =
			app.createWindow( WindowSettings( 160, 120, "Automatic Density", WindowStyle::Default,
											  WindowBackend::Default, 32, {}, 0.f, false, true ),
							  ContextSettings( false, 0, 0, GLv_default, true, false ) );
		ASSERT_TRUE( automatic != nullptr );
		EXPECT_FALSE( automatic->getWindow()->hasPixelDensityOverride() );
		EXPECT_EQ( automatic->getPixelDensity(), automatic->getWindow()->getDisplayScale() );
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.25f );
	}
}

UTEST( PerWindowDensity, AutomaticSourcesCanChangeLiveAndOverridesKeepPriority ) {
	RestoreDensity restore;
	DensityEnvironment densityEnvironment;
	DensityEnvironment modeEnvironment( "EEPP_PIXEL_DENSITY_MODE" );
	DensityEnvironment referenceEnvironment( "EEPP_PIXEL_DENSITY_REFERENCE_PPI" );
	DensityEnvironment roundingEnvironment( "EEPP_PIXEL_DENSITY_ROUNDING" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY", "" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_MODE", "" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_REFERENCE_PPI", "" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_ROUNDING", "0" );
	WindowSettings settings( 320, 240, "Automatic Density Sources", WindowStyle::Default,
							 WindowBackend::Default, 32, {}, 0, false, true );
	UIApplication app(
		settings,
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 0.f ) );
	auto* window = app.getWindow();
	auto* scene = app.getUI();
	EXPECT_TRUE( window->getPixelDensityMode() == PixelDensityMode::DisplayScale );
	EXPECT_EQ( window->getPixelDensity(), window->getDisplayScale() );
	window->setPixelDensityMode( PixelDensityMode::PhysicalPPI );
	scene->update( Time::Zero );
	const Float physicalDPI = getPhysicalAutoBackingPPI( window );
	const Float physicalDensity =
		physicalDPI > 0.f ? eemax( 1.f, physicalDPI / 96.f ) : window->getDisplayScale();
	EXPECT_EQ( window->getPixelDensity(), physicalDensity );
	EXPECT_EQ( scene->getPixelDensity(), physicalDensity );
	window->setPixelDensityReferencePPI( physicalDPI > 0.f ? physicalDPI * 2.f : 96.f );
	scene->update( Time::Zero );
	EXPECT_EQ( scene->getPixelDensity(), physicalDPI > 0.f ? 1.f : window->getDisplayScale() );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_REFERENCE_PPI", "82" );
	window->setPixelDensityReferencePPI( 0.f );
	scene->update( Time::Zero );
	EXPECT_EQ( window->getPixelDensityReferencePPI(), 82.f );
	EXPECT_EQ( scene->getPixelDensity(),
			   physicalDPI > 0.f ? eemax( 1.f, physicalDPI / 82.f ) : window->getDisplayScale() );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_REFERENCE_PPI", "invalid" );
	window->setPixelDensityReferencePPI( 0.f );
	EXPECT_EQ( window->getPixelDensityReferencePPI(), 96.f );
	window->setPixelDensityOverride( 1.75f );
	window->setPixelDensityMode( PixelDensityMode::DisplayScale );
	scene->update( Time::Zero );
	EXPECT_EQ( scene->getPixelDensity(), 1.75f );
	window->setPixelDensityOverride( 0.f );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_MODE", "physical-ppi" );
	window->setPixelDensityMode( PixelDensityMode::Default );
	scene->update( Time::Zero );
	EXPECT_TRUE( window->getPixelDensityMode() == PixelDensityMode::PhysicalPPI );
	EXPECT_EQ( scene->getPixelDensity(), physicalDensity );
	window->setPixelDensityMode( PixelDensityMode::DisplayScale );
	scene->update( Time::Zero );
	EXPECT_EQ( scene->getPixelDensity(), window->getDisplayScale() );
}

UTEST( PerWindowDensity, PhysicalAutoRoundsToNearestQuarterUnlessDisabled ) {
	DensityEnvironment overrideEnvironment;
	DensityEnvironment roundingEnvironment( "EEPP_PIXEL_DENSITY_ROUNDING" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY", "" );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_ROUNDING", "" );
	DensityApplication fixture;
	auto* window = fixture.app.getWindow();
	auto* scene = fixture.app.getUI();
	window->setPixelDensityMode( PixelDensityMode::PhysicalPPI );
	window->setPixelDensityOverride( 0.f );
	const Float physicalDPI = getPhysicalAutoBackingPPI( window );
	if ( physicalDPI <= 0.f )
		UTEST_SKIP( "Physical display PPI is unavailable" );
	for ( const auto& [raw, expected] : { std::pair{ 1.41f, 1.5f }, std::pair{ 1.61f, 1.5f },
										  std::pair{ 1.89f, 2.f }, std::pair{ 2.61f, 2.5f } } ) {
		window->setPixelDensityReferencePPI( physicalDPI / raw );
		scene->update( Time::Zero );
		EXPECT_EQ( window->getPixelDensity(), expected );
		EXPECT_EQ( scene->getPixelDensity(), expected );
	}
	window->setPixelDensityOverride( 1.61f );
	EXPECT_EQ( window->getPixelDensity(), 1.61f );
	Sys::setEnv( "EEPP_PIXEL_DENSITY_ROUNDING", "0" );
	window->setPixelDensityOverride( 0.f );
	scene->update( Time::Zero );
	EXPECT_NEAR( scene->getPixelDensity(), 2.61f, 0.0001f );
}

UTEST( PerWindowDensity, UIOverridePreservesWindowSizeAndIniAuto ) {
	RestoreDensity restore;
	DensityEnvironment environment;
	Sys::setEnv( "EEPP_PIXEL_DENSITY", "" );
	UIApplication app( WindowSettings( 320, 240, "UI Override Dimensions", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 0.f, false, true ),
					   UIApplication::Settings(
						   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.75f, false ),
					   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	EXPECT_EQ( app.getWindow()->getPixelDensity(), 1.75f );
	EXPECT_EQ( app.getWindow()->getWindowInfo()->WindowConfig.Width, 320u );
	EXPECT_EQ( app.getWindow()->getWindowInfo()->WindowConfig.Height, 240u );
	const std::string data = "[window]\npixeldensity=auto\nwidth=320\nheight=240\n";
	IniFile ini( reinterpret_cast<const Uint8*>( data.data() ), data.size() );
	auto settings = Engine::instance()->createWindowSettings( &ini, "window" );
	EXPECT_EQ( settings.PixelDensity, 0.f );
	EXPECT_EQ( settings.Width, 320u );
	EXPECT_EQ( settings.Height, 240u );
}

UTEST( PerWindowDensity, RawInputAndResizeUseTargetDensity ) {
	RestoreDensity restore;
	UIApplication app(
		WindowSettings( 320, 240, "Input Density A", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1.f, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
								 std::nullopt, false ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* sceneB =
		app.createWindow( WindowSettings( 320, 240, "Input Density B", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 2.f, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( sceneB != nullptr );
	auto contextA = app.getUI()->makeCurrent();
	unsigned callbacks = 0;
	Float callbackDensity = 0.f;
	const auto callback = sceneB->getWindow()->getInput()->pushCallback( [&]( InputEvent* ) {
		++callbacks;
		callbackDensity = PixelDensity::getPixelDensity();
	} );
	InputEvent resize{};
	resize.Type = InputEvent::VideoResize;
	resize.WinID = sceneB->getWindow()->getWindowID();
	resize.resize.w = 800;
	resize.resize.h = 600;
	EXPECT_TRUE( app.getWindow()->getInput()->pushEvent( resize ) );
	EXPECT_EQ( callbacks, 1u );
	EXPECT_EQ( callbackDensity, 2.f );
	EXPECT_TRUE( sceneB->getSize() == Sizef( 400, 300 ) );
	EXPECT_TRUE( sceneB->getRoot()->getSize() == Sizef( 400, 300 ) );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
	EXPECT_EQ( Engine::instance()->getCurrentWindow(), app.getWindow() );
	EXPECT_EQ( SceneManager::instance()->getUISceneNode(), app.getUI() );
	sceneB->getWindow()->getInput()->popCallback( callback );
}

UTEST( UISceneDensity, RepeatedTransitionsRebuildLogicalAndPreservePhysicalGeometry ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	auto* bridge = Node::New();
	bridge->setParent( scene->getRoot() );
	UIWidget* logical;
	UIWidget* physical;
	{
		auto context = scene->makeCurrent();
		logical = UIWidget::New();
		logical->setParent( bridge );
		logical->setPosition( 8, 12 );
		logical->setSize( 80, 40 );
		logical->setPadding( Rectf( 2, 4, 6, 8 ) );
		logical->setPaddingPixelsLeft( 3.f );
		logical->setLayoutMargin( Rectf( 3, 5, 7, 9 ) );
		logical->setLayoutPixelsMarginRight( 4.f );
		physical = UIWidget::New();
		physical->setParent( bridge );
		physical->setPixelsPosition( 3, 5 );
		physical->setPixelsSize( 60, 30 );
	}
	const Sizef viewport = scene->getPixelsSize();
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		auto context = scene->makeCurrent();
		EXPECT_TRUE( scene->getPixelsSize() == viewport );
		EXPECT_TRUE( scene->getRoot()->getPixelsSize() == viewport );
		EXPECT_NEAR( scene->getRoot()->getSize().x, viewport.x / density, 0.01f );
		EXPECT_TRUE( logical->getSize() == Sizef( 80, 40 ) );
		EXPECT_TRUE( logical->getPixelsSize() == Sizef( 80 * density, 40 * density ) );
		EXPECT_TRUE( logical->getPixelsPosition() == Vector2f( 8 * density, 12 * density ) );
		EXPECT_EQ( logical->getPixelsPadding().Left, 3.f );
		EXPECT_EQ( logical->getPixelsPadding().Top, std::ceil( 4.f * density ) );
		EXPECT_EQ( logical->getLayoutPixelsMargin().Right, 4.f );
		EXPECT_EQ( logical->getLayoutPixelsMargin().Left, std::ceil( 3.f * density ) );
		EXPECT_TRUE( physical->getPixelsSize() == Sizef( 60, 30 ) );
		EXPECT_TRUE( physical->getPixelsPosition() == Vector2f( 3, 5 ) );
	}
}

UTEST( UISceneDensity, AnchorsFollowLogicalEdgesAndHitTestingUsesNewGeometry ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	UIWidget* stretch;
	UIWidget* corner;
	{
		auto context = scene->makeCurrent();
		const Sizef viewport = scene->getRoot()->getSize();
		stretch = UIWidget::New();
		stretch->setParent( scene->getRoot() );
		stretch->setPosition( 10, 15 );
		stretch->setSize( viewport.x - 30, viewport.y - 45 );
		stretch->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
		corner = UIWidget::New();
		corner->setParent( scene->getRoot() );
		corner->setPosition( viewport.x - 80, viewport.y - 70 );
		corner->setSize( 40, 30 );
		corner->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	}
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		auto context = scene->makeCurrent();
		const Sizef viewport = scene->getRoot()->getSize();
		EXPECT_NEAR( stretch->getSize().x, viewport.x - 30.f, 0.01f );
		EXPECT_NEAR( stretch->getSize().y, viewport.y - 45.f, 0.01f );
		EXPECT_NEAR( corner->getPosition().x, viewport.x - 80.f, 0.01f );
		EXPECT_NEAR( corner->getPosition().y, viewport.y - 70.f, 0.01f );
		EXPECT_EQ( scene->overFind( corner->getPixelsPosition() + Vector2f( 10, 10 ) ), corner );
	}
}

UTEST( UISceneDensity, FontsResolveParentFirstAndPreserveCssPixels ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	UITextView* parent;
	UITextView* relative;
	UITextView* pixels;
	UITextView* dp;
	{
		auto context = scene->makeCurrent();
		parent = UITextView::New();
		parent->setId( "font-parent" );
		parent->setParent( scene->getRoot() );
		relative = UITextView::New();
		relative->setId( "relative-font" );
		relative->setParent( parent );
		pixels = UITextView::New();
		pixels->setId( "pixel-font" );
		pixels->setParent( scene->getRoot() );
		dp = UITextView::New();
		dp->setId( "dp-font" );
		dp->setParent( scene->getRoot() );
	}
	scene->setStyleSheet( R"css(
#font-parent { font-size: 16dp; }
#relative-font { font-size: 1.25em; }
#pixel-font { font-size: 14px; }
#dp-font { font-size: 14dp; }
)css" );
	scene->update( Time::Zero );
	const Float sharedDefault = scene->getUIThemeManager()->getDefaultTheme()->getDefaultFontSize();
	auto activeScene = scene->makeCurrent();
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		EXPECT_EQ( PixelDensity::getPixelDensity(), density );
		EXPECT_EQ( parent->getFontSize(), static_cast<Uint32>( 16.f * density ) );
		EXPECT_EQ( relative->getFontSize(), static_cast<Uint32>( 20.f * density ) );
		EXPECT_EQ( pixels->getFontSize(), 14u );
		EXPECT_EQ( dp->getFontSize(), static_cast<Uint32>( 14.f * density ) );
		EXPECT_EQ( scene->getUIThemeManager()->getDefaultTheme()->getDefaultFontSize(),
				   sharedDefault );
	}
}

UTEST( UISceneDensity, SharedThemeDefaultsDoNotDriftOnFractionalRoundTrips ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	auto* child = UISceneNode::New( fixture.app.getWindow() );
	child->setParent( scene->getRoot() );
	child->setPixelDensity( 1.25f );
	UITextView* hostText;
	UITextView* childText;
	{
		auto context = scene->makeCurrent();
		hostText = UITextView::New();
		hostText->setParent( scene->getRoot() );
	}
	{
		auto context = child->makeCurrent();
		childText = UITextView::New();
		childText->setParent( child->getRoot() );
	}
	const Float logicalSize = scene->getUIThemeManager()->getDefaultTheme()->getDefaultFontSizeDp();
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		EXPECT_EQ( hostText->getFontSize(), static_cast<Uint32>( logicalSize * density ) );
		EXPECT_EQ( childText->getFontSize(), static_cast<Uint32>( logicalSize * 1.25f ) );
	}
}

UTEST( UISceneDensity, EmbeddedScenesInheritOrOverrideIndependently ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	UISceneNode* inherited = UISceneNode::New( fixture.app.getWindow() );
	inherited->setParent( host->getRoot() );
	UISceneNode* overridden = UISceneNode::New( fixture.app.getWindow() );
	overridden->setParent( host->getRoot() );
	overridden->setPixelDensity( 1.25f );
	for ( Float density : { 1.f, 2.f, 1.5f } ) {
		host->setPixelDensity( density );
		EXPECT_EQ( inherited->getPixelDensity(), density );
		EXPECT_FALSE( inherited->hasPixelDensityOverride() );
		EXPECT_EQ( overridden->getPixelDensity(), 1.25f );
		EXPECT_TRUE( overridden->hasPixelDensityOverride() );
		{
			auto context = overridden->makeCurrent();
			EXPECT_EQ( PixelDensity::dpToPx( 8.f ), 10.f );
		}
	}
	overridden->clearPixelDensityOverride();
	EXPECT_EQ( overridden->getPixelDensity(), 1.5f );
	host->clearPixelDensityOverride();
	EXPECT_EQ( host->getPixelDensity(), fixture.app.getWindow()->getPixelDensity() );
	EXPECT_EQ( inherited->getPixelDensity(), host->getPixelDensity() );
	EXPECT_EQ( overridden->getPixelDensity(), host->getPixelDensity() );
}

UTEST( UISceneDensity, EmbeddedSceneCanReparentSiblingDuringReflow ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	auto* window = fixture.app.getWindow();
	auto destination = std::unique_ptr<UISceneNode>( UISceneNode::New( window ) );
	auto* first = UISceneNode::New( window );
	auto* moving = UISceneNode::New( window );
	auto* remaining = UISceneNode::New( window );
	first->setParent( host->getRoot() );
	moving->setParent( host->getRoot() );
	remaining->setParent( host->getRoot() );
	auto context = first->makeCurrent();
	auto* probe = eeNew( DensityReflowProbe, () );
	probe->capture = [&]( UIDensityReflow& ) { moving->setParent( destination->getRoot() ); };

	host->setPixelDensity( 1.5f );

	EXPECT_EQ( host->getChildUISceneNodes().size(), 2u );
	EXPECT_EQ( moving->getParent(), destination->getRoot() );
	EXPECT_EQ( moving->getPixelDensity(), 2.f );
	EXPECT_EQ( remaining->getPixelDensity(), 1.5f );
}

UTEST( UISceneDensity, EmbeddedSceneCanDeleteSiblingDuringReflow ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	auto* window = fixture.app.getWindow();
	auto* first = UISceneNode::New( window );
	auto* removed = UISceneNode::New( window );
	first->setParent( host->getRoot() );
	removed->setParent( host->getRoot() );
	auto context = first->makeCurrent();
	auto* probe = eeNew( DensityReflowProbe, () );
	probe->capture = [&]( UIDensityReflow& ) { eeSAFE_DELETE( removed ); };

	host->setPixelDensity( 1.5f );

	EXPECT_TRUE( removed == nullptr );
	ASSERT_EQ( host->getChildUISceneNodes().size(), 1u );
	EXPECT_EQ( host->getChildUISceneNodes().front(), first );
	EXPECT_EQ( first->getPixelDensity(), 1.5f );
}

UTEST( UISceneDensity, EmbeddedSceneCanCreateSiblingDuringReflow ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	auto* window = fixture.app.getWindow();
	auto* first = UISceneNode::New( window );
	auto* second = UISceneNode::New( window );
	first->setParent( host->getRoot() );
	second->setParent( host->getRoot() );
	auto context = first->makeCurrent();
	auto* probe = eeNew( DensityReflowProbe, () );
	UISceneNode* created = nullptr;
	probe->capture = [&]( UIDensityReflow& ) {
		created = UISceneNode::New( window );
		created->setParent( host->getRoot() );
	};

	host->setPixelDensity( 1.5f );

	ASSERT_TRUE( created != nullptr );
	EXPECT_EQ( host->getChildUISceneNodes().size(), 3u );
	EXPECT_EQ( created->getPixelDensity(), 1.5f );
	EXPECT_EQ( second->getPixelDensity(), 1.5f );
}

UTEST( UISceneDensity, DensityRequestedDuringChildPropagationAppliesOnNextUpdate ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	auto* child = UISceneNode::New( fixture.app.getWindow() );
	child->setParent( host->getRoot() );
	auto context = child->makeCurrent();
	auto* probe = eeNew( DensityReflowProbe, () );
	probe->capture = [&]( UIDensityReflow& reflow ) {
		if ( reflow.getNewDensity() == 1.5f )
			host->setPixelDensity( 1.25f );
	};

	host->setPixelDensity( 1.5f );
	EXPECT_EQ( host->getPixelDensity(), 1.5f );
	EXPECT_EQ( child->getPixelDensity(), 1.5f );
	host->update( Time::Zero );
	EXPECT_EQ( host->getPixelDensity(), 1.25f );
	EXPECT_EQ( child->getPixelDensity(), 1.25f );
}

UTEST( UISceneDensity, ReparentedWidgetSubtreeUsesDestinationDensity ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	host->setPixelDensity( 1.f );
	auto context = host->makeCurrent();
	auto* destination = UISceneNode::New( fixture.app.getWindow() );
	destination->setParent( host->getRoot() );
	destination->setPixelDensity( 2.f );
	destination->setStyleSheet( "#moved-css { font-size: 15dp; }" );
	auto* parent = UIWidget::New();
	parent->setSize( 100, 60 );
	auto* physical = UIWidget::New();
	physical->setParent( parent );
	physical->setPixelsSize( 37, 19 );
	auto* logical = UITextView::New();
	logical->setParent( parent );
	logical->setFontSize( 14 );
	auto* styled = UITextView::New();
	styled->setParent( parent );
	styled->setId( "moved-css" );
	parent->setParent( destination->getRoot() );
	destination->update( Time::Zero );
	EXPECT_EQ( parent->getPixelsSize().x, 200.f );
	EXPECT_EQ( physical->getPixelsSize().x, 37.f );
	EXPECT_EQ( logical->getFontSize(), 28u );
	EXPECT_EQ( styled->getFontSize(), 30u );

	parent->setParent( host->getRoot() );
	parent->setParent( destination->getRoot() );
	destination->update( Time::Zero );
	EXPECT_EQ( parent->getPixelsSize().x, 200.f );
	EXPECT_EQ( logical->getFontSize(), 28u );

	// A peer moves this widget before its original scene can repair its geometry.
	auto* mover = eeNew( DensityCreationWidget, () );
	auto* moved = UIWidget::New();
	moved->setSize( 100, 50 );
	mover->victim = moved;
	mover->reparentTo = destination->getRoot();
	host->setPixelDensity( 2.f );
	EXPECT_TRUE( moved->getUISceneNode() == destination );
	EXPECT_EQ( moved->getPixelsSize().x, 200.f );
}

UTEST( UISceneDensity, EmbeddedViewportsFollowReflowedParentBounds ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	host->setStyleSheet( "" );
	auto context = host->makeCurrent();
	auto* parent = UIWidget::New();
	parent->setParent( host->getRoot() );
	parent->setSize( 160, 100 );
	auto* inherited = UISceneNode::New( fixture.app.getWindow() );
	inherited->setParent( parent );
	auto* overridden = UISceneNode::New( fixture.app.getWindow() );
	overridden->setParent( parent );
	overridden->setPixelDensity( 1.25f );
	auto* web = eeNew( DensityWebView, () );
	web->setParent( host->getRoot() );
	web->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	web->setSize( 200, 120 );
	// WebView reconciles document extents and scrollbar visibility in scheduled updates.
	for ( int i = 0; i < 8; ++i )
		host->update( Milliseconds( 16 ) );
	for ( Float density : { 1.f, 2.f, 1.25f } ) {
		host->setPixelDensity( density );
		for ( int i = 0; i < 8; ++i )
			host->update( Milliseconds( 16 ) );
		EXPECT_TRUE( inherited->getPixelsSize() == parent->getPixelsSize() );
		EXPECT_TRUE( overridden->getPixelsSize() == parent->getPixelsSize() );
		EXPECT_EQ( overridden->getPixelDensity(), 1.25f );
		EXPECT_TRUE( web->getDocumentSceneNode()->getViewportPixelsSize() ==
					 web->getDocumentViewportPixelsSize() );
	}
}

UTEST( UISceneDensity, EmbeddedViewportResizeUpdatesMediaWithoutDensityReflow ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	auto* parent = UIWidget::New();
	parent->setSize( 160, 100 );
	auto* child = UISceneNode::New( fixture.app.getWindow() );
	child->setParent( parent );
	child->setPixelDensity( 1.25f );
	auto context = child->makeCurrent();
	auto* probe = eeNew( DensityReflowProbe, () );
	probe->setId( "viewport-probe" );
	int reflows = 0;
	probe->capture = [&]( UIDensityReflow& ) { ++reflows; };
	child->setPixelsSize( 40, 40 );
	child->setStyleSheet( "#viewport-probe { background-color: #ff0000; } "
						  "@media (min-width:100dp) { "
						  "#viewport-probe { background-color: #00ff00; } }" );
	EXPECT_TRUE( probe->getBackgroundColor() == Color::Red );

	child->update( Time::Zero );
	EXPECT_EQ( reflows, 0 );
	EXPECT_TRUE( child->getPixelsSize() == parent->getPixelsSize() );
	EXPECT_EQ( child->getSize().x, 256.f );
	EXPECT_TRUE( probe->getBackgroundColor() == Color( "#00ff00" ) );

	host->setPixelDensity( 1.f );
	EXPECT_EQ( reflows, 0 );
	EXPECT_TRUE( child->getPixelsSize() == parent->getPixelsSize() );
	EXPECT_EQ( child->getSize().x, 128.f );
}

UTEST( UISceneDensity, EmbeddedViewportResizeReappliesViewportLengths ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	host->setPixelDensity( 1.f );
	auto hostContext = host->makeCurrent();
	auto* parent = UIWidget::New();
	parent->setSize( 200.f, 160.f );
	auto* child = UISceneNode::New( fixture.app.getWindow() );
	child->setParent( parent );
	auto context = child->makeCurrent();
	child->setPixelsSize( 100.f, 100.f );
	auto* widget = eeNew( DensityReflowProbe, () );
	widget->setId( "viewport-length" );
	int reflows = 0;
	widget->capture = [&]( UIDensityReflow& ) { ++reflows; };
	child->setStyleSheet( "#viewport-length { width: 50vw; height: 20dp; }" );
	EXPECT_EQ( widget->getPixelsSize().x, 50.f );
	child->update( Time::Zero );
	EXPECT_EQ( widget->getPixelsSize().x, 100.f );
	parent->setPixelsSize( 240.f, 160.f );
	child->update( Time::Zero );
	EXPECT_EQ( widget->getPixelsSize().x, 120.f );
	EXPECT_EQ( reflows, 0 );
}

UTEST( UISceneDensity, InternalPixelSettersPreserveTheOtherLogicalAxis ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.25f );
	auto context = scene->makeCurrent();
	auto* width = eeNew( PixelSetterWidget, () );
	auto* height = eeNew( PixelSetterWidget, () );
	width->setSize( 100.5f, 20.5f );
	height->setSize( 100.5f, 20.5f );
	width->setInternalPixelsWidth( 40.f );
	height->setInternalPixelsHeight( 30.f );
	scene->setPixelDensity( 2.f );
	EXPECT_EQ( width->getPixelsSize().x, 40.f );
	EXPECT_EQ( width->getPixelsSize().y, 41.f );
	EXPECT_EQ( height->getPixelsSize().x, 201.f );
	EXPECT_EQ( height->getPixelsSize().y, 30.f );
}

UTEST( UISceneDensity, WindowViewportMinimumResolvesBeforeDensitySizeEnforcement ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	scene->setStyleSheet( "" );
	auto context = scene->makeCurrent();
	auto* window = UIWindow::New();
	window->setParent( scene->getRoot() );
	window->setStyleSheetProperty(
		StyleSheetProperty( "window-min-size", "min(640dp,90vw) min(440dp,80vh)" ) );
	window->setPixelsSize( 700, 500 );
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		for ( int i = 0; i < 4; ++i )
			scene->update( Milliseconds( 16 ) );
		EXPECT_LE( window->getPixelsSize().x, scene->getPixelsSize().x );
		EXPECT_LE( window->getPixelsSize().y, scene->getPixelsSize().y );
		EXPECT_LE( window->getMinWindowSize().x * density, scene->getPixelsSize().x * 0.9f + 1.f );
	}
}

UTEST( UISceneDensity, SettingsControlsStayReachableWhenLogicalViewportNarrows ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	auto context = scene->makeCurrent();
	auto* panel = UISettingsPanel::New( scene->getRoot() );
	panel->setPixelsSize( scene->getPixelsSize() );
	panel->addCategory( "appearance.fonts", "Appearance", "Fonts & Scale" );
	Float value = 1.f;
	panel->addFloat(
		{ "scale", "appearance.fonts", "UI Scale Factor",
		  "Scale the complete user interface without restarting the application." },
		1, 6, 0.1, [&] { return value; }, [&]( double scale ) { value = scale; } );
	panel->build();
	panel->setFilter( "scale" );
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		for ( int i = 0; i < 8; ++i )
			scene->update( Milliseconds( 16 ) );
		auto* row = panel->find<UIWidget>( "setting_scale" );
		ASSERT_TRUE( row != nullptr );
		auto* control = row->find<UIWidget>( "setting_control_widget" );
		ASSERT_TRUE( control != nullptr );
		EXPECT_TRUE( control->hasVisibility() );
		EXPECT_LE( control->getWorldBounds().Bottom, scene->getPixelsSize().y );
		EXPECT_GT( row->find<UIWidget>( "setting_name" )->getPixelsSize().x, 0.f );
	}
}

UTEST( UISceneDensity, RuntimeVisibilitySurvivesWhileChangedMediaStateApplies ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "#density-media-state { --show:true; visible:var(--show); } "
						  "#density-variable-state { --show:true; visible:var(--show); } "
						  "@media (pixel-density:2) { #density-media-state { --show:false; } }" );
	auto* shown = scene->loadLayoutFromString(
		"<vbox id='density-runtime-shown' visible='false' enabled='false' />" );
	auto* hidden = scene->loadLayoutFromString(
		"<vbox id='density-runtime-hidden' visible='true' enabled='true' />" );
	auto* media = scene->loadLayoutFromString( "<vbox id='density-media-state' />" );
	auto* var = scene->loadLayoutFromString( "<vbox id='density-variable-state' />" );
	scene->update( Time::Zero );
	shown->setVisible( true );
	shown->setEnabled( true );
	hidden->setVisible( false );
	hidden->setEnabled( false );
	var->setVisible( false );
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		EXPECT_TRUE( shown->isVisible() );
		EXPECT_TRUE( shown->isEnabled() );
		EXPECT_FALSE( hidden->isVisible() );
		EXPECT_FALSE( hidden->isEnabled() );
		EXPECT_FALSE( var->isVisible() );
		EXPECT_EQ( media->isVisible(), density != 2.f );
	}
}

UTEST( UISceneDensity, UnchangedKeyframeAnimationKeepsItsAction ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	auto* widget = UIWidget::New();
	widget->setId( "density-animation" );
	widget->setParent( scene->getRoot() );
	scene->setStyleSheet( R"css(
@keyframes density-fade { from { opacity: 0.2; } to { opacity: 1; } }
#density-animation { animation: density-fade 10s linear infinite; }
)css" );
	scene->update( Milliseconds( 500 ) );
	const auto actions = widget->getActions();
	ASSERT_FALSE( actions.empty() );
	scene->setPixelDensity( 1.25f );
	const auto after = widget->getActions();
	ASSERT_EQ( actions.size(), after.size() );
	for ( size_t i = 0; i < actions.size(); ++i )
		EXPECT_EQ( actions[i], after[i] );
}

UTEST( UISceneDensity, SharedMediaQueriesMatchEachSceneIndependently ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	CSS::StyleSheetParser parser;
	ASSERT_TRUE( parser.loadFromString( std::string_view{ R"css(
#media-density { background-color: #ff0000; }
@media (pixel-density: 1.25) { #media-density { background-color: #00ff00; } }
@media (min-width: 700dp) { #media-density { width: 80dp; } }
)css" } ) );
	auto* child = UISceneNode::New( fixture.app.getWindow() );
	child->setParent( scene->getRoot() );
	child->setPixelDensity( 1.25f );
	UIWidget* hostWidget;
	UIWidget* childWidget;
	{
		auto context = scene->makeCurrent();
		hostWidget = UIWidget::New();
		hostWidget->setId( "media-density" );
		hostWidget->setParent( scene->getRoot() );
	}
	{
		auto context = child->makeCurrent();
		childWidget = UIWidget::New();
		childWidget->setId( "media-density" );
		childWidget->setParent( child->getRoot() );
	}
	scene->setStyleSheet( parser.getStyleSheet() );
	child->setStyleSheet( parser.getStyleSheet() );
	scene->update( Time::Zero );
	child->update( Time::Zero );
	EXPECT_TRUE( hostWidget->getBackgroundColor() == Color::Red );
	EXPECT_TRUE( childWidget->getBackgroundColor() == Color( "#00ff00" ) );
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		EXPECT_TRUE( hostWidget->getBackgroundColor() ==
					 ( density == 1.25f ? Color( "#00ff00" ) : Color::Red ) );
		EXPECT_TRUE( childWidget->getBackgroundColor() == Color( "#00ff00" ) );
		child->reloadStyle( true, true, true );
		EXPECT_TRUE( hostWidget->getBackgroundColor() ==
					 ( density == 1.25f ? Color( "#00ff00" ) : Color::Red ) );
	}
}

UTEST( UISceneDensity, StylesheetQueriesExposeTheirOwnActiveRules ) {
	CSS::StyleSheetParser parser;
	ASSERT_TRUE( parser.loadFromString( std::string_view{ R"css(
:root { --scale-marker: standard; }
@media (pixel-density: 2) { :root { --scale-marker: high; } }
)css" } ) );
	CSS::StyleSheet standard;
	standard = parser.getStyleSheet();
	CSS::StyleSheet high;
	high = parser.getStyleSheet();
	CSS::MediaFeatures features{};
	features.type = CSS::media_type_screen;
	features.pixelDensity = 1.f;
	standard.updateMediaLists( features );
	features.pixelDensity = 2.f;
	EXPECT_TRUE( high.updateMediaLists( features ) );
	auto rule = high.getStyleFromSelector( ":root", true );
	ASSERT_TRUE( rule && rule->getMediaQueryList() );
	EXPECT_TRUE( high.isMediaValid( rule->getMediaQueryList() ) );
	EXPECT_FALSE( standard.isMediaValid( rule->getMediaQueryList() ) );
	EXPECT_TRUE( standard.getStyleFromSelector( ":root", true ).get() != rule.get() );
}

UTEST( UISceneDensity, MediaLengthsResolveFromFeaturesWithoutAmbientWindow ) {
	CSS::MediaFeatures features{};
	features.type = CSS::media_type_screen;
	features.width = 600;
	features.height = 400;
	features.pixelDensity = 1.25f;
	features.resolution = 120;
	auto unrelatedDensity = PixelDensity::makeCurrent( 3.f );
	for ( const auto* source :
		  { "(min-width:600dp)", "(min-width:600px)", "(min-width:6.25in)", "(resolution:120dpi)",
			"(resolution:1.25dppx)", "(pixel-density:1.25)" } ) {
		auto query = CSS::MediaQuery::parse( source );
		ASSERT_TRUE( query != nullptr );
		EXPECT_TRUE( query->check( features ) );
	}
	auto excluded = CSS::MediaQuery::parse( "(max-width:599dp)" );
	ASSERT_TRUE( excluded != nullptr );
	EXPECT_FALSE( excluded->check( features ) );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 3.f );
}

UTEST( UISceneDensity, HtmlPixelsFollowTheirLogicalFontContract ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	auto* root = scene->loadLayoutFromString(
		HTMLFormatter::HTMLtoXML( "<html><body><span id='html-density-font' "
								  "style='font-size:14px'>text</span></body></html>" ) );
	ASSERT_TRUE( root != nullptr );
	auto* text = root->querySelector( "#html-density-font" )->asType<UIRichText>();
	ASSERT_TRUE( text != nullptr );
	for ( Float density : { 1.f, 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		EXPECT_EQ( text->getFontSize(), static_cast<Uint32>( 14.f * density ) );
	}
}

UTEST( UISceneDensity, CustomRestoreRunsAfterLayoutAtNewDensity ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	scene->setStyleSheet( R"css(
		#density-spacer { layout-width: 40dp; layout-height: 16dp; }
		@media (pixel-density: 2) { #density-spacer { layout-width: 80dp; } }
	)css" );
	auto context = scene->makeCurrent();
	auto* layout = scene->loadLayoutFromString( R"xml(
		<hbox layout_width="300dp" layout_height="40dp">
			<widget id="density-spacer" />
		</hbox>
	)xml" );
	ASSERT_TRUE( layout != nullptr );
	auto* probe = eeNew( DensityReflowProbe, () );
	probe->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	probe->setSize( 40, 16 );
	probe->setParent( layout );
	scene->update( Time::Zero );
	ASSERT_EQ( probe->getPixelsPosition().x, 40.f );
	int captures = 0;
	int restores = 0;
	probe->capture = [&]( UIDensityReflow& reflow ) {
		++captures;
		EXPECT_EQ( PixelDensity::getPixelDensity(), 1.f );
		EXPECT_EQ( reflow.getOldDensity(), 1.f );
		EXPECT_EQ( reflow.getNewDensity(), 2.f );
		EXPECT_EQ( probe->getPixelsPosition().x, 40.f );
		reflow.restoreAfterLayout( [&] {
			EXPECT_EQ( restores++, 0 );
			EXPECT_EQ( PixelDensity::getPixelDensity(), 2.f );
			EXPECT_EQ( probe->getPixelsPosition().x, 160.f );
		} );
		reflow.restoreAfterLayout( [&]( const UIDensityReflow& restore ) {
			EXPECT_EQ( restores++, 1 );
			EXPECT_TRUE( restore.isWidgetCurrent() );
			EXPECT_EQ( restore.getNewDensity(), 2.f );
		} );
	};
	auto ambientDensity = PixelDensity::makeCurrent( 3.f );
	scene->setPixelDensity( 2.f );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 3.f );
	EXPECT_EQ( captures, 1 );
	EXPECT_EQ( restores, 2 );
}

UTEST( UISceneDensity, CustomRestoreCancelsForDeletedReparentedAndClosingWidgets ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	auto context = scene->makeCurrent();
	auto* destination = UISceneNode::New( fixture.app.getWindow() );
	destination->setParent( scene->getRoot() );
	destination->setPixelDensity( 1.f );
	auto* first = eeNew( DensityReflowProbe, () );
	first->setParent( scene->getRoot() );
	auto* removed = eeNew( DensityReflowProbe, () );
	removed->setParent( scene->getRoot() );
	auto* moved = eeNew( DensityReflowProbe, () );
	moved->setParent( scene->getRoot() );
	int captures = 0;
	int staleRestores = 0;
	const auto capture = [&]( UIDensityReflow& reflow ) {
		++captures;
		reflow.restoreAfterLayout( [&] { ++staleRestores; } );
	};
	removed->capture = capture;
	moved->capture = capture;
	first->capture = [&]( UIDensityReflow& reflow ) {
		reflow.restoreAfterLayout( [&]( const UIDensityReflow& restore ) {
			eeSAFE_DELETE( removed );
			moved->setParent( destination->getRoot() );
			first->close();
			EXPECT_FALSE( restore.isWidgetCurrent() );
		} );
		capture( reflow );
	};
	scene->setPixelDensity( 2.f );
	EXPECT_EQ( captures, 3 );
	EXPECT_EQ( staleRestores, 0 );
	EXPECT_TRUE( removed == nullptr );
	EXPECT_EQ( moved->getUISceneNode(), destination );
}

UTEST( UISceneDensity, ScrollPositionAndWrappedEditorAnchorSurviveReflow ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	UIScrollView* scroll;
	DensityEditor* editor;
	{
		auto context = scene->makeCurrent();
		scroll = UIScrollView::New();
		scroll->setParent( scene->getRoot() );
		scroll->setSize( 200, 160 );
		auto* content = UIWidget::New();
		content->setParent( scroll );
		content->setSize( 400, 2000 );
		editor = eeNew( DensityEditor, () );
		editor->setParent( scene->getRoot() );
		editor->setSize( 300, 180 );
		editor->setLineWrapMode( LineWrapMode::Word );
		String lines;
		for ( int i = 0; i < 200; ++i )
			lines +=
				"A sufficiently long document line with wrapping content and a stable anchor.\n";
		editor->getDocument().textInput( lines );
		scene->update( Time::Zero );
		scroll->getVerticalScrollBar()->setValue( 0.65f );
		editor->setScroll( { 0.f, editor->getLineHeight() * 100.25f }, false );
	}
	const auto anchor =
		editor->getDocumentView().getVisibleIndexPosition( Doc::VisibleIndex( 100 ) );
	for ( Float density : { 1.f, 2.f, 1.25f } ) {
		scene->setPixelDensity( density );
		EXPECT_NEAR( scroll->getVerticalScrollBar()->getValue(), 0.65f, 0.001f );
		const auto topIndex = static_cast<Doc::VisibleIndex>(
			static_cast<Int64>( std::floor( editor->getScroll().y / editor->getLineHeight() ) ) );
		const auto top = editor->getDocumentView().getVisibleIndexPosition( topIndex );
		EXPECT_EQ( top.line(), anchor.line() );
		ASSERT_GT( editor->getMaxScroll().y, 0.f );
		EXPECT_NEAR( editor->verticalThumb(), editor->getScroll().y / editor->getMaxScroll().y,
					 0.001f );
	}
}

UTEST( UISceneDensity, HiddenWrappedEditorWithoutLayoutHasNoScrollAnchor ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	auto context = scene->makeCurrent();
	auto* editor = UICodeEditor::New();
	editor->setParent( scene->getRoot() );
	editor->setPixelsSize( 0, 0 );
	editor->setVisible( false );
	editor->setLineWrapMode( LineWrapMode::Word );
	ASSERT_GT( editor->getLineHeight(), 0.f );
	ASSERT_EQ( editor->getDocumentView().getVisibleLinesCount(), size_t{ 0 } );
	for ( Float density : { 1.5f, 1.f, 2.f } ) {
		scene->setPixelDensity( density );
		EXPECT_TRUE( std::isfinite( editor->getScroll().y ) );
	}
	// The editor can still construct its wrapped view when it becomes visible.
	editor->setSize( 300, 180 );
	editor->setVisible( true );
	editor->getDocument().textInput( "An editor opened after a density change.\n" );
	scene->update( Time::Zero );
	ASSERT_GT( editor->getDocumentView().getVisibleLinesCount(), size_t{ 0 } );
	scene->setPixelDensity( 1.25f );
	EXPECT_GT( editor->getDocumentView().getVisibleLinesCount(), size_t{ 0 } );
}

UTEST( PerWindowDensity, WindowChangesReachInheritingScenesBeforeUpdate ) {
	DensityApplication fixture;
	auto* window = fixture.app.getWindow();
	auto* scene = fixture.app.getUI();
	const Float fallback = PixelDensity::getPixelDensity();
	auto* probe = eeNew( PixelSetterWidget, () );
	probe->setParent( scene->getRoot() );
	const Uint32 version = window->getDisplayMetricsVersion();
	window->setPixelDensityOverride( 1.25f );
	EXPECT_GT( window->getDisplayMetricsVersion(), version );
	InputEvent displayChange{};
	displayChange.Type = InputEvent::Window;
	displayChange.WinID = window->getWindowID();
	displayChange.window.type = InputEvent::WindowDisplayChanged;
	for ( int i = 0; i < 3; ++i )
		window->getInput()->pushEvent( displayChange );
	Engine::instance()->updateInput();
	scene->update( Time::Zero );
	EXPECT_EQ( probe->densityChanges, 1 );
	EXPECT_EQ( scene->getPixelDensity(), 1.25f );
	scene->update( Time::Zero );
	EXPECT_EQ( probe->densityChanges, 1 );
	EXPECT_EQ( PixelDensity::getPixelDensity(), fallback );
	const Uint32 stableVersion = window->getDisplayMetricsVersion();
	EXPECT_FALSE( window->refreshPixelDensity() );
	EXPECT_EQ( window->getDisplayMetricsVersion(), stableVersion );
	scene->setPixelDensity( 1.75f );
	window->setPixelDensityOverride( 2.f );
	scene->update( Time::Zero );
	EXPECT_EQ( scene->getPixelDensity(), 1.75f );
	scene->clearPixelDensityOverride();
	EXPECT_EQ( scene->getPixelDensity(), 2.f );
	EXPECT_EQ( PixelDensity::getPixelDensity(), fallback );
}

UTEST( PerWindowDensity, DisplayMetricsUpdateMediaWithoutDensityReflow ) {
	DensityApplication fixture;
	auto* window = fixture.app.getWindow();
	auto* host = fixture.app.getUI();
	host->setPixelDensity( 1.25f );
	auto* hostProbe = eeNew( DensityReflowProbe, () );
	int reflows = 0;
	const auto countReflow = [&]( UIDensityReflow& ) { ++reflows; };
	hostProbe->capture = countReflow;
	auto* child = eeNew( DensityScene, ( window ) );
	child->setParent( host->getRoot() );
	host->update( Time::Zero );
	auto context = child->makeCurrent();
	auto* childProbe = eeNew( DensityReflowProbe, () );
	childProbe->capture = countReflow;
	childProbe->setId( "dpi-probe" );
	const Float displayDPI = child->getDPI();
	ASSERT_GT( displayDPI, 1.f );
	child->setTestDPI( 1.f );
	child->setStyleSheet( "#dpi-probe { background-color: #ff0000; } "
						  "@media (resolution:1dpi) { "
						  "#dpi-probe { background-color: #00ff00; } }" );
	EXPECT_TRUE( childProbe->getBackgroundColor() == Color( "#00ff00" ) );

	const auto version = window->getDisplayMetricsVersion();
	window->setPixelDensityOverride( 1.5f );
	EXPECT_GT( window->getDisplayMetricsVersion(), version );
	host->update( Time::Zero );
	EXPECT_EQ( host->getPixelDensity(), 1.25f );
	EXPECT_EQ( child->getPixelDensity(), 1.25f );
	EXPECT_EQ( reflows, 0 );
	EXPECT_EQ( child->getDPI(), displayDPI );
	EXPECT_TRUE( childProbe->getBackgroundColor() == Color::Red );
}

UTEST( UISceneDensity, DefaultMetricsScaleAndExplicitPixelMetricsStayFixed ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	UITreeView* defaults;
	UITreeView* custom;
	UITabWidget* tabs;
	UICodeEditor* editor;
	{
		auto context = scene->makeCurrent();
		defaults = UITreeView::New();
		defaults->setParent( scene->getRoot() );
		custom = UITreeView::New();
		custom->setParent( scene->getRoot() );
		custom->setIndentWidth( 13.f );
		custom->setExpanderIconSize( 15 );
		custom->setIconSize( 19 );
		custom->setSortIconSize( 23 );
		custom->setDragBorderDistance( 7.f );
		tabs = UITabWidget::New();
		tabs->setParent( scene->getRoot() );
		editor = UICodeEditor::New();
		editor->setParent( scene->getRoot() );
		editor->setLineNumberPadding( 7.f, 9.f );
	}
	for ( Float density : { 1.f, 2.f, 1.25f } ) {
		scene->setPixelDensity( density );
		EXPECT_EQ( defaults->getIndentWidth(), 6.f * density );
		EXPECT_EQ( defaults->getExpanderIconSize(), static_cast<size_t>( 12.f * density ) );
		EXPECT_EQ( defaults->getIconSize(), static_cast<size_t>( 12.f * density ) );
		EXPECT_EQ( defaults->getSortIconSize(), static_cast<size_t>( 20.f * density ) );
		EXPECT_EQ( defaults->getDragBorderDistance(), 8.f * density );
		EXPECT_EQ( custom->getIndentWidth(), 13.f );
		EXPECT_EQ( custom->getExpanderIconSize(), size_t{ 15 } );
		EXPECT_EQ( custom->getIconSize(), size_t{ 19 } );
		EXPECT_EQ( custom->getSortIconSize(), size_t{ 23 } );
		EXPECT_EQ( custom->getDragBorderDistance(), 7.f );
		EXPECT_EQ( tabs->getTabVerticalDragResistance(), 64.f * density );
		EXPECT_EQ( editor->getLineNumberPaddingLeft(), 7.f * density );
		EXPECT_EQ( editor->getLineNumberPaddingRight(), 9.f * density );
	}
}

UTEST( UISceneDensity, HooksCanCreateControlsAndDeletePendingPeers ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	auto* parent = eeNew( DensityCreationWidget, () );
	parent->setParent( scene->getRoot() );
	parent->victim = UIWidget::New();
	parent->victim->setParent( scene->getRoot() );
	scene->setPixelDensity( 1.f );
	ASSERT_TRUE( parent->created != nullptr );
	EXPECT_TRUE( parent->victim == nullptr );
	ASSERT_TRUE( parent->createdWindow != nullptr );
	EXPECT_GT( parent->createdWindow->getPixelsSize().y, 0.f );
	EXPECT_LT( parent->createdWindow->getPixelsSize().y, scene->getPixelsSize().y );
	EXPECT_EQ( parent->created->getFontSize(),
			   static_cast<Uint32>( scene->getUIThemeManager()->getDefaultFontSize() ) );
	scene->setPixelDensity( 1.25f );
	EXPECT_EQ( parent->created->getFontSize(),
			   static_cast<Uint32>( scene->getUIThemeManager()->getDefaultFontSize() ) );
}

UTEST( UISceneDensity, ImageDestinationCacheFollowsLogicalBounds ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setStyleSheet( "" );
	Image bitmap( 16, 12, 4, Color::Red );
	auto texture = TextureFactory::instance()->loadFromPixels(
		bitmap.getPixelsPtr(), bitmap.getWidth(), bitmap.getHeight(), bitmap.getChannels() );
	ASSERT_TRUE( texture != nullptr );
	DensityImage* image;
	{
		auto context = scene->makeCurrent();
		image = eeNew( DensityImage, () );
		image->setParent( scene->getRoot() );
		image->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		image->setSize( 80, 40 );
		image->setScaleType( UIScaleType::Expand );
		image->setDrawable( texture );
	}
	for ( Float density : { 1.f, 2.f, 1.25f } ) {
		scene->setPixelDensity( density );
		EXPECT_TRUE( image->destinationSize() == Sizef( 80.f * density, 40.f * density ) );
	}
}

UTEST( UISceneDensity, LogicalCustomOriginsFollowDensity ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	auto context = scene->makeCurrent();
	auto* wrapper = Node::New();
	wrapper->setParent( scene->getRoot() );
	auto* widget = UIWidget::New();
	widget->setParent( wrapper );
	wrapper->setRotation( 30.f, OriginPoint( 8.f, 12.f ) );
	wrapper->setScaleOriginPointPixels( OriginPoint( 3.f, 5.f ) );
	widget->setScale( 2.f, OriginPoint( 6.f, 4.f ) );
	for ( Float density : { 2.f, 1.25f, 1.f } ) {
		scene->setPixelDensity( density );
		EXPECT_NEAR( wrapper->getRotationOriginPoint().x, 8.f * density, 0.001f );
		EXPECT_NEAR( wrapper->getRotationOriginPoint().y, 12.f * density, 0.001f );
		EXPECT_EQ( wrapper->getScaleOriginPoint().x, 3.f );
		EXPECT_NEAR( widget->getScaleOriginPoint().x, 6.f * density, 0.001f );
	}
}

UTEST( UISceneDensity, EditorDropsProportionalWidthCacheOnFontChange ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	auto context = scene->makeCurrent();
	auto font = FontTrueType::New( "density-proportional" );
	ASSERT_TRUE(
		font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );
	auto* editor = UICodeEditor::New();
	editor->setParent( scene->getRoot() );
	editor->setFont( font.get() );
	editor->setFontSize( 20.f );
	editor->getDocument().textInput( "narrow iii and wide WWW" );
	const Float width = editor->getLineWidth( 0 );
	ASSERT_GT( width, 0.f );
	scene->setPixelDensity( 2.f );
	EXPECT_GT( editor->getLineWidth( 0 ), width * 1.8f );
	scene->setPixelDensity( 1.f );
	EXPECT_NEAR( editor->getLineWidth( 0 ), width, 1.f );
	eeDelete( editor );
}

UTEST( UISceneDensity, SharedSvgDataUrisKeepDensitySpecificRasters ) {
	DensityApplication fixture;
	auto resources = ResourceScope::New();
	DrawableResolver resolver( *resources );
	const std::string uri =
		"data:image/svg+xml," +
		URI::encode( "<svg xmlns='http://www.w3.org/2000/svg' width='10' height='6'>"
					 "<rect width='10' height='6' fill='red'/></svg>" );
	DrawablePtr first;
	DrawablePtr second;
	{
		auto density = PixelDensity::makeCurrent( 1.f );
		first = resolver.resolve( uri );
		ASSERT_TRUE( first != nullptr );
		EXPECT_TRUE( first->getPixelsSize() == Sizef( 10, 6 ) );
	}
	{
		auto density = PixelDensity::makeCurrent( 2.f );
		second = resolver.resolve( uri );
		ASSERT_TRUE( second != nullptr );
		EXPECT_TRUE( second->getPixelsSize() == Sizef( 20, 12 ) );
		EXPECT_TRUE( first->getPixelsSize() == Sizef( 10, 6 ) );
	}
	{
		auto density = PixelDensity::makeCurrent( 1.f );
		auto cached = resolver.resolve( uri );
		ASSERT_TRUE( cached != nullptr );
		EXPECT_TRUE( cached->getPixelsSize() == Sizef( 10, 6 ) );
		EXPECT_TRUE( second->getPixelsSize() == Sizef( 20, 12 ) );
	}
}

UTEST( UISceneDensity, FileSvgVariantsShareSourceWithoutChangingHtmlIntrinsicSize ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	auto context = scene->makeCurrent();
	const std::string path = Sys::getTempPath() + "eepp-density-raster-" +
							 String::toString( Sys::getSystemTime() ) + ".svg";
	ASSERT_TRUE( FileSystem::fileWrite( path, std::string_view( DensityTestSvg ) ) );
	ScopedOp cleanup( nullptr, [&] { FileSystem::fileRemove( path ); } );
	const std::string uri = URI::fromFSPath( path ).toString();
	auto* image = UIHTMLImage::New();
	image->setParent( scene->getRoot() );
	image->setStyleSheetProperty( StyleSheetProperty( "src", uri ) );
	auto first = image->getDrawable();
	ASSERT_TRUE( first != nullptr );
	EXPECT_EQ( first->getPixelsSize().x, 10.f );
	scene->setPixelDensity( 2.f );
	ASSERT_TRUE( image->getDrawable() != nullptr );
	EXPECT_EQ( image->getDrawable()->getPixelsSize().x, 20.f );
	EXPECT_EQ( image->getMaxIntrinsicWidth(), 20.f );
	EXPECT_EQ( first->getPixelsSize().x, 10.f );
	scene->setPixelDensity( 1.f );
	EXPECT_EQ( image->getMaxIntrinsicWidth(), 10.f );

	scene->setThreadPool( ThreadPool::createShared( 1 ) );
	scene->setPixelDensity( 1.5f );
	auto* deferred = UIHTMLImage::New();
	deferred->setParent( scene->getRoot() );
	deferred->setStyleSheetProperty( StyleSheetProperty( "defer", "true" ) );
	deferred->setStyleSheetProperty( StyleSheetProperty( "src", uri ) );
	Clock clock;
	while ( !deferred->getDrawable() && clock.getElapsedTime() < Seconds( 2 ) ) {
		scene->update( Milliseconds( 16 ) );
		Sys::sleep( Milliseconds( 1 ) );
	}
	ASSERT_TRUE( deferred->getDrawable() != nullptr );
	EXPECT_EQ( deferred->getDrawable()->getPixelsSize().x, 15.f );
	EXPECT_EQ( deferred->getMaxIntrinsicWidth(), 15.f );
}

UTEST( UISceneDensity, SharedWebSvgCacheKeepsSceneRasterVariants ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	scene->setPixelDensity( 1.f );
	auto context = scene->makeCurrent();
	auto* child = UISceneNode::New( fixture.app.getWindow(), false );
	child->setParent( scene->getRoot() );
	child->setPixelDensity( 2.f );
	auto cache = WebResourceCache::New();
	scene->setWebResourceCache( cache, 24 );
	child->setWebResourceCache( cache, 24 );
	int fetches = 0;
	cache->setFetcher(
		[&fetches]( const WebResourceRequest&, WebResourceCache::FetchCompletion complete ) {
			++fetches;
			auto status = Network::Http::Response::Ok;
			complete( Network::Http::Response::createFakeResponse( {}, status, DensityTestSvg ) );
		} );
	WebResourceRequest request;
	request.uri = URI( "https://example.invalid/icon.svg?variant=red" );
	auto first = scene->requestWebTexture( request, {} );
	auto second = child->requestWebTexture( request, {} );
	ASSERT_TRUE( first && second );
	EXPECT_EQ( first->getImageWidth(), 10u );
	EXPECT_EQ( second->getImageWidth(), 20u );
	EXPECT_EQ( second->getRasterScale(), 2.f );
	EXPECT_TRUE( scene->requestWebTexture( request, {} ) == first );
	EXPECT_EQ( fetches, 2 );
	EXPECT_EQ( cache->getRetainedBytes(), ( 10u * 6u + 20u * 12u ) * 4u );
}

UTEST( UISceneDensity, SvgRerasterizesAtFinalPhysicalTargetSize ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	auto* root = scene->loadLayoutFromString( HTMLFormatter::HTMLtoXML(
		"<html><body><svg id='density-svg' xmlns='http://www.w3.org/2000/svg' "
		"width='10' height='10' viewBox='0 0 10 10' style='width:20px;height:20px'>"
		"<rect width='10' height='10' fill='red'/></svg></body></html>" ) );
	ASSERT_TRUE( root != nullptr );
	auto* svg = root->querySelector( "#density-svg" )->asType<UISvg>();
	ASSERT_TRUE( svg != nullptr );
	for ( Float density : { 1.f, 2.f, 1.25f } ) {
		scene->setPixelDensity( density );
		Clock clock;
		while ( clock.getElapsedTime() < Seconds( 2 ) ) {
			scene->update( Milliseconds( 16 ) );
			if ( svg->getDrawable() ) {
				auto* sprite = static_cast<Sprite*>( svg->getDrawable().get() );
				if ( sprite->getCurrentTextureRegion() &&
					 eeabs( sprite->getCurrentTextureRegion()->getTexture()->getImageWidth() -
							20.f * density ) <= 1.f )
					break;
			}
			Sys::sleep( Milliseconds( 1 ) );
		}
		ASSERT_TRUE( svg->getDrawable() != nullptr );
		auto* sprite = static_cast<Sprite*>( svg->getDrawable().get() );
		ASSERT_TRUE( sprite->getCurrentTextureRegion() != nullptr );
		EXPECT_NEAR( sprite->getCurrentTextureRegion()->getTexture()->getImageWidth(),
					 20.f * density, 1.f );
		EXPECT_NEAR( sprite->getCurrentTextureRegion()->getTexture()->getImageHeight(),
					 20.f * density, 1.f );
	}
}

UTEST( UISceneDensity, SvgWorkerCanFinishAfterWidgetDestruction ) {
	DensityApplication fixture;
	auto* scene = fixture.app.getUI();
	auto pool = ThreadPool::createShared( 1 );
	scene->setThreadPool( pool );
	std::atomic<bool> releaseWorker{ false };
	pool->run( [&] {
		while ( !releaseWorker.load() )
			std::this_thread::yield();
	} );
	auto* svg = scene->loadLayoutFromString(
		"<svg xmlns='http://www.w3.org/2000/svg' width='1024' height='1024' "
		"viewBox='0 0 16 16'><rect width='16' height='16' fill='red'/></svg>" );
	const auto tag = reinterpret_cast<Uint64>( svg );
	const bool queued = svg && pool->existsTagInQueue( tag );
	releaseWorker.store( true );
	ASSERT_TRUE( queued );
	Clock clock;
	while ( pool->existsTagInQueue( tag ) && clock.getElapsedTime() < Seconds( 2 ) )
		std::this_thread::yield();
	ASSERT_FALSE( pool->existsTagInQueue( tag ) );
	eeDelete( svg );
	std::atomic<bool> finished{ false };
	pool->run( [&] { finished.store( true ); } );
	clock.restart();
	while ( !finished.load() && clock.getElapsedTime() < Seconds( 2 ) )
		Sys::sleep( Milliseconds( 1 ) );
	// Join before the sentinel's state goes out of scope, including on a slow host.
	scene->setThreadPool( nullptr );
	pool.reset();
	EXPECT_TRUE( finished.load() );
	scene->update( Time::Zero );
}

UTEST( PixelDensityRegression, PasswordTextUsesResolvedFontSize ) {
	DensityApplication fixture;
	static_assert( sizeof( PasswordInputProbe<UITextInput> ) == sizeof( UITextInput ) );
	static_assert( sizeof( PasswordInputProbe<UIHTMLTextInput> ) == sizeof( UIHTMLTextInput ) );
	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		PixelDensity::setPixelDensity( density );
		auto checkInput = [&]( auto* input ) {
			input->setParent( fixture.app.getUI() );
			input->setFontSize( PixelDensity::dpToPx( 16.f ) );
			input->setText( "secret" );
			input->setMode( UITextInput::TextInputMode::Password );
			EXPECT_EQ( input->visibleText().getCharacterSize(), input->getFontSize() );
			EXPECT_TRUE( input->visibleText().getFont() == input->getFont() );
			Text expected( input->visibleText().getString(), input->getFont(),
						   input->getFontSize() );
			EXPECT_NEAR( input->visibleText().getTextWidth(), expected.getTextWidth(), 0.01f );

			input->setFontSize( PixelDensity::dpToPx( 20.f ) );
			EXPECT_EQ( input->visibleText().getCharacterSize(), input->getFontSize() );
			input->setMode( UITextInput::TextInputMode::Normal );
			input->setMode( UITextInput::TextInputMode::Password );
			EXPECT_EQ( input->visibleText().getCharacterSize(), input->getFontSize() );
		};
		checkInput( eeNew( PasswordInputProbe<UITextInput>, () ) );
		checkInput( eeNew( PasswordInputProbe<UIHTMLTextInput>, () ) );
	}
}

UTEST( PixelDensityRegression, PixelPaddingUpdatesAndRepeatedValues ) {
	DensityApplication fixture;
	auto* widget = eeNew( PixelSetterWidget, () );
	widget->setParent( fixture.app.getUI() );
	const Rectf logical( 4, 6, 8, 10 );
	widget->setPadding( logical );
	int changes = widget->paddingChanges;
	// A pixel value equal to the previous DP value must still update the widget.
	widget->setPaddingPixels( logical );
	EXPECT_TRUE( widget->getPixelsPadding() == logical );
	EXPECT_TRUE( widget->getPadding() == Rectf( 2, 3, 4, 5 ) );
	EXPECT_EQ( widget->paddingChanges, changes + 1 );
	widget->setPaddingPixels( logical );
	EXPECT_EQ( widget->paddingChanges, changes + 1 );

	using Setter = UIWidget* (UIWidget::*)( const Float& );
	Setter setters[] = { &UIWidget::setPaddingPixelsLeft, &UIWidget::setPaddingPixelsTop,
						 &UIWidget::setPaddingPixelsRight, &UIWidget::setPaddingPixelsBottom };
	for ( auto setter : setters ) {
		widget->setPadding( Rectf( 4, 4, 4, 4 ) );
		changes = widget->paddingChanges;
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->paddingChanges, changes + 1 );
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->paddingChanges, changes + 1 );
		( widget->*setter )( 5 );
		const Rectf pixels = widget->getPixelsPadding();
		const Rectf dp = widget->getPadding();
		EXPECT_TRUE( dp == PixelDensity::pxToDp( pixels ).ceil() );
		EXPECT_EQ( pixels.Left + pixels.Top + pixels.Right + pixels.Bottom, 29.f );
	}
}

UTEST( PixelDensityRegression, PixelMarginsUpdateAndRepeatedValues ) {
	DensityApplication fixture;
	auto* widget = eeNew( PixelSetterWidget, () );
	widget->setParent( fixture.app.getUI() );
	const Rectf logical( 4, 6, 8, 10 );
	widget->setLayoutMargin( logical );
	int changes = widget->marginChanges;
	widget->setLayoutPixelsMargin( logical );
	EXPECT_TRUE( widget->getLayoutPixelsMargin() == logical );
	EXPECT_TRUE( widget->getLayoutMargin() == Rectf( 2, 3, 4, 5 ) );
	EXPECT_EQ( widget->marginChanges, changes + 1 );
	widget->setLayoutPixelsMargin( logical );
	EXPECT_EQ( widget->marginChanges, changes + 1 );

	using Setter = UIWidget* (UIWidget::*)( const Float& );
	Setter setters[] = { &UIWidget::setLayoutPixelsMarginLeft, &UIWidget::setLayoutPixelsMarginTop,
						 &UIWidget::setLayoutPixelsMarginRight,
						 &UIWidget::setLayoutPixelsMarginBottom };
	for ( auto setter : setters ) {
		widget->setLayoutMargin( Rectf( 4, 4, 4, 4 ) );
		changes = widget->marginChanges;
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->marginChanges, changes + 1 );
		( widget->*setter )( 4 );
		EXPECT_EQ( widget->marginChanges, changes + 1 );
		( widget->*setter )( 5 );
		const Rectf pixels = widget->getLayoutPixelsMargin();
		EXPECT_TRUE( widget->getLayoutMargin() == PixelDensity::pxToDp( pixels ).ceil() );
		EXPECT_EQ( pixels.Left + pixels.Top + pixels.Right + pixels.Bottom, 29.f );
	}
}

UTEST( PixelDensityRegression, ButtonLayoutIncludesTrailingMargin ) {
	DensityApplication fixture;
	fixture.app.getUI()->setStyleSheet( StyleSheet{} );
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* button = UIPushButton::New();
		button->setParent( fixture.app.getUI()->getRoot() );
		button->removeSkin();
		button->unsetFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
		button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		button->setPadding( Rectf::Zero );
		// Keep two visible items: their outer margins must remain part of the packed extent.
		button->setSize( 38, 24 );
		button->getIcon()->setSize( 12, 12 );
		auto* text = button->getTextView();
		text->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		text->setSize( 20, 10 );
		text->applyProperty( StyleSheetProperty( "margin-inline-start", "2dp" ) );
		text->applyProperty( StyleSheetProperty( "margin-inline-end", "4dp" ) );
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			button->setDirection( direction );
			for ( auto alignment : { UIHorizontalAlignment::Left, UIHorizontalAlignment::Center,
									 UIHorizontalAlignment::Right } ) {
				button->setHorizontalAlignment( alignment );
				EXPECT_EQ( button->getContentSize().x, PixelDensity::dpToPx( 38.f ) );
				EXPECT_EQ( button->updateLayout().x, PixelDensity::dpToPx( 38.f ) );
				EXPECT_EQ( text->getPixelsPosition().x,
						   PixelDensity::dpToPx( button->isRTL() ? 4.f : 14.f ) );
			}
		}
		eeDelete( button );
	}
}

UTEST( PixelDensityRegression, ButtonLayoutIgnoresMarginsOfLoneVisibleItem ) {
	DensityApplication fixture;
	fixture.app.getUI()->setStyleSheet( StyleSheet{} );
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* button = UIPushButton::New();
		button->setParent( fixture.app.getUI()->getRoot() );
		button->removeSkin();
		button->unsetFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
		button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		button->setPadding( Rectf( 4, 4, 4, 4 ) );
		button->setSize( 80, 24 );
		auto* icon = button->getIcon();
		icon->setSize( 12, 12 );
		auto* text = button->getTextView();
		text->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		text->setSize( 20, 10 );
		const Rectf declaredMargin( 2, 1, 6, 3 );
		icon->setLayoutMargin( declaredMargin );
		text->setLayoutMargin( declaredMargin );
		for ( auto orientation : { InnerWidgetOrientation::WidgetIconTextBox,
								   InnerWidgetOrientation::WidgetTextBoxIcon,
								   InnerWidgetOrientation::IconTextBoxWidget,
								   InnerWidgetOrientation::IconWidgetTextBox,
								   InnerWidgetOrientation::TextBoxIconWidget,
								   InnerWidgetOrientation::TextBoxWidgetIcon } ) {
			button->setInnerWidgetOrientation( orientation );
			for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
				button->setDirection( direction );
				for ( bool showIcon : { true, false } ) {
					icon->setVisible( showIcon );
					text->setVisible( !showIcon );
					auto* visible = showIcon ? static_cast<UIWidget*>( icon ) : text;
					const Float width = visible->getPixelsSize().x;
					EXPECT_EQ( button->getContentSize().x, width + PixelDensity::dpToPx( 8.f ) );
					EXPECT_EQ( button->updateLayout().x, width + PixelDensity::dpToPx( 8.f ) );
					EXPECT_NEAR( visible->getPixelsPosition().x,
								 ( button->getPixelsSize().x - width ) * 0.5f, 0.01f );
					EXPECT_TRUE( visible->getLayoutMargin() == declaredMargin );
				}
			}
		}
		// An expanded lone label must also receive the space previously reserved for margins.
		button->setExpandTextView( true );
		EXPECT_EQ( button->updateLayout().x, button->getPixelsSize().x );
		EXPECT_EQ( text->getPixelsPosition().x, PixelDensity::dpToPx( 4.f ) );
		EXPECT_EQ( text->getPixelsSize().x, PixelDensity::dpToPx( 72.f ) );
		eeDelete( button );
	}
}

UTEST( PixelDensityRegression, ExpandedButtonsKeepAssignedWidths ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	for ( Float density : { 1.f, 1.25f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* row = ui->loadLayoutFromString( R"xml(
<hbox lw="400dp" lh="wc">
 <PushButton id="weighted_label" lw="0" layout_weight="0.8" lh="wc"
  expand-text="true" text-align="left" text-overflow="ellipsis" />
 <Widget id="remaining_space" lw="0" layout_weight="0.2" lh="10dp" />
</hbox>
)xml" )
						->asType<UILinearLayout>();
		auto* button = row->find<UIPushButton>( "weighted_label" );
		auto* remainder = row->find<UIWidget>( "remaining_space" );
		const String longLabel( 200, 'W' );
		for ( const auto& text : { longLabel, String( "Short" ), longLabel } ) {
			button->setText( text );
			ui->flushDirtyStyleAndLayout();
			EXPECT_NEAR( row->getPixelsSize().x * 0.8f, button->getPixelsSize().x, 0.01f );
			EXPECT_NEAR( row->getPixelsSize().x * 0.2f, remainder->getPixelsSize().x, 0.01f );
			EXPECT_EQ( 0.f, button->getTextView()->getRealAlignOffset().x );
		}
		// A directly assigned fixed width has the same contract, without a weighted parent.
		button->setParent( ui->getRoot() );
		button->setLayoutWeight( 0 );
		button->setSize( 100, button->getSize().y );
		button->setText( "" );
		button->setText( longLabel );
		ui->flushDirtyStyleAndLayout();
		EXPECT_EQ( PixelDensity::dpToPx( 100.f ), button->getPixelsSize().x );
		eeDelete( row );
		eeDelete( button );
	}
}

UTEST( PixelDensityRegression, TabCloseInlineEndMarginScalesWithDensity ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	ui->setStyleSheet( R"css(
		tab { skin: none; padding: 0dp; }
		tab::text { padding: 0dp; }
		tab::close { width: 10dp; height: 10dp; margin-inline-end: 4dp; }
	)css" );
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* tabs = UITabWidget::New();
		tabs->setParent( ui->getRoot() );
		tabs->setSize( 300, 100 );
		tabs->setTabsClosable( true );
		auto* tab = tabs->add( "Close margin", UIWidget::New() );
		ui->flushDirtyStyleAndLayout();
		auto* close = tab->getCloseButton();
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			tabs->setDirection( direction );
			tab->updateLayout();
			const Float gap = tab->isRTL() ? close->getPixelsPosition().x
										   : tab->getPixelsSize().x - close->getPixelsPosition().x -
												 close->getPixelsSize().x;
			const Rectf& margin = close->getLayoutPixelsMargin();
			EXPECT_EQ( tab->isRTL() ? margin.Left : margin.Right, PixelDensity::dpToPx( 4.f ) );
			EXPECT_NEAR( gap, PixelDensity::dpToPx( 4.f ), 1.f );
		}
		eeDelete( tabs );
	}
}

UTEST( PixelDensityRegression, BreezeTabIconKeepsBalancedOuterMargins ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* tabs = UITabWidget::New();
		tabs->setParent( ui->getRoot() );
		tabs->setSize( 300, 100 );
		tabs->setTabsClosable( true );
		auto* tab = tabs->add( "breeze.css", UIWidget::New() );
		auto* icon = tab->getIcon();
		icon->setSize( 16, 16 );
		icon->setVisible( true );
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			tabs->setDirection( direction );
			ui->flushDirtyStyleAndLayout();
			tab->updateLayout();
			auto* close = tab->getCloseButton();
			const Float leadingGap =
				tab->isRTL()
					? tab->getPixelsSize().x - icon->getPixelsPosition().x - icon->getPixelsSize().x
					: icon->getPixelsPosition().x;
			const Float trailingGap = tab->isRTL()
										  ? close->getPixelsPosition().x
										  : tab->getPixelsSize().x - close->getPixelsPosition().x -
												close->getPixelsSize().x;
			EXPECT_NEAR( leadingGap, PixelDensity::dpToPx( 4.f ), 1.f );
			EXPECT_NEAR( trailingGap, PixelDensity::dpToPx( 4.f ), 1.f );
		}
		eeDelete( tabs );
	}
}

UTEST( PixelDensityRegression, BreezeButtonIconGapRequiresVisibleText ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	for ( Float density : { 1.f, 1.5f, 2.f, 3.f } ) {
		PixelDensity::setPixelDensity( density );
		for ( bool selectButton : { false, true } ) {
			auto* button = selectButton ? UISelectButton::New() : UIPushButton::New();
			button->setParent( ui->getRoot() );
			button->setText( "Edit build" );
			button->setTextAsFallback( true );
			button->setIcon( DrawablePtr( RectangleDrawable::New( {}, Sizef( 12, 12 ) ) ) );
			auto* icon = button->getIcon();
			icon->setSize( 12, 12 );
			auto* text = button->getTextView();
			for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
				button->setDirection( direction );
				const auto check = [&]( bool labelVisible ) {
					ui->flushDirtyStyleAndLayout();
					button->updateLayout();
					EXPECT_EQ( text->isVisible(), labelVisible );
					const Rectf& margin = icon->getLayoutPixelsMargin();
					EXPECT_EQ( button->isRTL() ? margin.Left : margin.Right,
							   PixelDensity::dpToPx( 4.f ) );
					if ( labelVisible ) {
						const Float gap =
							button->isRTL()
								? icon->getPixelsPosition().x - text->getPixelsPosition().x -
									  text->getPixelsSize().x
								: text->getPixelsPosition().x - icon->getPixelsPosition().x -
									  icon->getPixelsSize().x;
						EXPECT_NEAR( gap, PixelDensity::dpToPx( 4.f ), 1.f );
					} else {
						const Float left = icon->getPixelsPosition().x;
						const Float right =
							button->getPixelsSize().x - left - icon->getPixelsSize().x;
						EXPECT_NEAR( left, right, 1.f );
					}
				};
				check( false );
				button->setTextAsFallback( false );
				check( true );
				button->setText( "" );
				check( false );
				button->setText( "Edit build" );
				check( true );
				text->setVisible( false );
				check( false );
				text->setVisible( true );
				check( true );
				button->setTextAsFallback( true );
			}
			eeDelete( button );
		}
	}
}

UTEST( PixelDensityRegression, LoaderPagerAndMergeToolbarUseLogicalDefaults ) {
	DensityApplication fixture;
	auto* loader = UILoader::New();
	loader->setParent( fixture.app.getUI() );
	EXPECT_EQ( loader->getOutlineThickness(), 8.f );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( fixture.app.getUI() );
	EXPECT_EQ( viewer->getLoader()->getOutlineThickness(), 6.f );
	EXPECT_EQ( viewer->getLoader()->getPixelsSize().x, 128.f );
	auto* pager = UIViewPager::New();
	pager->setParent( fixture.app.getUI() );
	EXPECT_EQ( pager->getDragResistance(), 8.f );
	auto* merge = UIMergeView::New();
	merge->setParent( fixture.app.getUI() );
	auto* button = merge->addToolbarAction( "dpi-test", "Test", {}, "", [] {} );
	EXPECT_EQ( button->getLayoutMargin().Right, 4.f );
	EXPECT_EQ( button->getLayoutPixelsMargin().Right, 8.f );
}

UTEST( PixelDensityRegression, ListBoxAutomaticRowHeightAddsLogicalPadding ) {
	DensityApplication fixture;
	auto* list = eeNew( DensityListBox, () );
	list->setParent( fixture.app.getUI() );
	list->getReferenceItem()->setFontSize( 16 );
	list->recalculateRowHeight();
	const auto& style = list->getReferenceItem()->getFontStyleConfig();
	const Uint32 fontHeight =
		style.getFont() ? style.getFont()->getFontHeight( style.getFontCharacterSize() ) : 12;
	EXPECT_EQ( list->getRowHeight(), static_cast<Uint32>( fontHeight / 2.f + 4 ) );
}

UTEST( PixelDensityRegression, MessageBoxControlsUseLogicalDimensions ) {
	DensityApplication fixture;
	auto* editBox = UIMessageBox::New( UIMessageBox::TEXT_EDIT, "Edit" );
	EXPECT_EQ( editBox->getTextEdit()->getSize().x, 600.f );
	EXPECT_EQ( editBox->getTextEdit()->getSize().y, 200.f );
	EXPECT_EQ( editBox->getTextEdit()->getPixelsSize().x, 1200.f );
	auto* dropdownBox = UIMessageBox::New( UIMessageBox::DROPDOWNLIST, "Choose" );
	EXPECT_EQ( dropdownBox->getDropDownList()->getSize().x, 200.f );
	EXPECT_EQ( dropdownBox->getDropDownList()->getPixelsSize().x, 400.f );
	auto* comboBox = UIMessageBox::New( UIMessageBox::COMBOBOX, "Choose" );
	EXPECT_EQ( comboBox->getComboBox()->getSize().x, 200.f );
	EXPECT_EQ( comboBox->getComboBox()->getPixelsSize().x, 400.f );
}

UTEST( PixelDensityRegression, FontPickerUsesLogicalSceneInset ) {
	DensityApplication fixture;
	fixture.app.getUI()->setSize( 600, 450 );
	const Sizef scene = fixture.app.getUI()->getSize();
	auto* dialog = UIFontPickerDialog::New();

	EXPECT_EQ( dialog->getMinWindowSize().x, scene.x - 32.f );
	EXPECT_EQ( dialog->getMinWindowSize().y, scene.y - 32.f );
}

UTEST( PixelDensityRegression, CodeEditorMatchesDefaultTextFontSize ) {
	RestoreDensity restoreDensity;
	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		UIApplication app(
			WindowSettings( 1200, 1000, "Editor Default Font Size", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 density ) );
		auto* ui = app.getUI();
		ui->setStyleSheet( "" );
		auto* manager = ui->getUIThemeManager();
		const auto verifyDefault = [&]( Uint32 expected ) {
			auto* text = UITextView::New();
			auto* editor = UICodeEditor::New();
			text->setParent( ui->getRoot() );
			editor->setParent( ui->getRoot() );
			ui->update( Time::Zero );
			EXPECT_EQ( text->getFontSize(), expected );
			EXPECT_EQ( editor->getFontSize(), static_cast<Float>( expected ) );
			editor->fontSizeGrow();
			editor->fontSizeReset();
			EXPECT_EQ( editor->getFontSize(), static_cast<Float>( text->getFontSize() ) );
			editor->applyProperty( StyleSheetProperty( "font-size", "14dp" ) );
			EXPECT_EQ( editor->getFontSize(), PixelDensity::dpToPx( 14.f ) );
			eeDelete( editor );
			eeDelete( text );
		};
		verifyDefault(
			static_cast<Uint32>( manager->getDefaultTheme()->getDefaultFontSizeDp() * density ) );
		manager->getDefaultTheme()->setDefaultFontSize( PixelDensity::dpToPx( 10.f ) );
		verifyDefault( 10.f * density );
		manager->setDefaultFontSize( 20.f * density );
		verifyDefault( 20.f * density );
		manager->setDefaultTheme( UIThemePtr{} );
		verifyDefault( 20.f * density );
	}
}

UTEST( UISceneDensity, ManagerFontSizeOverrideIsSceneLocal ) {
	DensityApplication fixture;
	auto* host = fixture.app.getUI();
	auto* child = UISceneNode::New( fixture.app.getWindow() );
	child->setParent( host->getRoot() );
	auto* manager = host->getUIThemeManager();
	auto* childManager = child->getUIThemeManager();
	const auto theme = manager->getDefaultThemeHandle();
	ASSERT_TRUE( theme && childManager->getDefaultTheme() == theme.get() );
	const Float themeFontSize = theme->getDefaultFontSize();
	const Float themeFontSizeDp = theme->getDefaultFontSizeDp();

	manager->setDefaultFontSize( 20.f );
	EXPECT_EQ( manager->getDefaultFontSize(), 20.f );
	for ( Float density : { 1.f, 1.25f, 2.f } ) {
		host->setPixelDensity( density );
		EXPECT_EQ( manager->getDefaultFontSize(), 10.f * density );
		EXPECT_EQ( childManager->getDefaultFontSize(), themeFontSizeDp * density );
		EXPECT_EQ( theme->getDefaultFontSize(), themeFontSize );
	}

	manager->setDefaultTheme( UIThemePtr{} );
	EXPECT_EQ( manager->getDefaultFontSize(), 20.f );
	manager->setDefaultFontSize( 22.f );
	EXPECT_EQ( manager->getDefaultFontSize(), 22.f );
	manager->setDefaultTheme( theme );
	EXPECT_EQ( manager->getDefaultFontSize(), 22.f );
}

UTEST( UISceneDensity, ThemeFontSizeCanUseAnExplicitDensity ) {
	auto context = PixelDensity::makeCurrent( 3.f );
	auto theme = UITheme::New( "density-theme", "density-theme" );
	theme->setDefaultFontSize( 20.f, 2.f );
	EXPECT_EQ( theme->getDefaultFontSize(), 20.f );
	EXPECT_EQ( theme->getDefaultFontSizeDp(), 10.f );
}

UTEST( PixelDensityRegression, BreezePreservesConfiguredCodeEditorFontSize ) {
	DensityApplication fixture;
	auto* ui = fixture.app.getUI();
	auto* editor = UICodeEditor::New();
	editor->setParent( ui->getRoot() );
	// ecode applies its configured size after creation, before deferred CSS is resolved.
	const Float configuredSize = PixelDensity::dpToPx( 17.f );
	editor->setFontSize( configuredSize );
	ui->update( Time::Zero );
	EXPECT_EQ( editor->getFontSize(), configuredSize );
	editor->reloadStyle( true, true, true, true, true );
	ui->update( Time::Zero );
	EXPECT_EQ( editor->getFontSize(), configuredSize );
	// Settings changes must also survive subsequent theme/style reloads.
	const Float changedSize = PixelDensity::dpToPx( 19.f );
	editor->setFontSize( changedSize );
	editor->reloadStyle( true, true, true, true, true );
	ui->update( Time::Zero );
	EXPECT_EQ( editor->getFontSize(), changedSize );
	eeDelete( editor );
}

UTEST( PixelDensityRegression, EditorLineNumberPaddingScalesExactlyOnce ) {
	DensityApplication fixture;
	auto* editor = UICodeEditor::New();
	editor->setParent( fixture.app.getUI() );
	editor->setLineNumberPadding( 3, 5 );
	EXPECT_EQ( editor->getLineNumberPaddingLeft(), 6.f );
	EXPECT_EQ( editor->getLineNumberPaddingRight(), 10.f );
	editor->setLineNumberPadding( 3, 5 );
	EXPECT_EQ( editor->getLineNumberPaddingLeft(), 6.f );
	EXPECT_EQ( editor->getLineNumberPaddingRight(), 10.f );
	EXPECT_EQ( editor->getLineNumberWidth(),
			   eeceil( editor->getLineNumberDigits() * editor->getGlyphWidth() + 16.f ) );
}

UTEST( PixelDensityRegression, DiffGuttersUsePhysicalGlyphWidthAndGrowWithLineNumbers ) {
	DensityApplication fixture;
	auto* diff = UIDiffView::New();
	diff->setParent( fixture.app.getUI() );
	for ( bool large : { false, true, false } ) {
		diff->loadFromPatch( large
								 ? "--- a/test\n+++ b/test\n@@ -100000,1 +100000,1 @@\n-old\n+new\n"
								 : "--- a/test\n+++ b/test\n@@ -1,1 +1,1 @@\n-old\n+new\n" );
		const int digits = large ? 6 : 5;
		EXPECT_EQ( diff->getEditor()->getPluginsGutterSpace(),
				   diff->getEditor()->getGlyphWidth() * ( digits * 2 + 1 ) );
		diff->setViewMode( UIDiffView::ViewMode::SideBySide );
		for ( auto* editor : { diff->getLeftEditor(), diff->getRightEditor() } ) {
			EXPECT_EQ( editor->getPluginsGutterSpace(), editor->getGlyphWidth() * digits );
			editor->setFontSize( 18 );
			EXPECT_EQ( editor->getPluginsGutterSpace(), editor->getGlyphWidth() * digits );
		}
		diff->setViewMode( UIDiffView::ViewMode::Unified );
	}
}

UTEST( PixelDensityRegression, DiffImagesUseNativePixelsAndOnlyShrinkToFit ) {
	DensityApplication fixture;
	fixture.app.getUI()->setThreadPool( ThreadPool::createShared( 2 ) );
	const std::string oldPath( Sys::getTempPath() + "eepp-diff-density-old.png" );
	const std::string newPath( Sys::getTempPath() + "eepp-diff-density-new.png" );
	// Odd dimensions expose rounding errors when undoing fractional pixel density.
	Image oldImage( 81, 51, 4, Color::Red );
	Image newImage( 81, 51, 4, Color::Blue );
	ASSERT_TRUE( oldImage.saveToFile( oldPath, Image::SaveType::PNG ) );
	ASSERT_TRUE( newImage.saveToFile( newPath, Image::SaveType::PNG ) );

	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* diff = UIDiffView::New();
		diff->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		diff->setPixelsSize( 400, 200 );
		diff->loadFromFile( oldPath, newPath );
		auto* left = diff->getLeftImageViewer();
		auto* right = diff->getRightImageViewer();
		ASSERT_TRUE( left && right );
		const auto waitForImages = [&] {
			Clock clock;
			while ( ( !left->getImage()->getDrawable() || !right->getImage()->getDrawable() ) &&
					clock.getElapsedTime() < Seconds( 5 ) ) {
				SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
				Sys::sleep( Milliseconds( 1 ) );
			}
			return left->getImage()->getDrawable() && right->getImage()->getDrawable();
		};
		ASSERT_TRUE( waitForImages() );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 81.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 51.f, viewer->getImage()->getPixelsSize().y );
		}

		diff->setPixelsSize( 80, 100 );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 40.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 25.f, viewer->getImage()->getPixelsSize().y );
		}
		diff->setPixelsSize( 400, 20 );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 31.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 20.f, viewer->getImage()->getPixelsSize().y );
		}

		diff->setPixelsSize( 400, 200 );
		diff->setViewMode( UIDiffView::ViewMode::Unified );
		const auto viewers = diff->findAllByType<UIImageViewer>( UI_TYPE_IMAGE_VIEWER );
		ASSERT_EQ( size_t{ 3 }, viewers.size() );
		auto* visualDiff = viewers[2]->getImage();
		ASSERT_TRUE( visualDiff->getDrawable() );
		EXPECT_EQ( 81.f, visualDiff->getPixelsSize().x );
		EXPECT_EQ( 51.f, visualDiff->getPixelsSize().y );
		diff->setPixelsSize( 40, 100 );
		EXPECT_EQ( 40.f, visualDiff->getPixelsSize().x );
		EXPECT_EQ( 25.f, visualDiff->getPixelsSize().y );
		diff->setPixelsSize( 400, 200 );
		EXPECT_EQ( 81.f, visualDiff->getPixelsSize().x );
		EXPECT_EQ( 51.f, visualDiff->getPixelsSize().y );
		diff->setViewMode( UIDiffView::ViewMode::SideBySide );
		ASSERT_TRUE( waitForImages() );
		for ( auto* viewer : { left, right } ) {
			EXPECT_EQ( 81.f, viewer->getImage()->getPixelsSize().x );
			EXPECT_EQ( 51.f, viewer->getImage()->getPixelsSize().y );
		}
		eeDelete( diff );
	}
	FileSystem::fileRemove( oldPath );
	FileSystem::fileRemove( newPath );
}

UTEST( PixelDensityRegression, TableContainerPaddingAndOriginStayLogical ) {
	DensityApplication fixture;
	auto* table = eeNew( DensityWidgetTable, () );
	table->setParent( fixture.app.getUI() );
	table->unsetFlags( UI_AUTO_PADDING );
	table->setSize( 200, 100 );
	table->setPadding( Rectf( 3, 4, 5, 6 ) );
	EXPECT_TRUE( table->getContainerPadding() == Rectf( 3, 4, 5, 6 ) );
	EXPECT_EQ( table->getContainer()->getPosition().x, 3.f );
	EXPECT_EQ( table->getContainer()->getPosition().y, 4.f );
	EXPECT_EQ( table->getContainer()->getPixelsPosition().x, 6.f );
	EXPECT_EQ( table->getContainer()->getPixelsPosition().y, 8.f );
	unsigned char pixels[20 * 30 * 4] = {};
	auto texture = TextureFactory::instance()->loadFromPixels( pixels, 20, 30, 4 );
	auto skin = UISkin::New( "table-dpi-test" );
	skin->setStateDrawable( UIState::StateFlagNormal, NinePatch::New( texture, 2, 4, 6, 8, 2.f ) );
	table->setSkin( skin.get() );
	table->setFlags( UI_AUTO_PADDING );
	table->refreshSkinPadding();
	const Rectf expected = PixelDensity::pxToDp( skin->getBorderSize() ) + table->getPadding();
	EXPECT_TRUE( table->getContainerPadding() == expected );
	EXPECT_EQ( table->getContainer()->getPosition().x, expected.Left );
	EXPECT_EQ( table->getContainer()->getPosition().y, expected.Top );
}
