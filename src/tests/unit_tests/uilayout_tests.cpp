#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE;
using namespace EE::UI;

UTEST( UINode, PixelSizesClampToZeroWithoutExplicitMinimums ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 320, 240, "Nonnegative pixel sizes" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		auto* widget = UIWidget::New();
		const auto fitted = widget->fitMinMaxSizePx( { -20, -30 } );
		EXPECT_EQ( fitted.x, 0.f );
		EXPECT_EQ( fitted.y, 0.f );
		widget->setPixelsSize( -20, -30 );
		EXPECT_EQ( widget->getPixelsSize().x, 0.f );
		EXPECT_EQ( widget->getPixelsSize().y, 0.f );
		// Shrinking a padded container below its padding must not give its child a negative size.
		widget->setPixelsSize( 200, 0 );
		auto* child = UIWidget::New();
		child->setParent( widget );
		child->setPixelsSize( widget->getPixelsSize().x, widget->getPixelsSize().y - 4 );
		EXPECT_EQ( child->getPixelsSize().y, 0.f );
	}
}

UTEST( UINode, PixelMinimumsAndChangingEmConstraints ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 320, 240, "Pixel sizing constraints" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		auto* widget = UIWidget::New();
		widget->setMinSize( { 10, 20 } );
		widget->setPixelsSize( -10, -10 );
		EXPECT_EQ( widget->getPixelsSize().x, 10 * density );
		EXPECT_EQ( widget->getPixelsSize().y, 20 * density );
		widget->setPixelsSize( 40 * density, 50 * density );
		EXPECT_EQ( widget->getPixelsSize().x, 40 * density );
		EXPECT_EQ( widget->getPixelsSize().y, 50 * density );

		auto* text = UITextView::New();
		text->setFontSize( 12 * density );
		auto* relative = UIWidget::New();
		relative->setParent( text );
		relative->setMinSizeEq( "1em", "1em" );
		relative->setMaxSizeEq( "1em", "1em" );
		for ( Uint32 fontSize : { 12u, 6u, 24u, 3u } ) {
			text->setFontSize( fontSize * density );
			relative->setPixelsSize( 100 * density, 100 * density );
			EXPECT_EQ( relative->getPixelsSize().x, fontSize * density );
			EXPECT_EQ( relative->getPixelsSize().y, fontSize * density );
		}
	}
}

UTEST( UILinearLayout, AlignAgainstLayoutUsesParentPadding ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UILinearLayout Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIWidget* parent = UIWidget::New();
	parent->setPixelsSize( 200, 120 );
	parent->setPadding( { 10, 20, 30, 40 } );
	parent->setParent( app.getUI()->getRoot() );

	UILinearLayout* layout = UILinearLayout::NewVertical();
	layout->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	layout->setPixelsSize( 50, 20 );
	layout->setLayoutGravity( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
	layout->setParent( parent );

	app.getUI()->updateDirtyLayouts();

	EXPECT_NEAR( 65.f, layout->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 40.f, layout->getPixelsPosition().y, 0.1f );
}

UTEST( UILinearLayout, CrossAxisAlignmentUsesOwnPadding ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UILinearLayout Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UILinearLayout* vertical = UILinearLayout::NewVertical();
	vertical->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	vertical->setPixelsSize( 200, 120 );
	vertical->setPadding( { 10, 20, 30, 40 } );
	vertical->setParent( app.getUI()->getRoot() );

	UIWidget* centered = UIWidget::New();
	centered->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	centered->setPixelsSize( 50, 20 );
	centered->setLayoutGravity( UI_HALIGN_CENTER );
	centered->setParent( vertical );

	UIWidget* right = UIWidget::New();
	right->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	right->setPixelsSize( 50, 20 );
	right->setLayoutGravity( UI_HALIGN_RIGHT );
	right->setParent( vertical );

	UILinearLayout* horizontal = UILinearLayout::NewHorizontal();
	horizontal->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	horizontal->setPixelsSize( 200, 120 );
	horizontal->setPadding( { 10, 20, 30, 40 } );
	horizontal->setParent( app.getUI()->getRoot() );

	UIWidget* middle = UIWidget::New();
	middle->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	middle->setPixelsSize( 50, 20 );
	middle->setLayoutGravity( UI_VALIGN_CENTER );
	middle->setParent( horizontal );

	UIWidget* bottom = UIWidget::New();
	bottom->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	bottom->setPixelsSize( 50, 20 );
	bottom->setLayoutGravity( UI_VALIGN_BOTTOM );
	bottom->setParent( horizontal );

	app.getUI()->updateDirtyLayouts();

	EXPECT_NEAR( 65.f, centered->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 120.f, right->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 40.f, middle->getPixelsPosition().y, 0.1f );
	EXPECT_NEAR( 60.f, bottom->getPixelsPosition().y, 0.1f );
}

UTEST( UILinearLayout, HorizontalWeightsNormalizeAcrossVisibleChildren ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UILinearLayout Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UILinearLayout* layout = UILinearLayout::NewHorizontal();
	layout->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	layout->setPixelsSize( 300, 100 );
	layout->setParent( app.getUI()->getRoot() );

	const auto addChild = [layout]( Float weight ) {
		UIWidget* child = UIWidget::New();
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		child->setPixelsSize( 0, 20 );
		child->setLayoutWeight( weight );
		child->setParent( layout );
		return child;
	};
	UIWidget* first = addChild( 1 );
	UIWidget* middle = addChild( 2 );
	UIWidget* last = addChild( 1 );

	app.getUI()->updateDirtyLayouts();
	EXPECT_NEAR( 75.f, first->getPixelsSize().getWidth(), 0.1f );
	EXPECT_NEAR( 150.f, middle->getPixelsSize().getWidth(), 0.1f );
	EXPECT_NEAR( 75.f, last->getPixelsSize().getWidth(), 0.1f );

	middle->setVisible( false );
	app.getUI()->updateDirtyLayouts();
	EXPECT_NEAR( 150.f, first->getPixelsSize().getWidth(), 0.1f );
	EXPECT_NEAR( 150.f, last->getPixelsSize().getWidth(), 0.1f );
}

UTEST( UILinearLayout, HorizontalWeightAccountsForFixedSiblingMargins ) {
	UIApplication app(
		WindowSettings( 480, 240, "eepp - UILinearLayout Margin Weight Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UIWidget* parent = UIWidget::New();
	parent->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	parent->setPixelsSize( 400, 100 );
	parent->setPadding( { 12, 0, 12, 0 } );
	parent->setParent( app.getUI()->getRoot() );

	UILinearLayout* layout = UILinearLayout::NewHorizontal();
	layout->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	layout->setParent( parent );

	UIWidget* weighted = UIWidget::New();
	weighted->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	weighted->setPixelsSize( 0, 30 );
	weighted->setLayoutWeight( 1 );
	weighted->setParent( layout );

	for ( int i = 0; i < 3; ++i ) {
		UIWidget* fixed = UIWidget::New();
		fixed->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		fixed->setPixelsSize( 60, 30 );
		fixed->setLayoutMarginLeft( 8 );
		fixed->setParent( layout );
	}

	app.getUI()->updateDirtyLayouts();

	EXPECT_NEAR( 172.f, weighted->getPixelsSize().getWidth(), 0.1f );
	EXPECT_NEAR( 376.f, layout->getLastChild()->asType<UIWidget>()->getPixelsPosition().x + 60.f,
				 0.1f );
}

UTEST( UILinearLayout, VerticalWeightsNormalize ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UILinearLayout Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	UILinearLayout* layout = UILinearLayout::NewVertical();
	layout->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	layout->setPixelsSize( 100, 200 );
	layout->setParent( app.getUI()->getRoot() );

	UIWidget* first = UIWidget::New();
	first->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	first->setPixelsSize( 20, 0 );
	first->setLayoutWeight( 1 );
	first->setParent( layout );

	UIWidget* second = UIWidget::New();
	second->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	second->setPixelsSize( 20, 0 );
	second->setLayoutWeight( 3 );
	second->setParent( layout );

	app.getUI()->updateDirtyLayouts();
	EXPECT_NEAR( 50.f, first->getPixelsSize().getHeight(), 0.1f );
	EXPECT_NEAR( 150.f, second->getPixelsSize().getHeight(), 0.1f );
}
