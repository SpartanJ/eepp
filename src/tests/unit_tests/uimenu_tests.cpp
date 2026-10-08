#include "utest.h"

#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uipopup.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::Graphics;
using namespace EE::UI;
using namespace EE::Window;

UTEST( UIMenu, SemanticActivationLifecycleAndRoles ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIMenu Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	UIMenu* menu = UIMenu::New();
	menu->setParent( app.getUI() );

	int clicked = 0;
	int shown = 0;
	int hidden = 0;
	menu->on( Event::OnItemClicked, [&clicked]( const Event* ) { ++clicked; } );
	menu->on( Event::OnMenuShow, [&shown]( const Event* ) { ++shown; } );
	menu->on( Event::OnMenuHide, [&hidden]( const Event* ) { ++hidden; } );

	UIMenuItem* item = menu->add( "Item" );
	EXPECT_EQ( item->getMenuRole(), MenuRole::NoRole );
	EXPECT_EQ( item->setMenuRole( MenuRole::About ), item );
	EXPECT_EQ( item->getMenuRole(), MenuRole::About );
	item->activate();
	EXPECT_EQ( clicked, 1 );
	item->setEnabled( false );
	item->activate();
	EXPECT_EQ( clicked, 1 );
	item->setEnabled( true );

	menu->show();
	item->setOnShouldCloseCb( []( UIMenuItem* ) { return false; } );
	item->activate();
	EXPECT_TRUE( menu->isVisible() );
	EXPECT_EQ( clicked, 2 );
	item->setOnShouldCloseCb( {} );
	menu->hide();

	UIMenuCheckBox* checkBox = menu->addCheckBox( "Check" );
	EXPECT_FALSE( checkBox->isActive() );
	checkBox->activate();
	EXPECT_TRUE( checkBox->isActive() );
	checkBox->activate();
	EXPECT_FALSE( checkBox->isActive() );
	EXPECT_EQ( clicked, 4 );

	UIMenuRadioButton* radioA = menu->addRadioButton( "A", true );
	UIMenuRadioButton* radioB = menu->addRadioButton( "B" );
	radioB->activate();
	EXPECT_FALSE( radioA->isActive() );
	EXPECT_TRUE( radioB->isActive() );
	radioB->activate();
	EXPECT_TRUE( radioB->isActive() );
	EXPECT_EQ( clicked, 6 );

	menu->notifyMenuWillShow();
	menu->notifyMenuDidHide();
	EXPECT_EQ( shown, 2 );
	EXPECT_EQ( hidden, 3 );

	EXPECT_EQ( menu->getMenuBarRole(), MenuBarRole::Normal );
	EXPECT_EQ( menu->setMenuBarRole( MenuBarRole::Help ), menu );
	EXPECT_EQ( menu->getMenuBarRole(), MenuBarRole::Help );

	UIPopUpMenu* childMenu = UIPopUpMenu::New();
	childMenu->setParent( app.getUI() );
	UIMenuSubMenu* subMenu = menu->addSubMenu( "Submenu", {}, childMenu );
	int subMenuShowStep = 0;
	subMenu->on( Event::OnMenuShow, [&subMenuShowStep]( const Event* ) {
		subMenuShowStep = 0 == subMenuShowStep ? 1 : -1;
	} );
	childMenu->on( Event::OnMenuShow, [&subMenuShowStep]( const Event* ) {
		subMenuShowStep = 1 == subMenuShowStep ? 2 : -1;
	} );
	subMenu->showSubMenu();
	EXPECT_EQ( subMenuShowStep, 2 );
}

UTEST( UIMenu, NestedMenuBarAndSubmenuAtHighDensity ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Menu DPI Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	const Float previousDensity = PixelDensity::getPixelDensity();
	PixelDensity::setPixelDensity( 2.f );
	auto* container = UIWidget::New();
	container->setParent( app.getUI() );
	container->setPosition( 30, 40 );
	auto* bar = UIMenuBar::New();
	bar->setParent( container );
	bar->setPosition( 10, 15 );
	bar->setSize( 300, 24 );
	auto* popup = UIPopUpMenu::New();
	popup->add( "Item" );
	bar->addMenuButton( "File", popup );
	auto* button = bar->getButton( 0 );
	Vector2f expected( 0, button->getSize().getHeight() );
	button->nodeToWorld( expected );
	bar->showMenu( 0 );
	Vector2f actual( 0, 0 );
	popup->nodeToWorld( actual );
	EXPECT_EQ( actual.x, expected.x );
	EXPECT_EQ( actual.y, expected.y );
	popup->setPadding( Rectf( 0, 0, 3, 0 ) );
	auto* child = UIPopUpMenu::New();
	child->add( "Child" );
	auto* item = popup->addSubMenu( "Submenu", {}, child );
	// The placement policy aligns the submenu to the parent menu's right edge.
	Vector2f subExpected( popup->getSize().getWidth(), 0 );
	popup->nodeToWorld( subExpected );
	item->showSubMenu();
	Vector2f subActual( 0, 0 );
	child->nodeToWorld( subActual );
	EXPECT_EQ( subActual.x, subExpected.x );
	PixelDensity::setPixelDensity( previousDensity );
}

UTEST( UIMenu, SharedPopupPlacementAtSceneEdges ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Popup Placement Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* anchor = UIWidget::New();
	anchor->setParent( app.getUI()->getRoot() );
	anchor->setPixelsSize( 40, 20 );
	anchor->setPixelsPosition( 100, 100 );
	auto* popup = UIWidget::New();
	popup->setParent( app.getUI()->getRoot() );
	popup->setPixelsSize( 100, 80 );
	Vector2f position( 100, 100 );
	UIMenu::findBestMenuPos( position, popup, nullptr, nullptr, anchor );
	EXPECT_TRUE( position == Vector2f( 100, 120 ) );
	UIPopUp::align( anchor, popup );
	EXPECT_TRUE( popup->getScreenPos() == position );

	anchor->setPixelsPosition( 100, 220 );
	position = Vector2f( 100, 220 );
	UIMenu::findBestMenuPos( position, popup, nullptr, nullptr, anchor );
	EXPECT_TRUE( position == Vector2f( 100, 140 ) );
	UIPopUp::align( anchor, popup );
	EXPECT_TRUE( popup->getScreenPos() == position );

	popup->setPixelsSize( 100, 140 );
	anchor->setPixelsPosition( 100, 90 );
	position = Vector2f( 100, 90 );
	UIMenu::findBestMenuPos( position, popup, nullptr, nullptr, anchor );
	EXPECT_TRUE( position == Vector2f( 140, 90 ) );
	// Field popups retain vertical attachment instead of moving beside the field.
	UIPopUp::align( anchor, popup );
	EXPECT_TRUE( popup->getScreenPos() == Vector2f( 100, 100 ) );
	anchor->setPixelsPosition( 280, 90 );
	position = Vector2f( 280, 90 );
	UIMenu::findBestMenuPos( position, popup, nullptr, nullptr, anchor );
	EXPECT_TRUE( position == Vector2f( 180, 90 ) );
	UIPopUp::align( anchor, popup );
	EXPECT_TRUE( popup->getScreenPos() == Vector2f( 220, 100 ) );

	popup->setPixelsSize( 100, 80 );
	position = Vector2f( 300, 230 );
	UIMenu::findBestMenuPos( position, popup );
	EXPECT_TRUE( position == Vector2f( 200, 150 ) );
	position = Vector2f( 300, 10 );
	UIMenu::findBestMenuPos( position, popup );
	EXPECT_TRUE( position == Vector2f( 200, 10 ) );
	popup->setPixelsSize( 400, 300 );
	position = Vector2f( 300, 230 );
	UIMenu::findBestMenuPos( position, popup );
	EXPECT_TRUE( position == Vector2f::Zero );
}

UTEST( UIMenu, SharedPopupPlacementUsesWorldPixelsAtHighDensity ) {
	UIApplication app(
		WindowSettings( 640, 480, "eepp - Popup World Coordinates Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	const Float previousDensity = PixelDensity::getPixelDensity();
	PixelDensity::setPixelDensity( 2.f );
	app.getUI()->setPosition( 30, 40 );
	auto* container = UIWidget::New();
	container->setParent( app.getUI()->getRoot() );
	container->setPosition( 20, 30 );
	auto* anchor = UIWidget::New();
	anchor->setParent( container );
	anchor->setPosition( 10, 15 );
	anchor->setSize( 50, 20 );
	auto* popup = UIWidget::New();
	popup->setParent( app.getUI()->getRoot() );
	popup->setSize( 80, 60 );
	const Rectf field( anchor->getScreenRect() );
	Vector2f position( field.Left, field.Top );
	UIMenu::findBestMenuPos( position, popup, nullptr, nullptr, anchor );
	EXPECT_TRUE( position == Vector2f( field.Left, field.Bottom ) );
	UIPopUp::align( anchor, popup, true );
	EXPECT_TRUE( popup->getScreenPos() == position );
	UIPopUp::align( anchor, popup, true, true );
	EXPECT_TRUE(
		popup->getScreenPos() ==
		Vector2f( field.Left + ( field.getWidth() - popup->getPixelsSize().getWidth() ) / 2,
				  field.Bottom ) );
	popup->setPixelsSize( 800, 600 );
	position = Vector2f( -100, -100 );
	UIMenu::findBestMenuPos( position, popup );
	EXPECT_TRUE( position == app.getUI()->getWorldBounds().getPosition() );
	PixelDensity::setPixelDensity( previousDensity );
}

namespace {

class DensityTestCheckBox : public UICheckBox {
  public:
	using UICheckBox::onAutoSize;
};

class DensityTestRadioButton : public UIRadioButton {
  public:
	using UIRadioButton::onAutoSize;
};

class DensityTestScrollBar : public UIScrollBar {
  public:
	DensityTestScrollBar( UIOrientation orientation ) : UIScrollBar( "scrollbar", orientation ) {}

	using UIScrollBar::onAutoSize;
};

} // namespace

UTEST( UIUnits, AutoSizeAtHighDensity ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Sizing DPI Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	const Float previousDensity = PixelDensity::getPixelDensity();
	PixelDensity::setPixelDensity( 2.f );
	auto* checkbox = eeNew( DensityTestCheckBox, () );
	checkbox->setParent( app.getUI() );
	checkbox->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	checkbox->setFlags( UI_AUTO_SIZE );
	checkbox->setTextSeparation( 8 );
	checkbox->setPadding( Rectf( 3, 4, 5, 6 ) );
	checkbox->getCheckedButton()->setSize( 10, 12 );
	checkbox->setSize( 0, 0 );
	checkbox->onAutoSize();
	EXPECT_EQ( checkbox->getPixelsSize().x, (int)checkbox->getTextWidth() + 20 + 16 +
												checkbox->getPixelsPadding().Left +
												checkbox->getPixelsPadding().Right );
	auto* radio = eeNew( DensityTestRadioButton, () );
	radio->setParent( app.getUI() );
	radio->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	radio->setFlags( UI_AUTO_SIZE );
	radio->setPadding( Rectf( 3, 4, 5, 6 ) );
	radio->getActiveButton()->setSize( 10, 12 );
	radio->setMinSize( Sizef::Zero );
	radio->setSize( 0, 0 );
	radio->onAutoSize();
	EXPECT_EQ( radio->getSize().y, 12 + radio->getPadding().Top + radio->getPadding().Bottom );

	auto region = TextureRegion::New();
	region->setDestSize( Sizef( 20, 30 ) );
	region->setPixelDensity( 2.f );
	unsigned char texturePixels[20 * 30 * 4] = {};
	auto texture = TextureFactory::instance()->loadFromPixels( texturePixels, 20, 30, 4 );
	auto ninePatch = NinePatch::New( texture, 2, 2, 2, 2, 2.f );
	DrawablePtr drawables[] = { DrawablePtr( RectangleDrawable::New( {}, Sizef( 20, 30 ) ) ),
								region, ninePatch };
	for ( auto& drawable : drawables ) {
		auto skin = UISkin::New( "dpi-test" );
		skin->setStateDrawable( UIState::StateFlagNormal, drawable );
		for ( auto orientation : { UIOrientation::Vertical, UIOrientation::Horizontal } ) {
			auto* scrollbar = eeNew( DensityTestScrollBar, ( orientation ) );
			scrollbar->setParent( app.getUI() );
			scrollbar->setSize( 100, 100 );
			scrollbar->getSlider()->getBackSlider()->setSkin( skin.get() );
			scrollbar->onAutoSize();
			const Sizef pixels = skin->getPixelsSize();
			EXPECT_EQ( scrollbar->getCurMinSize().x, pixels.x / 2.f );
			EXPECT_EQ( scrollbar->getCurMinSize().y, pixels.y / 2.f );
			if ( orientation == UIOrientation::Vertical ) {
				EXPECT_EQ( scrollbar->getPixelsSize().x, pixels.x );
				EXPECT_EQ( scrollbar->getSlider()->getPixelsSize().x, pixels.x );
			} else {
				EXPECT_EQ( scrollbar->getPixelsSize().y, pixels.y );
				EXPECT_EQ( scrollbar->getSlider()->getPixelsSize().y, pixels.y );
			}
		}
	}
	PixelDensity::setPixelDensity( previousDensity );
}
