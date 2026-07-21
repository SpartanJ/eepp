#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE;
using namespace EE::UI;

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
