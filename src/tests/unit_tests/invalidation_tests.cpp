#include "utest.hpp"
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/window.hpp>

#include <functional>

using namespace EE;
using namespace EE::Math;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::Window;

namespace {

struct InvalidationScene {
	UIApplication app;
	UISceneNode* scene;
	UIWidget* widget;

	InvalidationScene() :
		app(
			WindowSettings( 320, 240, "eepp - Invalidation", WindowStyle::Default,
							WindowBackend::Default, 32 ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) ),
		scene( app.getUI() ),
		widget( scene->loadLayoutFromString(
			R"xml(<widget layout_width="100dp" layout_height="50dp" />)xml" ) ) {
		settle();
	}

	/** Presents the scene, which is what clears its invalidation. */
	void settle() {
		scene->update( Milliseconds( 1 ) );
		scene->draw();
	}

	/** True if @p change requested a redraw. */
	bool invalidates( const std::function<void()>& change ) {
		settle();
		change();
		return scene->invalidated();
	}
};

} // namespace

UTEST( Invalidation, repeatedValuesOnlyInvalidateOnce ) {
	InvalidationScene test;
	auto* widget = test.widget;
	ASSERT_TRUE( widget != nullptr );
	EXPECT_FALSE( test.invalidates( [] {} ) );

	const std::vector<std::pair<const char*, std::function<void()>>> changes = {
		{ "rotation", [&] { widget->setRotation( 45.f ); } },
		{ "scale", [&] { widget->setScale( 2.f ); } },
		{ "blend", [&] { widget->setBlendMode( BlendMode::Add() ); } },
		{ "reverse draw", [&] { widget->setReverseDraw( true ); } },
		{ "background color", [&] { widget->setBackgroundColor( Color::Red ); } },
		{ "background tint", [&] { widget->setBackgroundTint( Color::Red, 0 ); } },
		{ "background drawable",
		  [&] { widget->setBackgroundDrawable( "rectangle(solid, red)", 0 ); } },
		{ "background position x", [&] { widget->setBackgroundPositionX( "left", 0 ); } },
		{ "background position y", [&] { widget->setBackgroundPositionY( "bottom", 0 ); } },
		{ "background size", [&] { widget->setBackgroundSize( "40% 2dp", 0 ); } },
		{ "background repeat", [&] { widget->setBackgroundRepeat( "repeat", 0 ); } },
		{ "background origin", [&] { widget->setBackgroundOrigin( "content-box", 0 ); } },
		{ "background clip", [&] { widget->setBackgroundClip( "content-box", 0 ); } },
		{ "foreground color", [&] { widget->setForegroundColor( Color::Green ); } },
		{ "foreground tint", [&] { widget->setForegroundTint( Color::Green, 0 ); } },
		{ "foreground size", [&] { widget->setForegroundSize( "10dp 2dp", 0 ); } },
		{ "foreground radius", [&] { widget->setForegroundTopLeftRadius( "3dp" ); } },
		{ "border color", [&] { widget->setBorderColor( Color::Blue ); } },
		{ "border width", [&] { widget->setBorderWidth( 2 ); } },
		{ "corners", [&] { widget->setBorderRadius( 6 ); } },
		{ "border radius", [&] { widget->setTopLeftRadius( "4dp" ); } },
	};
	for ( const auto& [name, change] : changes ) {
		const bool first = test.invalidates( change );
		const bool second = test.invalidates( change );
		if ( !first || second )
			printf( "%s: first %d second %d\n", name, first, second );
		EXPECT_TRUE( first );
		EXPECT_FALSE( second );
	}
}

UTEST( Invalidation, reappliedAndReadPropertiesDoNotInvalidate ) {
	InvalidationScene test;
	auto* widget = test.widget;
	ASSERT_TRUE( widget != nullptr );
	const std::vector<std::pair<const char*, const char*>> properties = {
		{ "border-left-color", "#ff0000" },	 { "border-top-width", "2dp" },
		{ "border-bottom-style", "dashed" }, { "background-color", "#00ff00" },
		{ "background-size", "50% 1dp" },	 { "rotation", "30" },
		{ "foreground-color", "#0000ff" },
	};
	const auto* specification = StyleSheetSpecification::instance();
	for ( const auto& [name, value] : properties ) {
		const StyleSheetProperty property( specification->getProperty( name ), value );
		const auto apply = [&] { widget->applyProperty( property ); };
		const bool first = test.invalidates( apply );
		const bool second = test.invalidates( apply );
		if ( !first || second )
			printf( "%s: first %d second %d\n", name, first, second );
		EXPECT_TRUE( first );
		EXPECT_FALSE( second );
	}

	// Once a border exists, reading its properties has no effect on the scene.
	EXPECT_FALSE( test.invalidates( [&] {
		widget->getPropertyString( specification->getProperty( "border-left-color" ), 0 );
		widget->getPropertyString( specification->getProperty( "border-top-width" ), 0 );
	} ) );
}

UTEST( Invalidation, drawingNeverRequestsAnotherDraw ) {
	InvalidationScene test;
	auto* widget = test.widget;
	ASSERT_TRUE( widget != nullptr );
	widget->setBackgroundColor( Color::Red );
	widget->setBackgroundDrawable( "rectangle(solid, blue)", 0 );
	widget->setBackgroundSize( "50% 2dp", 0 );
	widget->setForegroundColor( Color::Green );
	widget->setBorderWidth( 1 );
	widget->setBorderColor( Color::White );
	widget->setBorderRadius( 4 );
	widget->setAlpha( 128.f );
	test.settle();
	test.settle();
	EXPECT_FALSE( test.scene->invalidated() );
}

UTEST( Invalidation, referencedDrawablesFollowTheLayerSize ) {
	InvalidationScene test;
	auto* widget = test.widget;
	ASSERT_TRUE( widget != nullptr );
	widget->setBackgroundDrawable( "rectangle(solid, red)", 0 );
	test.settle();
	test.settle();
	auto* layer = widget->getBackground()->getLayer( 0 );
	EXPECT_STDSTREQ( "rectangle(solid, red)", layer->getDrawableRef() );

	// Updates that keep the size keep the drawable.
	const auto drawable = layer->getDrawable();
	ASSERT_TRUE( drawable != nullptr );
	widget->setBackgroundPositionX( "right", 0 );
	test.settle();
	EXPECT_TRUE( drawable == layer->getDrawable() );

	const Sizef size( widget->getPixelsSize() );
	widget->setPixelsSize( Sizef( size.getWidth() * 2, size.getHeight() ) );
	test.settle();
	EXPECT_FALSE( drawable == layer->getDrawable() );
	EXPECT_STDSTREQ( "rectangle(solid, red)", layer->getDrawableRef() );
}

UTEST( Invalidation, transformOriginsStayCurrent ) {
	InvalidationScene test;
	auto* widget = test.widget;
	ASSERT_TRUE( widget != nullptr );
	const Float density = PixelDensity::getPixelDensity();
	const Vector2f center( widget->getPixelsSize().getWidth() * 0.5f,
						   widget->getPixelsSize().getHeight() * 0.5f );
	widget->setPosition( 10.f, 20.f );
	widget->setRotation( 30.f );
	widget->setScale( 2.f );
	EXPECT_NEAR( widget->getRotationOrigin().x, center.x, 0.01f );
	EXPECT_NEAR( widget->getScaleOrigin().x, center.x, 0.01f );

	// Custom origins replace the center, are in dp, and with the same values are still a change.
	EXPECT_TRUE(
		test.invalidates( [&] { widget->setRotation( 30.f, OriginPoint( 3.f, 4.f ) ); } ) );
	EXPECT_TRUE( widget->getRotationOriginPoint().OriginType == OriginPoint::OriginCustom );
	EXPECT_NEAR( widget->getRotationOrigin().x, 3.f * density, 0.01f );
	EXPECT_NEAR( widget->getRotationOrigin().y, 4.f * density, 0.01f );
	EXPECT_TRUE( test.invalidates( [&] { widget->setScale( 2.f, OriginPoint( 5.f, 6.f ) ); } ) );
	EXPECT_NEAR( widget->getScaleOrigin().x, 5.f * density, 0.01f );
	EXPECT_NEAR( widget->getScaleOrigin().y, 6.f * density, 0.01f );

	// Every scale setter uses node coordinates, wherever the node is.
	EXPECT_TRUE(
		test.invalidates( [&] { widget->setScaleOriginPoint( OriginPoint( 7.f, 8.f ) ); } ) );
	EXPECT_NEAR( widget->getScaleOrigin().x, 7.f * density, 0.01f );
	EXPECT_FALSE( test.invalidates( [&] { widget->setScale( 2.f ); } ) );

	// Types can be switched in any direction.
	widget->setRotationOriginPoint( OriginPoint( OriginPoint::OriginTopLeft ) );
	EXPECT_NEAR( widget->getRotationOrigin().x, 0.f, 0.01f );
	widget->setRotationOriginPoint( OriginPoint( OriginPoint::OriginCenter ) );
	EXPECT_NEAR( widget->getRotationOrigin().x, center.x, 0.01f );
	EXPECT_FALSE( test.invalidates( [&] { widget->setRotation( 30.f ); } ) );

	// A resize moves a centered origin by itself.
	widget->setPixelsSize(
		Sizef( widget->getPixelsSize().getWidth() * 2, widget->getPixelsSize().getHeight() ) );
	EXPECT_NEAR( widget->getRotationOrigin().x, center.x * 2, 0.01f );

	// Angles are normalized when stored, so a full turn away isn't a change.
	EXPECT_FALSE( test.invalidates( [&] { widget->setRotation( 390.f ); } ) );

	// So do CSS origins, which are equations resolved against the size.
	const auto* specification = StyleSheetSpecification::instance();
	const StyleSheetProperty origin( specification->getProperty( "rotation-origin-point-x" ),
									 "25%" );
	EXPECT_TRUE( test.invalidates( [&] { widget->applyProperty( origin ); } ) );
	EXPECT_NEAR( widget->getRotationOrigin().x, center.x * 0.5f * 2, 0.01f );
	EXPECT_FALSE( test.invalidates( [&] { widget->applyProperty( origin ); } ) );
	widget->setPixelsSize(
		Sizef( widget->getPixelsSize().getWidth() * 2, widget->getPixelsSize().getHeight() ) );
	EXPECT_NEAR( widget->getRotationOrigin().x, center.x * 2, 0.01f );
}

namespace {

class CountingCellIcon : public RectangleDrawable {
  public:
	explicit CountingCellIcon( int& clones ) : mClones( clones ) {}

	DrawablePtr clone() const {
		++mClones;
		return makeResource<CountingCellIcon>( mClones );
	}

  private:
	int& mClones;
};

class CellIconModel : public Models::Model {
  public:
	UIIconPtr icon = UIIcon::New( "regression-icon" );

	size_t rowCount( const Models::ModelIndex& parent = {} ) const {
		return parent.isValid() ? 0 : 2;
	}

	size_t columnCount( const Models::ModelIndex& = {} ) const { return 1; }

	Models::Variant data( const Models::ModelIndex&, Models::ModelRole role ) const {
		if ( role == Models::ModelRole::Icon )
			return icon.get();
		return "row";
	}
};

} // namespace

UTEST( Invalidation, tableAndTreeRetainPrivateCellIconsAcrossFrames ) {
	InvalidationScene test;
	int clones = 0;
	auto model = std::make_shared<CellIconModel>();
	model->icon->setSource( 12, makeResource<CountingCellIcon>( clones ) );
	model->icon->setSource( 24, makeResource<CountingCellIcon>( clones ) );
	for ( bool tree : { false, true } ) {
		Abstract::UIAbstractTableView* view =
			tree ? static_cast<Abstract::UIAbstractTableView*>( UITreeView::New() )
				 : static_cast<Abstract::UIAbstractTableView*>( UITableView::New() );
		view->setParent( test.scene->getRoot() );
		view->setPixelsSize( 280, 200 );
		view->setIconSize( 12 );
		view->setModel( model );
		test.settle();
		const int initialClones = clones;
		EXPECT_GE( initialClones, 2 );
		for ( int frame = 0; frame < 5; ++frame )
			test.settle();
		EXPECT_EQ( initialClones, clones );

		// New size and replaced sources must still rebuild each cell's private instance.
		view->setIconSize( 24 );
		test.settle();
		EXPECT_EQ( initialClones + 2, clones );
		model->icon->setSource( 24, makeResource<CountingCellIcon>( clones ) );
		test.settle();
		EXPECT_EQ( initialClones + 4, clones );
		test.settle();
		EXPECT_EQ( initialClones + 4, clones );
		eeDelete( static_cast<UIWidget*>( view ) );
	}
}
