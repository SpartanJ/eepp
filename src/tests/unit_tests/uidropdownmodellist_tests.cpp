#include "utest.h"

#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace {

class IconDropdownModel : public Model {
  public:
	IconDropdownModel( UIIcon* icon, DrawablePtr drawable ) :
		mDrawable( std::move( drawable ) ), mIcon( icon ) {}

	size_t rowCount( const ModelIndex& = {} ) const override { return 3; }

	size_t columnCount( const ModelIndex& = {} ) const override { return 1; }

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( role == ModelRole::Display )
			return Variant( "A fairly long process filter label" );
		if ( role == ModelRole::Icon ) {
			if ( index.row() == 0 )
				return Variant( mIcon );
			if ( index.row() == 1 )
				return Variant( mDrawable );
		}
		return {};
	}

  private:
	DrawablePtr mDrawable;
	UIIcon* mIcon;
};

} // namespace

UTEST( UIDropDownModelList, SelectedIconTracksSelectionAndModelReplacement ) {
	UIApplication app(
		WindowSettings( 640, 480, "Dropdown icons", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* ui = app.getUI();
	auto* dropdown = UIDropDownModelList::New();
	dropdown->setParent( ui->getRoot() );
	dropdown->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
	dropdown->setSize( 200, 24 );
	auto drawable = ui->findIconDrawable( "document-open", 40 );
	ASSERT_TRUE( drawable != nullptr );
	auto model = std::make_shared<IconDropdownModel>( ui->findIcon( "list-view" ), drawable );
	dropdown->setModel( model );
	auto* list = dropdown->getListView();
	ASSERT_TRUE( dropdown->getIcon() == nullptr );
	list->setSelection( model->index( 0, 0 ) );
	auto* icon = dropdown->getIcon();
	ASSERT_TRUE( icon != nullptr );
	EXPECT_TRUE( icon->isVisible() );
	EXPECT_TRUE( icon->getDrawable() != nullptr );
	EXPECT_FALSE( icon->isEnabled() );
	EXPECT_TRUE( dropdown->getText() == "A fairly long process filter label" );
	EXPECT_FALSE( list->isVisible() );

	list->moveSelection( 1 );
	EXPECT_TRUE( dropdown->getIcon() == icon );
	EXPECT_TRUE( icon->getDrawable() == drawable );
	EXPECT_GE( dropdown->getPixelsSize().getHeight(),
			   drawable->getPixelsSize().getHeight() + dropdown->getPixelsPadding().getHeight() );
	list->moveSelection( 1 );
	EXPECT_FALSE( icon->isVisible() );
	EXPECT_TRUE( icon->getDrawable() == nullptr );
	list->setSelection( model->index( 0, 0 ) );
	list->getSelection().clear();
	EXPECT_FALSE( icon->isVisible() );

	auto* replacement = UIListView::New();
	dropdown->setListView( replacement );
	replacement->setSelection( model->index( 1, 0 ) );
	EXPECT_TRUE( icon->getDrawable() == drawable );
	EXPECT_TRUE( icon->isVisible() );
	dropdown->setAlpha( 128 );
	EXPECT_EQ( icon->getAlpha(), 128.f );
	dropdown->setModel( nullptr );
	EXPECT_FALSE( icon->isVisible() );
	EXPECT_TRUE( icon->getDrawable() == nullptr );
	eeDelete( dropdown );
}

UTEST( UIDropDownModelList, SelectedIconReservesLabelSpaceAtDifferentDensitiesAndDirections ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 640, 480, "Dropdown icon layout", WindowStyle::Default,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		auto* ui = app.getUI();
		auto* dropdown = UIDropDownModelList::New();
		dropdown->setParent( ui->getRoot() );
		dropdown->setSize( 140, 30 );
		dropdown->setPadding( Rectf( 6, 6, 4, 4 ) );
		dropdown->setTextOverflow( "ellipsis" );
		dropdown->getListView()->setIconSize( PixelDensity::dpToPxI( 16 ) );
		auto model = std::make_shared<IconDropdownModel>( ui->findIcon( "list-view" ), nullptr );
		dropdown->setModel( model );
		const Rectf padding = dropdown->getPixelsPadding();
		for ( auto direction : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
			dropdown->setDirection( direction );
			dropdown->getListView()->setSelection( model->index( 2, 0 ) );
			dropdown->getListView()->setSelection( model->index( 0, 0 ) );
			auto* icon = dropdown->getIcon();
			ASSERT_TRUE( icon != nullptr );
			const Float width = dropdown->getPixelsSize().getWidth();
			const Float space = icon->getPixelsSize().getWidth() + PixelDensity::dpToPx( 4 );
			EXPECT_TRUE( dropdown->getPixelsPadding() == padding );
			EXPECT_LE( dropdown->getTextCache()->getTextWidth(),
					   width - padding.getWidth() - space );
			const Float textLeft = padding.Left + dropdown->getRealAlignOffset().x;
			const Float textRight = textLeft + dropdown->getTextCache()->getTextWidth();
			EXPECT_GE( textLeft, padding.Left );
			if ( direction == UIDirection::LeftToRight ) {
				EXPECT_EQ( icon->getPixelsPosition().x, padding.Left );
				EXPECT_GE( dropdown->getRealAlignOffset().x, space );
				EXPECT_LE( textRight, width - padding.Right );
			} else {
				EXPECT_EQ( icon->getPixelsPosition().x + icon->getPixelsSize().getWidth(),
						   width - padding.Right );
				EXPECT_LE( textRight, width - padding.Right - space );
			}
		}
	}
}

UTEST( UIDropDown, SelectionNavigationBeforeOpeningPopup ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app(
			WindowSettings( 640, 480, "Unopened dropdown navigation", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 density ) );
		auto* dropdown = UIDropDownList::New();
		dropdown->setParent( app.getUI()->getRoot() );
		dropdown->setSize( 200, 22 );
		auto* list = dropdown->getListBox();
		list->addListBoxItems( { "Branches", "Status", "History" } );
		// An unopened popup can have no content viewport yet, especially with theme padding.
		list->setPixelsSize( 200, 0 );
		list->setPaddingPixelsTop( 2 );
		list->setPaddingPixelsBottom( 2 );
		list->setSelected( 1 );
		auto* selected = list->getItemSelected();
		ASSERT_TRUE( selected != nullptr );
		EXPECT_GE( selected->getParent()->getPixelsSize().getHeight(), 0.f );
		auto* input = UITextInput::New();
		input->setParent( app.getUI()->getRoot() );
		input->setFocus();
		ASSERT_FALSE( list->isVisible() );
		list->selectNext();
		EXPECT_EQ( list->getItemSelectedIndex(), 2u );
		list->selectPrev();
		EXPECT_EQ( list->getItemSelectedIndex(), 1u );
		list->selectPrev();
		EXPECT_EQ( list->getItemSelectedIndex(), 0u );
		EXPECT_FALSE( list->isVisible() );
		EXPECT_TRUE( input->hasFocus() );
	}
}

UTEST( UIDropDownModelList, basicFunctionality ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - UIDropDownModelList Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5 ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	UISceneNode* sceneNode = app.getUI();

	UIDropDownModelList* dropDown = UIDropDownModelList::New();
	dropDown->setParent( sceneNode );

	std::vector<std::string> items = { "Item 1", "Item 2", "Item 3" };
	auto model = ItemListOwnerModel<std::string>::create( items );
	dropDown->setModel( model );

	// Model should be set
	EXPECT_TRUE( dropDown->getModel() == model );

	// Items count should match
	EXPECT_EQ( dropDown->getListView()->getModel()->rowCount(), 3ul );

	// Max visible items
	dropDown->setSize( 200, 30 );
	dropDown->setMaxNumVisibleItems( 3 );
	dropDown->showList();
	EXPECT_FALSE( dropDown->getListView()->getVerticalScrollBar()->isVisible() );

	dropDown->getListView()->setVisible( false );
	dropDown->setMaxNumVisibleItems( 2 );
	dropDown->showList();
	EXPECT_EQ( dropDown->getMaxNumVisibleItems(), 2ul );
	EXPECT_TRUE( dropDown->getListView()->getVerticalScrollBar()->isVisible() );
}

UTEST( UIDropDownModelList, PopupHeightUsesScrollbarsAtTheNewWidth ) {
	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		UIApplication app( WindowSettings( 1000, 800, "Dropdown popup height", WindowStyle::Default,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto* dropdown = UIDropDownModelList::New();
		dropdown->setParent( app.getUI()->getRoot() );
		dropdown->setModel( ItemListOwnerModel<std::string>::create(
			{ "A long process filter label", "Second filter", "Third filter" } ) );
		auto* list = dropdown->getListView();
		list->setRowHeight( PixelDensity::dpToPx( 23 ) );
		list->setPadding( Rectf( 3, 5, 7, 9 ) );
		for ( bool headers : { false, true } ) {
			list->setHeadersVisible( headers );
			for ( Uint32 visibleRows : { 2u, 3u } ) {
				dropdown->setMaxNumVisibleItems( visibleRows );
				list->setHorizontalScrollMode( ScrollBarMode::Auto );
				// The unopened viewport is too narrow, so its old horizontal scrollbar is visible.
				list->setPixelsSize( 1, 1 );
				ASSERT_TRUE( list->getHorizontalScrollBar()->isVisible() );
				auto open = [&]( Float width, bool horizontalScroll ) {
					dropdown->setSize( width, 30 );
					dropdown->showList();
					EXPECT_EQ( list->getHorizontalScrollBar()->isVisible(), horizontalScroll );
					EXPECT_EQ( list->getVerticalScrollBar()->isVisible(), visibleRows < 3 );
					const Rectf padding = list->getPixelsPadding();
					const Float expectedHeight = std::ceil(
						visibleRows * list->getRowHeight() + padding.Top + padding.Bottom +
						list->getHeaderHeight() +
						( horizontalScroll
							  ? list->getHorizontalScrollBar()->getPixelsSize().getHeight()
							  : 0.f ) );
					EXPECT_EQ( list->getPixelsSize().getHeight(), expectedHeight );
					EXPECT_GE( list->getVisibleArea().getHeight(),
							   visibleRows * list->getRowHeight() + list->getHeaderHeight() );
					list->setVisible( false );
				};
				open( 400, false );
				open( 60, true );
				open( 400, false );
				const Rectf padding = list->getPixelsPadding();
				const Float fittingWidth =
					list->getMaxColumnContentWidth( 0, true ) + padding.Left + padding.Right +
					( visibleRows < 3 ? list->getVerticalScrollBar()->getPixelsSize().getWidth()
									  : 0.f );
				// Use whole dp so density rounding cannot move the narrow case back onto the fit.
				const Float fittingWidthDp = PixelDensity::pxToDp( fittingWidth );
				open( std::ceil( fittingWidthDp ) + 1, false );
				open( std::floor( fittingWidthDp ) - 1, true );
				open( std::ceil( fittingWidthDp ) + 1, false );
				list->setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
				open( 400, true );
				list->setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
				open( 60, false );
			}
		}
	}
}

UTEST( UIDropDown, PlacementParentingAndFocusCompatibility ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app(
			WindowSettings( 640, 480, "eepp - Dropdown Placement Test", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ),
			UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
									 density ) );
		auto* ui = app.getUI();
		ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto* window = UIWindow::New();
		window->setParent( ui->getRoot() );
		window->setPosition( 20, 15 );
		window->setSize( 250, 180 );
		for ( bool modelBased : { false, true } ) {
			UIDropDown* dropdown;
			UIWidget* popup;
			if ( modelBased ) {
				auto* modelDropdown = UIDropDownModelList::New();
				modelDropdown->setModel( ItemListOwnerModel<std::string>::create(
					{ "A reasonably wide dropdown item", "Second item", "Third item" } ) );
				modelDropdown->getListView()->setRowHeight( 20 );
				dropdown = modelDropdown;
				popup = modelDropdown->getListView();
			} else {
				auto* listDropdown = UIDropDownList::New();
				listDropdown->getListBox()->addListBoxItems(
					{ "A reasonably wide dropdown item", "Second item", "Third item" } );
				listDropdown->getListBox()->setRowHeight( 20 );
				dropdown = listDropdown;
				popup = listDropdown->getListBox();
			}
			dropdown->setParent( window->getContainer() );
			dropdown->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			dropdown->setSize( 100, 20 );
			dropdown->setPosition( 30, 30 );
			dropdown->setMaxNumVisibleItems( 3 );
			const Rectf bounds( ui->getWorldBounds() );
			for ( bool toRoot : { false, true } ) {
				dropdown->setPopUpToRoot( toRoot );
				for ( auto mode :
					  { UIDropDown::MenuWidthMode::DropDown, UIDropDown::MenuWidthMode::Contents,
						UIDropDown::MenuWidthMode::ContentsCentered,
						UIDropDown::MenuWidthMode::ExpandIfNeeded,
						UIDropDown::MenuWidthMode::ExpandIfNeededCentered } ) {
					dropdown->setMenuWidthMode( mode );
					dropdown->showList();
					const Rectf field( dropdown->getScreenRect() );
					const Float width = popup->getPixelsSize().getWidth();
					const bool centered = mode == UIDropDown::MenuWidthMode::ContentsCentered ||
										  mode == UIDropDown::MenuWidthMode::ExpandIfNeededCentered;
					Float x = centered ? field.Left + eefloor( ( field.getWidth() - width ) / 2 )
									   : field.Left;
					x = eeclamp( x, bounds.Left, eemax( bounds.Left, bounds.Right - width ) );
					EXPECT_TRUE( popup->getScreenPos() == Vector2f( x, field.Bottom ) );
					EXPECT_TRUE( popup->getParent() ==
								 ( toRoot ? ui->getRoot() : window->getContainer() ) );
					EXPECT_TRUE( popup->isVisible() );
					EXPECT_TRUE( popup->isEnabled() );
					EXPECT_TRUE( popup->hasFocus() );
					if ( mode == UIDropDown::MenuWidthMode::DropDown )
						EXPECT_EQ( width, field.getWidth() );
					popup->setVisible( false );
				}
			}
			// A list wider than the scene still belongs below the field when its height fits.
			dropdown->setMenuWidthMode( UIDropDown::MenuWidthMode::DropDown );
			dropdown->setPixelsSize( bounds.getWidth() + 100, 40 );
			dropdown->showList();
			EXPECT_TRUE( popup->getScreenPos() ==
						 Vector2f( bounds.Left, dropdown->getScreenRect().Bottom ) );
			popup->setVisible( false );
			dropdown->setSize( 100, 20 );
			Vector2f bottomPosition( bounds.Right - dropdown->getPixelsSize().getWidth(),
									 bounds.Bottom - dropdown->getPixelsSize().getHeight() );
			dropdown->getParent()->worldToNode( bottomPosition );
			dropdown->setPosition( bottomPosition );
			dropdown->showList();
			EXPECT_EQ( popup->getScreenRect().Bottom, dropdown->getScreenRect().Top );
			EXPECT_EQ( popup->getScreenRect().Right, bounds.Right );
			popup->getFirstChild()->setFocus();
			EXPECT_TRUE( popup->isVisible() );
			dropdown->setFocus();
			EXPECT_TRUE( popup->isVisible() );
			popup->setFocus();
			auto* other = UITextInput::New();
			other->setParent( ui->getRoot() );
			other->setFocus();
			EXPECT_FALSE( popup->isVisible() );
			EXPECT_FALSE( popup->isEnabled() );
			eeDelete( dropdown );
			eeDelete( other );
		}
	}
}

UTEST( UIDropDown, ReopenDuringFadeDoesNotHideTheNewPopup ) {
	UIApplication app(
		WindowSettings( 640, 480, "eepp - Dropdown Fade Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* ui = app.getUI();
	ui->getUIThemeManager()->setDefaultEffectsEnabled( true );
	ui->getUIThemeManager()->setWidgetsFadeOutTime( Milliseconds( 50 ) );
	auto* dropdown = UIDropDownList::New();
	dropdown->setParent( ui->getRoot() );
	dropdown->setSize( 100, 20 );
	dropdown->getListBox()->addListBoxItems( { "First item", "Second item" } );
	auto* popup = dropdown->getListBox();
	auto* other = UITextInput::New();
	other->setParent( ui->getRoot() );
	dropdown->showList();
	EXPECT_TRUE( popup->isVisible() );
	other->setFocus();
	EXPECT_FALSE( popup->isEnabled() );
	EXPECT_TRUE( popup->isVisible() );
	dropdown->showList();
	// A visible fading popup first toggles closed; reopen after its fade completes.
	for ( int i = 0; i < 3; ++i )
		ui->getActionManager()->update( Milliseconds( 100 ) );
	dropdown->showList();
	other->setFocus();
	popup->setVisible( false );
	dropdown->showList();
	for ( int i = 0; i < 3; ++i )
		ui->getActionManager()->update( Milliseconds( 100 ) );
	EXPECT_TRUE( popup->isVisible() );
	EXPECT_TRUE( popup->isEnabled() );
	EXPECT_EQ( popup->getAlpha(), 255.f );
}

UTEST( UIDropDown, ReopenAfterDisablingEffectsRestoresOpacity ) {
	UIApplication app(
		WindowSettings( 640, 480, "eepp - Dropdown Effects Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* ui = app.getUI();
	ui->getUIThemeManager()->setDefaultEffectsEnabled( true );
	ui->getUIThemeManager()->setWidgetsFadeOutTime( Milliseconds( 50 ) );
	auto* dropdown = UIDropDownList::New();
	dropdown->setParent( ui->getRoot() );
	dropdown->setSize( 100, 20 );
	dropdown->getListBox()->addListBoxItems( { "First item", "Second item" } );
	auto* other = UITextInput::New();
	other->setParent( ui->getRoot() );
	dropdown->showList();
	other->setFocus();
	for ( int i = 0; i < 3; ++i )
		ui->getActionManager()->update( Milliseconds( 100 ) );
	auto* popup = dropdown->getListBox();
	EXPECT_FALSE( popup->isVisible() );
	EXPECT_EQ( popup->getAlpha(), 0.f );
	ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
	dropdown->showList();
	EXPECT_TRUE( popup->isVisible() );
	EXPECT_TRUE( popup->isEnabled() );
	EXPECT_EQ( popup->getAlpha(), 255.f );
}
