#include "utest.h"

#include "../../eepp/ui/accessibility/accessibilitybackend.hpp"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/models/persistentmodelindex.hpp>
#include <eepp/ui/models/stringmapmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitreeview.hpp>

#include <cstdlib>
#include <future>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

namespace {

UIApplication::Settings accessibilityTestSettings() {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	return settings;
}

WindowSettings accessibilityTestWindow() {
	return WindowSettings( 320, 240, "Accessibility Regression Test", WindowStyle::Default,
						   WindowBackend::Default, 32, {}, 1, false, true );
}

class AccessibilityRowsModel : public Model {
  public:
	explicit AccessibilityRowsModel( size_t rows = 4 ) {
		for ( size_t index = 0; index < rows; ++index )
			mRows.emplace_back( static_cast<int>( index + 1 ) * 10 );
	}

	size_t rowCount( const ModelIndex& = {} ) const override { return mRows.size(); }

	size_t columnCount( const ModelIndex& = {} ) const override { return 1; }

	Variant data( const ModelIndex& index, ModelRole = ModelRole::Display ) const override {
		return mRows[index.row()];
	}

	size_t persistentCount() const { return mPersistentHandles.size(); }

	void insertAt( int row, int value ) {
		beginInsertRows( {}, row, row );
		mRows.insert( mRows.begin() + row, value );
		endInsertRows();
		invalidate( Model::DontInvalidateIndexes );
	}

	void removeAt( int row ) {
		if ( beginDeleteRows( {}, row, row ) ) {
			mRows.erase( mRows.begin() + row );
			endDeleteRows();
			invalidate( Model::DontInvalidateIndexes );
		}
	}

  private:
	std::vector<int> mRows;
};

class AccessibilityTreeModel : public AccessibilityRowsModel {
  public:
	AccessibilityTreeModel() : AccessibilityRowsModel( 32 ) {}

	size_t rowCount( const ModelIndex& parent = {} ) const override {
		if ( !parent.isValid() )
			return AccessibilityRowsModel::rowCount();
		return parent.internalData() == nullptr && parent.row() == 0 ? 1 : 0;
	}

	ModelIndex index( int row, int column = 0, const ModelIndex& parent = {} ) const override {
		return createIndex( row, column, parent.isValid() ? this : nullptr );
	}

	ModelIndex parentIndex( const ModelIndex& index ) const override {
		return index.internalData() != nullptr ? this->index( 0 ) : ModelIndex{};
	}
};

} // namespace

UTEST( Accessibility, CompositeFocusProjectsToAnExposedElement ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* group = UIWidget::New();
	group->setAccessibilityRole( AccessibilityRole::Group );
	group->setParent( scene->getRoot() );
	auto* spin = UISpinBox::New();
	spin->setParent( group );
	spin->getTextInput()->setFocus();
	auto ref = manager->getNodeRef( spin );
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == ref );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( ref ).states ) &
				 static_cast<Uint64>( AccessibilityState::Focused ) );
	auto* button = UIPushButton::New();
	button->setParent( group );
	button->setFocus();
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == manager->getNodeRef( button ) );
}

UTEST( Accessibility, OnlyTheFocusOwnerReportsFocusedState ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* group = UIWidget::New();
	group->setAccessibilityRole( AccessibilityRole::Group );
	group->setParent( scene->getRoot() );
	auto* spin = UISpinBox::New();
	spin->setParent( group );
	spin->getTextInput()->setFocus();
	auto isFocused = [manager]( AccessibilityNodeRef ref ) {
		return ( static_cast<Uint64>( manager->getNodeInfo( ref ).states ) &
				 static_cast<Uint64>( AccessibilityState::Focused ) ) != 0;
	};
	// Containing the focus is not having it: ancestors must not claim the focused state.
	EXPECT_TRUE( isFocused( manager->getNodeRef( spin ) ) );
	EXPECT_FALSE( isFocused( manager->getNodeRef( group ) ) );
	EXPECT_FALSE( isFocused( manager->getRoot() ) );
}

UTEST( Accessibility, DeletingUnqueriedWidgetEmitsNoEvents ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto root = manager->getRoot();
	auto* seen = UIPushButton::New();
	seen->setParent( app.getUI()->getRoot() );
	// Created before any client was listening, so no Created event announced it.
	auto* unseen = UIPushButton::New();
	unseen->setParent( app.getUI()->getRoot() );
	manager->onNativeClientObserved();
	ASSERT_TRUE( manager->isValid( manager->getNodeRef( seen ) ) );
	manager->clearPendingEvents();
	eeDelete( unseen );
	// No client knows this widget, so its deletion must neither allocate an identity for it nor
	// report a tree change.
	EXPECT_TRUE( manager->getPendingEvents().empty() );
	EXPECT_EQ( manager->getChildCount( root ), 1u );
	// A known sibling is still reported, exactly once.
	eeDelete( seen );
	size_t destroyed = 0;
	for ( const auto& event : manager->getPendingEvents() )
		destroyed += event.type == AccessibilityEvent::Destroyed && event.related.isValid();
	EXPECT_EQ( destroyed, 1u );
}

UTEST( Accessibility, ClosingNestedSceneNotifiesItsHostOnce ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	manager->onNativeClientObserved();
	auto root = manager->getRoot();
	auto* nested = UISceneNode::New( app.getWindow() );
	nested->setParent( scene->getRoot() );
	auto* first = UIPushButton::New();
	first->setParent( nested->getRoot() );
	auto* second = UIPushButton::New();
	second->setParent( nested->getRoot() );
	auto firstRef = manager->getNodeRef( first );
	auto secondRef = manager->getNodeRef( second );
	bool closingSceneHidesManager = false;
	first->addEventListener( Event::OnClose, [&]( const Event* ) {
		const auto* closing = static_cast<const UISceneNode*>( nested );
		closingSceneHidesManager = closing->getAccessibilityManager() == nullptr &&
								   nested->getAccessibilityManager() == nullptr;
	} );
	manager->clearPendingEvents();
	eeDelete( nested );
	EXPECT_TRUE( closingSceneHidesManager );
	// The host learns about the removal once; descendant destructors of the closing scene must
	// not reach its manager again.
	size_t childrenChanged = 0;
	bool firstDestroyed = false;
	bool secondDestroyed = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		if ( event.type == AccessibilityEvent::ChildrenChanged ) {
			EXPECT_TRUE( event.ref == root );
			++childrenChanged;
		} else if ( event.type == AccessibilityEvent::Destroyed ) {
			EXPECT_FALSE( event.related.isValid() );
			firstDestroyed |= event.ref == firstRef;
			secondDestroyed |= event.ref == secondRef;
		}
	}
	EXPECT_EQ( childrenChanged, 1u );
	EXPECT_TRUE( firstDestroyed );
	EXPECT_TRUE( secondDestroyed );
	EXPECT_EQ( manager->getChildCount( root ), 0u );
}

UTEST( Accessibility, CellEditorIsExposedBelowItsVirtualRow ) {
	class EditableRowsModel final : public AccessibilityRowsModel {
	  public:
		bool isEditable( const ModelIndex& ) const override { return true; }
	};
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	manager->onNativeClientObserved();
	auto model = std::make_shared<EditableRowsModel>();
	auto* list = UIListView::New();
	list->setParent( app.getUI()->getRoot() );
	list->setModel( model );
	list->setEditable( true );
	list->onCreateEditingDelegate = []( const ModelIndex& ) -> ModelEditingDelegate* {
		return StringModelEditingDelegate::New();
	};
	auto* cell = UIWidget::New();
	cell->setParent( list );
	auto host = manager->getNodeRef( list );
	auto row = manager->getChild( host, 1 );
	ASSERT_TRUE( manager->isValid( row ) );
	EXPECT_EQ( manager->getChildCount( row ), 0u );

	list->beginEditing( model->index( 1 ), cell );
	ASSERT_NE( list->getEditWidget(), nullptr );
	auto editor = manager->getNodeRef( list->getEditWidget() );
	ASSERT_EQ( manager->getChildCount( row ), 1u );
	EXPECT_TRUE( manager->getChild( row, 0 ) == editor );
	EXPECT_TRUE( manager->getParent( editor ) == row );
	EXPECT_EQ( manager->getIndexInParent( editor ), 0 );
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == editor );
	EXPECT_EQ( manager->getNodeInfo( editor ).role, AccessibilityRole::TextBox );

	manager->clearPendingEvents();
	list->stopEditing();
	EXPECT_FALSE( manager->isValid( editor ) );
	EXPECT_EQ( manager->getChildCount( row ), 0u );
	bool rowChanged = false;
	for ( const auto& event : manager->getPendingEvents() )
		rowChanged |= event.type == AccessibilityEvent::ChildrenChanged && event.ref == row;
	EXPECT_TRUE( rowChanged );
}

UTEST( Accessibility, SceneResizeAnnouncesOneLayoutChange ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* button = UIPushButton::New();
	button->setParent( scene->getRoot() );
	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	scene->setPixelsSize( scene->getPixelsSize() + Sizef( 16, 16 ) );
	// Widgets moved by the relayout stay silent; the window root reports the change once.
	size_t bounds = 0;
	for ( const auto& event : manager->getPendingEvents() ) {
		if ( event.type == AccessibilityEvent::BoundsChanged ) {
			EXPECT_TRUE( event.ref == manager->getRoot() );
			++bounds;
		}
	}
	EXPECT_EQ( bounds, 1u );
}

UTEST( Accessibility, UnchangedAccessibilityPropertiesStaySilent ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* button = UIPushButton::New();
	button->setParent( scene->getRoot() );
	manager->onNativeClientObserved();
	button->setAccessibilityLabel( "Save" );
	button->setAccessibilityDescription( "Saves the item" );
	button->setAccessibilityHidden( true );
	button->setAccessibilityRole( AccessibilityRole::Group );
	manager->clearPendingEvents();
	// Restyling re-applies the same values; none of them may reach the client again.
	button->applyProperty( CSS::StyleSheetProperty( "aria-label", "Save" ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-description", "Saves the item" ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "true" ) );
	button->setAccessibilityRole( AccessibilityRole::Group );
	EXPECT_EQ( manager->getPendingEvents().size(), 0u );
}

UTEST( Accessibility, RepeatedStateAndStructuralEventsKeepTheirOrder ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	manager->update();
	// Events are recorded only for a client that can receive them.
	manager->onNativeClientObserved();
	auto root = manager->getRoot();
	auto* checkbox = UICheckBox::New();
	checkbox->setParent( app.getUI()->getRoot() );
	auto ref = manager->getNodeRef( checkbox );
	manager->clearPendingEvents();
	manager->notify( root, AccessibilityEvent::Created, ref, 0 );
	manager->notify( root, AccessibilityEvent::Destroyed, ref, 0 );
	manager->notify( root, AccessibilityEvent::Created, ref, 0 );
	manager->notify( ref, AccessibilityEvent::StateChanged );
	manager->notify( ref, AccessibilityEvent::StateChanged );
	const auto& events = manager->getPendingEvents();
	ASSERT_EQ( events.size(), 5u );
	EXPECT_EQ( events[0].type, AccessibilityEvent::Created );
	EXPECT_EQ( events[1].type, AccessibilityEvent::Destroyed );
	EXPECT_EQ( events[2].type, AccessibilityEvent::Created );
	EXPECT_EQ( events[3].type, AccessibilityEvent::StateChanged );
	EXPECT_EQ( events[4].type, AccessibilityEvent::StateChanged );
	manager->notify( ref, AccessibilityEvent::ModelChanged );
	manager->notify( ref, AccessibilityEvent::ModelChanged );
	EXPECT_EQ( events.size(), 6u );
	EXPECT_EQ( events.back().type, AccessibilityEvent::ModelChanged );
}

UTEST( Accessibility, DeletingUnqueriedContainerInvalidatesItsQueriedDescendants ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto root = manager->getRoot();
	auto* container = UIWidget::New();
	container->setParent( app.getUI()->getRoot() );
	auto* group = UIWidget::New();
	group->setAccessibilityRole( AccessibilityRole::Group );
	group->setParent( container );
	auto* button = UIPushButton::New();
	button->setParent( group );
	auto groupRef = manager->getNodeRef( group );
	auto buttonRef = manager->getNodeRef( button );
	manager->clearPendingEvents();
	eeDelete( container );
	EXPECT_FALSE( manager->isValid( groupRef ) );
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	EXPECT_EQ( manager->getChildCount( root ), 0u );
	for ( const auto& event : manager->getPendingEvents() ) {
		if ( event.related.isValid() )
			EXPECT_TRUE( manager->isValid( event.ref ) );
	}
}

UTEST( Accessibility, ModelQueriesAreLazyAndIdentitiesFollowInsertedRows ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto model = std::make_shared<AccessibilityRowsModel>( 100000 );
	auto* table = UITableView::New();
	table->setParent( app.getUI()->getRoot() );
	table->setModel( model );
	auto host = manager->getNodeRef( table );
	EXPECT_EQ( manager->getChildCount( host ), 100000u );
	EXPECT_EQ( model->persistentCount(), 0u );
	// Exercise the view's real notification path while keeping this test independent of an OS
	// screen reader. Native observation enables the scene's notification gate only.
	manager->onNativeClientObserved();
	table->getSelection().set( model->index( 99999, 0 ) );
	auto selected = manager->getSelectedChildren( host );
	ASSERT_EQ( selected.size(), 1u );
	EXPECT_EQ( model->persistentCount(), 1u );
	auto last = manager->getChild( host, 99999 );
	EXPECT_TRUE( selected.front() == last );
	EXPECT_EQ( model->persistentCount(), 1u );
	EXPECT_EQ( manager->getIndexInParent( last ), 99999 );
	EXPECT_TRUE( manager->getNodeInfo( last ).name == String( "1000000" ) );

	auto third = manager->getChild( host, 2 );
	model->insertAt( 1, 15 );
	EXPECT_TRUE( manager->getChild( host, 3 ) == third );
	EXPECT_TRUE( manager->getChild( host, 2 ) != third );
	EXPECT_EQ( manager->getIndexInParent( third ), 3 );
	EXPECT_TRUE( manager->getNodeInfo( third ).name == String( "30" ) );
	model->removeAt( 0 );
	EXPECT_TRUE( manager->getChild( host, 2 ) == third );
	EXPECT_EQ( manager->getIndexInParent( third ), 2 );
	model->removeAt( 2 );
	EXPECT_FALSE( manager->isValid( third ) );
}

UTEST( Accessibility, CollapsedRowsDoNotMaterializeTheirFormerSiblings ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	manager->onNativeClientObserved();
	auto model = std::make_shared<AccessibilityTreeModel>();
	auto* tree = UITreeView::New();
	tree->setParent( app.getUI()->getRoot() );
	tree->setModel( model );
	auto host = manager->getNodeRef( tree );
	tree->setExpanded( model->index( 0 ), true );
	EXPECT_EQ( manager->getChildCount( host ), 33u );
	EXPECT_EQ( model->persistentCount(), 0u );
	auto leaf = manager->getChild( host, 1 );
	EXPECT_EQ( manager->getIndexInParent( leaf ), 1 );
	EXPECT_EQ( model->persistentCount(), 1u );
	tree->setExpanded( model->index( 0 ), false );
	EXPECT_EQ( manager->getIndexInParent( leaf ), -1 );
	EXPECT_EQ( model->persistentCount(), 1u );
}

UTEST( Accessibility, CloseCallbacksCannotRetainDeletedDescendantIdentities ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto* container = UIWidget::New();
	container->setParent( app.getUI()->getRoot() );
	auto* button = UIPushButton::New();
	button->setParent( container );
	auto ref = manager->getNodeRef( button );
	bool invalidatedBeforeCallback = false;
	bool cannotRecreateIdentities = false;
	container->addEventListener( Event::OnClose, [&]( const Event* ) {
		invalidatedBeforeCallback = !manager->isValid( ref );
		cannotRecreateIdentities = !manager->getNodeRef( button ).isValid() &&
								   manager->getChildren( manager->getRoot() ).empty();
		eeDelete( button );
	} );
	eeDelete( container );
	EXPECT_TRUE( invalidatedBeforeCallback );
	EXPECT_TRUE( cannotRecreateIdentities );
	EXPECT_FALSE( manager->isValid( ref ) );
}

UTEST( Accessibility, DeletingNodeCanRegisterPreviouslyUnqueriedParent ) {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	UIApplication app( WindowSettings( 320, 240, "Accessibility Lazy Parent Test",
									   WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
									   false, true ),
					   settings );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto* button = UIPushButton::New();
	button->setParent( app.getUI()->getRoot() );
	auto ref = manager->getNodeRef( button );
	ASSERT_TRUE( manager->isValid( ref ) );

	// Only the button was queried. Deletion must tolerate getParent() registering the root
	// and reallocating the dense identity map before it erases the button.
	eeDelete( button );
	EXPECT_FALSE( manager->isValid( ref ) );
	EXPECT_TRUE( manager->isValid( manager->getRoot() ) );
}

UTEST( Accessibility, NestedSceneProjectionAndDeletionUseHostManager ) {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	UIApplication app( WindowSettings( 320, 240, "Accessibility Nested Scene Test",
									   WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
									   false, true ),
					   settings );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto root = manager->getRoot();
	auto* nested = UISceneNode::New( app.getWindow() );
	nested->setParent( scene->getRoot() );
	auto* button = UIPushButton::New();
	button->setText( "Nested button" );
	button->setParent( nested->getRoot() );
	auto buttonRef = manager->getNodeRef( button );
	ASSERT_TRUE( manager->isValid( buttonRef ) );
	EXPECT_TRUE( nested->getAccessibilityManager() == manager );
	EXPECT_EQ( nested->getRoot()->getAccessibilityRole(), AccessibilityRole::None );
	EXPECT_EQ( manager->getChildCount( root ), 1u );
	EXPECT_TRUE( manager->getChild( root, 0 ) == buttonRef );
	EXPECT_TRUE( manager->getParent( buttonRef ) == root );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Nested button" ) );
	button->setFocus();
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == buttonRef );

	auto* table = UITableView::New();
	table->setParent( nested->getRoot() );
	table->setModel(
		ItemPairListOwnerModel<std::string, std::string>::create( { { "Name", "Value" } } ) );
	auto tableRef = manager->getNodeRef( table );
	auto rowRef = manager->getChild( tableRef, 0 );
	ASSERT_TRUE( manager->isValid( rowRef ) );

	eeDelete( nested );
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	EXPECT_FALSE( manager->isValid( tableRef ) );
	EXPECT_FALSE( manager->isValid( rowRef ) );
	EXPECT_EQ( manager->getChildCount( root ), 0u );
}

UTEST( Accessibility, SceneRebindingInvalidatesWidgetAndModelSubtree ) {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	UIApplication app( WindowSettings( 320, 240, "Accessibility Scene Rebinding Test",
									   WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
									   false, true ),
					   settings );
	ASSERT_NE( app.getUI(), nullptr );
	auto* sceneA = app.getUI();
	auto* sceneB = UISceneNode::New( app.getWindow() );
	Scene::SceneManager::instance()->add( sceneB );
	sceneB->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto* managerA = sceneA->getAccessibilityManager();
	auto* managerB = sceneB->getAccessibilityManager();
	auto rootA = managerA->getRoot();
	auto rootB = managerB->getRoot();
	auto* container = UIWidget::New();
	container->setAccessibilityRole( AccessibilityRole::Group );
	container->setParent( sceneA->getRoot() );
	auto* button = UIPushButton::New();
	button->setParent( container );
	auto* table = UITableView::New();
	table->setParent( container );
	table->setModel(
		ItemPairListOwnerModel<std::string, std::string>::create( { { "Name", "Value" } } ) );
	auto buttonRefA = managerA->getNodeRef( button );
	auto containerRefA = managerA->getNodeRef( container );
	auto tableRefA = managerA->getNodeRef( table );
	auto rowRefA = managerA->getChild( tableRefA, 0 );
	ASSERT_TRUE( managerA->isValid( rowRefA ) );

	container->setParent( sceneB->getRoot() );
	EXPECT_FALSE( managerA->isValid( buttonRefA ) );
	EXPECT_FALSE( managerA->isValid( containerRefA ) );
	EXPECT_FALSE( managerA->isValid( tableRefA ) );
	EXPECT_FALSE( managerA->isValid( rowRefA ) );
	EXPECT_FALSE( managerA->getNodeRef( button ).isValid() );
	EXPECT_EQ( managerA->getChildCount( rootA ), 0u );
	auto buttonRefB = managerB->getNodeRef( button );
	auto containerRefB = managerB->getNodeRef( container );
	auto tableRefB = managerB->getNodeRef( table );
	auto rowRefB = managerB->getChild( tableRefB, 0 );
	EXPECT_TRUE( managerB->isValid( buttonRefB ) );
	EXPECT_TRUE( managerB->isValid( rowRefB ) );
	EXPECT_EQ( managerB->getChildCount( rootB ), 1u );
	EXPECT_TRUE( managerB->getChild( rootB, 0 ) == containerRefB );
	EXPECT_EQ( managerB->getChildCount( containerRefB ), 2u );

	eeDelete( container );
	EXPECT_FALSE( managerA->isValid( buttonRefA ) );
	EXPECT_FALSE( managerA->isValid( rowRefA ) );
	EXPECT_FALSE( managerB->isValid( buttonRefB ) );
	EXPECT_FALSE( managerB->isValid( containerRefB ) );
	EXPECT_FALSE( managerB->isValid( rowRefB ) );
}

UTEST( Accessibility, NestedSceneRebindingInvalidatesPreviousHost ) {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	UIApplication app( WindowSettings( 320, 240, "Accessibility Nested Rebinding Test",
									   WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
									   false, true ),
					   settings );
	ASSERT_NE( app.getUI(), nullptr );
	auto* sceneA = app.getUI();
	auto* sceneB = UISceneNode::New( app.getWindow() );
	Scene::SceneManager::instance()->add( sceneB );
	sceneB->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto* managerA = sceneA->getAccessibilityManager();
	auto* managerB = sceneB->getAccessibilityManager();
	auto* nested = UISceneNode::New( app.getWindow() );
	nested->setParent( sceneA->getRoot() );
	auto* group = UIWidget::New();
	group->setAccessibilityRole( AccessibilityRole::Group );
	group->setParent( nested->getRoot() );
	auto* button = UIPushButton::New();
	button->setParent( group );
	auto refA = managerA->getNodeRef( button );
	auto groupRefA = managerA->getNodeRef( group );
	ASSERT_TRUE( managerA->isValid( refA ) );

	// The local UISceneNode remains the same; only its host accessibility owner changes.
	nested->setParent( sceneB->getRoot() );
	EXPECT_TRUE( button->getUISceneNode() == nested );
	EXPECT_TRUE( nested->getAccessibilityManager() == managerB );
	EXPECT_FALSE( managerA->isValid( refA ) );
	EXPECT_FALSE( managerA->isValid( groupRefA ) );
	auto refB = managerB->getNodeRef( button );
	ASSERT_TRUE( managerB->isValid( refB ) );
	eeDelete( nested );
	EXPECT_FALSE( managerB->isValid( refB ) );
}

UTEST( Accessibility, RebindingWithinHostPreservesIdentity ) {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	UIApplication app( WindowSettings( 320, 240, "Accessibility Shared Owner Test",
									   WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
									   false, true ),
					   settings );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* nested = UISceneNode::New( app.getWindow() );
	nested->setParent( scene->getRoot() );
	auto* button = UIPushButton::New();
	button->setParent( scene->getRoot() );
	auto ref = manager->getNodeRef( button );
	button->setParent( nested->getRoot() );
	EXPECT_TRUE( manager->isValid( ref ) );
	EXPECT_TRUE( manager->getNodeRef( button ) == ref );
	button->setParent( scene->getRoot() );
	EXPECT_TRUE( manager->getNodeRef( button ) == ref );
	eeDelete( button );
	EXPECT_FALSE( manager->isValid( ref ) );
}

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD
UTEST( Accessibility, QueuedNativeInitializationOutlivesRemovedWindows ) {
	if ( std::getenv( "EEPP_DISABLE_ACCESSIBILITY" ) )
		UTEST_SKIP( "Native accessibility is disabled by the environment" );
	auto pool = ThreadPool::createShared( 1 );
	std::promise<void> release;
	auto ready = release.get_future().share();
	pool->run( [ready] { ready.wait(); } );
	{
		// Keep initialization queued until both window-owned scenes and Inputs are destroyed.
		// Release the worker even when an assertion returns early from this test.
		ScopedOp unblock( nullptr, [&release] { release.set_value(); } );
		UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
										  1 );
		settings.accessibilityPolicy = AccessibilityPolicy::Enabled;
		settings.enableSystemFonts = false;
		settings.threadPool = pool;
		UIApplication app( WindowSettings( 320, 240, "Accessibility Queued Init Test",
										   WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
										   false, true ),
						   settings );
		ASSERT_NE( app.getUI(), nullptr );
		auto* manager = app.getUI()->getAccessibilityManager();
		manager->update();
		EXPECT_FALSE( manager->isBackendInitializationComplete() );
		auto* button = UIPushButton::New();
		auto ref = manager->getNodeRef( button );
		manager->notify( ref, AccessibilityEvent::ValueChanged );
		eeDelete( button );
		EXPECT_FALSE( manager->isValid( ref ) );
		auto* secondary = app.createWindow(
			WindowSettings( 320, 240, "Accessibility Queued Secondary", WindowStyle::Default,
							WindowBackend::Default, 32, {}, 1, false, true ) );
		ASSERT_NE( secondary, nullptr );
		secondary->getAccessibilityManager()->update();
		EXPECT_FALSE( secondary->getAccessibilityManager()->isBackendInitializationComplete() );
	}
	std::promise<void> finished;
	auto completion = finished.get_future();
	pool->run( [&finished] { finished.set_value(); } );
	EXPECT_TRUE( completion.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready );
	// Join before destroying the callback's promise, including on a failed timeout assertion.
	pool.reset();
}
#endif

UTEST( Accessibility, LiveProjectionIdentityActionsAndInvalidation ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Accessibility Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto scene = app.getUI();
	if ( !scene )
		UTEST_SKIP( "UIApplication initialization failed: a usable graphical display is required" );
	auto manager = scene->getAccessibilityManager();
	// Events are recorded only for a client that can receive them.
	manager->onNativeClientObserved();

	UIWidget* ignoredContainer = UIWidget::New();
	ignoredContainer->setParent( scene->getRoot() );
	UIPushButton* button = UIPushButton::New();
	button->setText( "Save" );
	button->setParent( ignoredContainer );
	UICheckBox* checkbox = UICheckBox::New();
	checkbox->setText( "Autosave" );
	checkbox->setParent( scene->getRoot() );

	auto root = manager->getRoot();
	auto buttonRef = manager->getNodeRef( button );
	auto checkboxRef = manager->getNodeRef( checkbox );
	EXPECT_TRUE( root.isValid() );
	EXPECT_EQ( manager->getNodeInfo( root ).role, AccessibilityRole::Application );
	EXPECT_TRUE( manager->getNodeInfo( root ).name == String( "eepp - Accessibility Test" ) );
	EXPECT_TRUE( buttonRef == manager->getNodeRef( button ) );
	EXPECT_EQ( manager->getChildCount( root ), 2u );
	EXPECT_TRUE( manager->getChild( root, 0 ) == buttonRef );
	auto rootChildren = manager->getChildren( root );
	EXPECT_EQ( rootChildren.size(), 2u );
	EXPECT_TRUE( rootChildren[0] == buttonRef );
	EXPECT_TRUE( rootChildren[1] == checkboxRef );
	EXPECT_TRUE( manager->getParent( buttonRef ) == root );
	ignoredContainer->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "true" ) );
	EXPECT_EQ( manager->getChildCount( root ), 1u );
	ignoredContainer->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "false" ) );

	auto buttonInfo = manager->getNodeInfo( buttonRef );
	EXPECT_EQ( buttonInfo.role, AccessibilityRole::Button );
	EXPECT_TRUE( buttonInfo.name == String( "Save" ) );
	EXPECT_TRUE( buttonInfo.actions & accessibilityActionMask( AccessibilityAction::Press ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-label", "Save item" ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-description", "Saves the item" ) );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Save item" ) );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).description == String( "Saves the item" ) );
	button->setAccessibilityLabel( {} );

	EXPECT_FALSE( checkbox->isChecked() );
	EXPECT_TRUE( manager->performAction( checkboxRef, { AccessibilityAction::Toggle, {} } ) );
	EXPECT_TRUE( checkbox->isChecked() );
	UISlider* slider = UISlider::NewHorizontal();
	slider->setMaxValue( 100.f );
	slider->setParent( scene->getRoot() );
	auto sliderRef = manager->getNodeRef( slider );
	auto sliderInfo = manager->getNodeInfo( sliderRef );
	EXPECT_TRUE( sliderInfo.range.valid );
	EXPECT_EQ( sliderInfo.range.minimum, 0. );
	EXPECT_EQ( sliderInfo.range.maximum, 100. );
	EXPECT_TRUE(
		manager->performAction( sliderRef, { AccessibilityAction::SetValue, String( "42" ) } ) );
	EXPECT_EQ( slider->getValue(), 42.f );
	UISelectButton* selectable = UISelectButton::New();
	selectable->setParent( scene->getRoot() );
	auto selectableRef = manager->getNodeRef( selectable );
	EXPECT_TRUE( manager->getNodeInfo( selectableRef ).actions &
				 accessibilityActionMask( AccessibilityAction::Select ) );
	EXPECT_TRUE(
		manager->performAction( selectableRef, { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( selectable->isSelected() );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( selectableRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::Selected ) );

	UISpinBox* spinBox = UISpinBox::New();
	spinBox->setParent( scene->getRoot() );
	auto spinBoxRef = manager->getNodeRef( spinBox );
	EXPECT_EQ( manager->getNodeInfo( spinBoxRef ).role, AccessibilityRole::SpinButton );
	EXPECT_TRUE(
		manager->performAction( spinBoxRef, { AccessibilityAction::SetValue, String( "7.5" ) } ) );
	EXPECT_EQ( spinBox->getValue(), 7.5 );

	UIProgressBar* progressBar = UIProgressBar::New();
	progressBar->setProgress( 25.f );
	progressBar->setParent( scene->getRoot() );
	auto progressInfo = manager->getNodeInfo( manager->getNodeRef( progressBar ) );
	EXPECT_EQ( progressInfo.role, AccessibilityRole::ProgressBar );
	EXPECT_TRUE( progressInfo.range.valid );
	Float progressValue = 0;
	EXPECT_TRUE( String::fromString( progressValue, progressInfo.value.toUtf8() ) );
	EXPECT_EQ( progressValue, 25.f );

	UITextInput* textInput = UITextInput::New();
	textInput->setText( "Project" );
	textInput->getDocument().setSelection( { 0, 3 } );
	textInput->setParent( scene->getRoot() );
	auto textInputRef = manager->getNodeRef( textInput );
	auto textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_TRUE( textInputInfo.text.valid );
	EXPECT_EQ( textInputInfo.text.caretOffset, 3 );
	EXPECT_EQ( textInputInfo.text.selectionStart, 3 );
	EXPECT_EQ( textInputInfo.text.selectionEnd, 3 );
	textInput->setText( String::fromUtf8( std::string( "A😀é" ) ) );
	textInput->getDocument().setSelection( { 0, 4 } );
	textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_EQ( textInputInfo.text.caretOffset, 4 );
	EXPECT_TRUE( textInputInfo.actions &
				 accessibilityActionMask( AccessibilityAction::SetTextSelection ) );
	EXPECT_TRUE( manager->performAction(
		textInputRef, { AccessibilityAction::SetTextSelection, String( "2:4" ) } ) );
	textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_EQ( textInputInfo.text.selectionStart, 2 );
	EXPECT_EQ( textInputInfo.text.selectionEnd, 4 );
	textInput->setMode( UITextInput::TextInputMode::Password );
	textInputInfo = manager->getNodeInfo( textInputRef );
	EXPECT_TRUE( static_cast<Uint64>( textInputInfo.states ) &
				 static_cast<Uint64>( AccessibilityState::Protected ) );
	EXPECT_FALSE( textInputInfo.text.valid );
	EXPECT_FALSE( textInputInfo.actions &
				  accessibilityActionMask( AccessibilityAction::SetTextSelection ) );

	UITextEdit* textEdit = UITextEdit::New();
	textEdit->setParent( scene->getRoot() );
	auto textEditRef = manager->getNodeRef( textEdit );
	EXPECT_EQ( manager->getNodeInfo( textEditRef ).role, AccessibilityRole::TextBox );
	EXPECT_TRUE( manager->performAction( textEditRef,
										 { AccessibilityAction::SetText, String( "Notes" ) } ) );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef ).value == String( "Notes" ) );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef ).name.empty() );
	EXPECT_TRUE( manager->getNodeInfo( textEditRef, false ).value.empty() );
	textEdit->setText( "first\nsecond" );
	textEdit->getDocument().setSelection( { 0, 2 }, { 1, 3 } );
	auto textEditInfo = manager->getNodeInfo( textEditRef );
	EXPECT_TRUE( textEditInfo.text.valid );
	EXPECT_EQ( textEditInfo.text.selectionStart, 2 );
	EXPECT_EQ( textEditInfo.text.selectionEnd, 9 );
	EXPECT_TRUE( manager->performAction(
		textEditRef, { AccessibilityAction::SetTextSelection, String( "6:8" ) } ) );
	auto textEditSelection = textEdit->getDocument().getSelection( true );
	EXPECT_EQ( textEditSelection.start().line(), 1 );
	EXPECT_EQ( textEditSelection.start().column(), 0 );
	EXPECT_EQ( textEditSelection.end().line(), 1 );
	EXPECT_EQ( textEditSelection.end().column(), 2 );
	EXPECT_TRUE( manager->performAction(
		textEditRef, { AccessibilityAction::SetTextSelection, String( "12:12" ) } ) );
	textEditSelection = textEdit->getDocument().getSelection( true );
	EXPECT_EQ( textEditSelection.start().line(), 1 );
	EXPECT_EQ( textEditSelection.start().column(), 6 );
	textEdit->setLocked( true );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( textEditRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::ReadOnly ) );
	EXPECT_FALSE( manager->performAction( textEditRef,
										  { AccessibilityAction::SetText, String( "Blocked" ) } ) );

	UIMenu* menu = UIMenu::New();
	menu->setParent( scene->getRoot() );
	UIMenuCheckBox* menuCheckBox = menu->addCheckBox( "Line numbers" );
	UIMenuRadioButton* menuRadioButton = menu->addRadioButton( "Dark theme" );
	auto menuCheckBoxRef = manager->getNodeRef( menuCheckBox );
	auto menuRadioButtonRef = manager->getNodeRef( menuRadioButton );
	EXPECT_EQ( manager->getNodeInfo( menuCheckBoxRef ).role, AccessibilityRole::CheckMenuItem );
	EXPECT_TRUE(
		manager->performAction( menuCheckBoxRef, { AccessibilityAction::Toggle, String() } ) );
	EXPECT_TRUE( menuCheckBox->isActive() );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( menuCheckBoxRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::Checked ) );
	EXPECT_TRUE(
		manager->performAction( menuRadioButtonRef, { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( menuRadioButton->isActive() );

	UIComboBox* comboBox = UIComboBox::New();
	comboBox->setParent( scene->getRoot() );
	comboBox->getListBox()->addListBoxItem( "First" );
	comboBox->setText( "First" );
	auto comboBoxRef = manager->getNodeRef( comboBox );
	auto comboBoxInfo = manager->getNodeInfo( comboBoxRef );
	EXPECT_EQ( comboBoxInfo.role, AccessibilityRole::ComboBox );
	EXPECT_TRUE( comboBoxInfo.value == String( "First" ) );
	EXPECT_TRUE( comboBoxInfo.actions & accessibilityActionMask( AccessibilityAction::Expand ) );
	EXPECT_TRUE( manager->performAction( comboBoxRef, { AccessibilityAction::Expand, String() } ) );
	comboBoxInfo = manager->getNodeInfo( comboBoxRef );
	EXPECT_TRUE( static_cast<Uint64>( comboBoxInfo.states ) &
				 static_cast<Uint64>( AccessibilityState::Expanded ) );
	EXPECT_TRUE( comboBoxInfo.actions & accessibilityActionMask( AccessibilityAction::Collapse ) );
	EXPECT_TRUE(
		manager->performAction( comboBoxRef, { AccessibilityAction::Collapse, String() } ) );

	UITabWidget* tabs = UITabWidget::New();
	tabs->setParent( scene->getRoot() );
	UITab* firstTab = tabs->add( "General", UIWidget::New() );
	UITab* secondTab = tabs->add( "Advanced", UIWidget::New() );
	tabs->setTabSelected( secondTab );
	auto firstTabInfo = manager->getNodeInfo( manager->getNodeRef( firstTab ) );
	auto secondTabInfo = manager->getNodeInfo( manager->getNodeRef( secondTab ) );
	EXPECT_EQ( firstTabInfo.role, AccessibilityRole::Tab );
	EXPECT_FALSE( static_cast<Uint64>( firstTabInfo.states ) &
				  static_cast<Uint64>( AccessibilityState::Selected ) );
	EXPECT_TRUE( static_cast<Uint64>( secondTabInfo.states ) &
				 static_cast<Uint64>( AccessibilityState::Selected ) );
	EXPECT_TRUE( firstTabInfo.actions & accessibilityActionMask( AccessibilityAction::Select ) );
	EXPECT_TRUE( manager->performAction( manager->getNodeRef( firstTab ),
										 { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( tabs->getTabSelected() == firstTab );

	UITableView* table = UITableView::New();
	table->setParent( scene->getRoot() );
	auto tableModel = ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "Alpha", "One" }, { "Beta", "Two" } } );
	tableModel->setColumnName( 0, "Name" );
	tableModel->setColumnName( 1, "Value" );
	table->setModel( tableModel );
	auto tableRef = manager->getNodeRef( table );
	EXPECT_EQ( manager->getNodeInfo( tableRef ).role, AccessibilityRole::Table );
	EXPECT_EQ( manager->getChildCount( tableRef ), 2u );
	auto firstRowRef = manager->getChild( tableRef, 0 );
	EXPECT_TRUE( firstRowRef.source != tableRef.source );
	EXPECT_EQ( manager->getNodeInfo( firstRowRef ).role, AccessibilityRole::Row );
	EXPECT_EQ( manager->getChildCount( firstRowRef ), 2u );
	auto firstCellRef = manager->getChild( firstRowRef, 0 );
	EXPECT_EQ( manager->getNodeInfo( firstCellRef ).role, AccessibilityRole::Cell );
	EXPECT_TRUE( manager->getNodeInfo( firstCellRef ).name == String( "Name" ) );
	EXPECT_TRUE( manager->getNodeInfo( firstCellRef ).value == String( "Alpha" ) );
	EXPECT_TRUE( manager->getParent( firstCellRef ) == firstRowRef );
	EXPECT_TRUE( manager->performAction( firstRowRef, { AccessibilityAction::Select, String() } ) );
	EXPECT_TRUE( table->getSelection().containsRow( 0 ) );
	auto replacementModel = ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "Gamma", "Three" }, { "Delta", "Four" } } );
	replacementModel->setColumnName( 0, "Name" );
	replacementModel->setColumnName( 1, "Value" );
	table->setModel( replacementModel );
	manager->notify( tableRef, AccessibilityEvent::ModelChanged );
	EXPECT_FALSE( manager->isValid( firstRowRef ) );
	firstRowRef = manager->getChild( tableRef, 0 );
	EXPECT_TRUE( manager->getNodeInfo( firstRowRef ).name == String( "Gamma" ) );

	UIListView* list = UIListView::New();
	list->setParent( scene->getRoot() );
	list->setModel( ItemListOwnerModel<std::string>::create( { "Red", "Green", "Blue" } ) );
	auto listRef = manager->getNodeRef( list );
	EXPECT_EQ( manager->getNodeInfo( listRef ).role, AccessibilityRole::List );
	EXPECT_EQ( manager->getChildCount( listRef ), 3u );
	EXPECT_EQ( manager->getChildren( listRef ).size(), 3u );
	auto listItemRef = manager->getChild( listRef, 1 );
	EXPECT_TRUE( manager->getChild( listRef, 1 ) == listItemRef );
	EXPECT_EQ( manager->getNodeInfo( listItemRef ).role, AccessibilityRole::ListItem );
	EXPECT_TRUE( manager->getNodeInfo( listItemRef ).name == String( "Green" ) );
	EXPECT_TRUE(
		manager->performAction( listItemRef, { AccessibilityAction::ScrollTo, String() } ) );
	EXPECT_TRUE( list->getSelection().isEmpty() );

	UITreeView* tree = UITreeView::New();
	tree->setParent( scene->getRoot() );
	std::map<std::string, std::vector<std::string>> treeItems{ { "Parent", { "Child" } } };
	tree->setModel( StringMapModel<>::create( treeItems ) );
	auto treeRef = manager->getNodeRef( tree );
	EXPECT_EQ( manager->getNodeInfo( treeRef ).role, AccessibilityRole::Tree );
	auto treeItemRef = manager->getChild( treeRef, 0 );
	EXPECT_EQ( manager->getNodeInfo( treeItemRef ).role, AccessibilityRole::TreeItem );
	EXPECT_EQ( manager->getChildCount( treeRef ), 1u );
	EXPECT_EQ( manager->getChildCount( treeItemRef ), 0u );
	EXPECT_TRUE( manager->getParent( treeItemRef ) == treeRef );
	EXPECT_TRUE( manager->getNodeInfo( treeItemRef ).actions &
				 accessibilityActionMask( AccessibilityAction::Expand ) );
	EXPECT_TRUE( manager->performAction( treeItemRef, { AccessibilityAction::Expand, String() } ) );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeInfo( treeItemRef ).states ) &
				 static_cast<Uint64>( AccessibilityState::Expanded ) );
	EXPECT_EQ( manager->getChildCount( treeRef ), 2u );
	auto childTreeItemRef = manager->getChild( treeRef, 1 );
	EXPECT_EQ( manager->getNodeInfo( childTreeItemRef ).role, AccessibilityRole::TreeItem );
	EXPECT_TRUE( manager->getParent( childTreeItemRef ) == treeRef );
	EXPECT_TRUE(
		manager->performAction( treeItemRef, { AccessibilityAction::Collapse, String() } ) );
	EXPECT_EQ( manager->getChildCount( treeRef ), 1u );

	manager->clearPendingEvents();
	auto dynamicButton = UIPushButton::New();
	dynamicButton->setText( "Dynamic" );
	manager->clearPendingEvents();
	dynamicButton->setParent( ignoredContainer );
	manager->onWidgetParentChange( dynamicButton );
	auto dynamicButtonRef = manager->getNodeRef( dynamicButton );
	bool createdEventFound = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		createdEventFound |= event.ref == root && event.related == dynamicButtonRef &&
							 event.type == AccessibilityEvent::Created && event.index >= 0;
	}
	EXPECT_TRUE( createdEventFound );

	manager->clearPendingEvents();
	eeDelete( dynamicButton );
	bool removedEventFound = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		removedEventFound |= event.ref == root && event.related == dynamicButtonRef &&
							 event.type == AccessibilityEvent::Destroyed && event.index >= 0;
	}
	EXPECT_TRUE( removedEventFound );

	button->setAccessibilityLabel( "Save project" );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Save project" ) );
	// Repeated state notifications are never coalesced.
	manager->clearPendingEvents();
	manager->notify( buttonRef, AccessibilityEvent::NameChanged );
	manager->notify( buttonRef, AccessibilityEvent::NameChanged );
	EXPECT_EQ( manager->getPendingEvents().size(), 2u );

	eeDelete( button );
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	bool destroyedEventFound = false;
	for ( const auto& event : manager->getPendingEvents() )
		destroyedEventFound |=
			event.ref == buttonRef && event.type == AccessibilityEvent::Destroyed;
	EXPECT_TRUE( destroyedEventFound );
}

UTEST( Accessibility, ComboBoxStateNotificationsFollowPopupMutations ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - ComboBox Accessibility Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	ASSERT_NE( scene, nullptr );
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	scene->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* manager = scene->getAccessibilityManager();
	manager->onNativeClientObserved();
	auto* combo = UIComboBox::New();
	combo->setParent( scene->getRoot() );
	combo->getListBox()->addListBoxItem( "First" );
	const auto ref = manager->getNodeRef( combo );
	bool earlyStateNotification = false;
	size_t visibilityChanges = 0;
	const auto listener = combo->getListBox()->on( Event::OnVisibleChange, [&]( const auto* ) {
		++visibilityChanges;
		for ( const auto& event : manager->getPendingEvents() ) {
			if ( event.type == AccessibilityEvent::StateChanged )
				earlyStateNotification = true;
		}
	} );
	const auto hasStateNotification = [&] {
		for ( const auto& event : manager->getPendingEvents() ) {
			if ( event.ref == ref && event.type == AccessibilityEvent::StateChanged )
				return true;
		}
		return false;
	};
	manager->clearPendingEvents();
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Expand, {} } ) );
	EXPECT_TRUE( hasStateNotification() );
	manager->clearPendingEvents();
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Collapse, {} } ) );
	EXPECT_TRUE( hasStateNotification() );
	EXPECT_FALSE( earlyStateNotification );
	EXPECT_EQ( visibilityChanges, 2u );
	combo->getListBox()->removeEventListener( listener );

	scene->getUIThemeManager()->setDefaultEffectsEnabled( true );
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Expand, {} } ) );
	manager->clearPendingEvents();
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Collapse, {} } ) );
	EXPECT_TRUE( hasStateNotification() );
	EXPECT_TRUE( combo->getListBox()->isVisible() );
	EXPECT_FALSE( combo->getListBox()->isEnabled() );
	auto info = manager->getNodeInfo( ref );
	EXPECT_FALSE( static_cast<Uint64>( info.states ) &
				  static_cast<Uint64>( AccessibilityState::Expanded ) );
	EXPECT_TRUE( info.actions & accessibilityActionMask( AccessibilityAction::Expand ) );
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Expand, {} } ) );
	EXPECT_TRUE( combo->getListBox()->isEnabled() );
	info = manager->getNodeInfo( ref );
	EXPECT_TRUE( static_cast<Uint64>( info.states ) &
				 static_cast<Uint64>( AccessibilityState::Expanded ) );
}

UTEST( Accessibility, TableCellsFollowVisibleColumnOrder ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - Table Accessibility Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	ASSERT_NE( scene, nullptr );
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto* manager = scene->getAccessibilityManager();
	manager->onNativeClientObserved();
	auto* table = UITableView::New();
	table->setParent( scene->getRoot() );
	auto model = ItemPairListOwnerModel<std::string, std::string>::create( { { "Alpha", "One" } } );
	model->setColumnName( 0, "Name" );
	model->setColumnName( 1, "Value" );
	table->setModel( model );
	const auto tableRef = manager->getNodeRef( table );
	const auto rowRef = manager->getChild( tableRef, 0 );
	const auto nameRef = manager->getChild( rowRef, 0 );
	const auto valueRef = manager->getChild( rowRef, 1 );
	EXPECT_TRUE( manager->getNodeInfo( nameRef ).name == String( "Name" ) );
	EXPECT_TRUE( manager->getNodeInfo( valueRef ).name == String( "Value" ) );
	manager->clearPendingEvents();
	EXPECT_TRUE( table->setColumnOrder( { 1, 0 } ) );
	bool childrenChanged = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		if ( event.ref == tableRef && event.type == AccessibilityEvent::ChildrenChanged )
			childrenChanged = true;
	}
	EXPECT_TRUE( childrenChanged );
	EXPECT_TRUE( manager->getChild( rowRef, 0 ) == valueRef );
	EXPECT_TRUE( manager->getChild( rowRef, 1 ) == nameRef );
	table->setColumnHidden( 1, true );
	EXPECT_EQ( manager->getChildCount( rowRef ), 1u );
	EXPECT_TRUE( manager->getChild( rowRef, 0 ) == nameRef );
	table->setColumnHidden( 1, false );
	EXPECT_TRUE( table->moveColumn( 0, 0 ) );
	EXPECT_TRUE( manager->getChild( rowRef, 0 ) == nameRef );
	EXPECT_TRUE( manager->getChild( rowRef, 1 ) == valueRef );
}

UTEST( Accessibility, CodeEditorIsExposedAsEditableText ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* editor = UICodeEditor::New();
	editor->setParent( scene->getRoot() );
	editor->getDocument().textInput( "int main" );
	auto ref = manager->getNodeRef( editor );
	auto info = manager->getNodeInfo( ref );
	EXPECT_EQ( info.role, AccessibilityRole::TextBox );
	EXPECT_TRUE( info.value == String( "int main" ) );
	EXPECT_TRUE( info.text.valid );
	EXPECT_TRUE( ( static_cast<Uint64>( info.states ) &
				   static_cast<Uint64>( AccessibilityState::MultiLine ) ) != 0 );
	// SetText edits the document in place, so it stays undoable.
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::SetText, "return 0;" } ) );
	EXPECT_TRUE( editor->getDocument().getText() == String( "return 0;" ) );
	EXPECT_TRUE( editor->getDocument().hasUndo() );
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::SetTextSelection, "0:6" } ) );
	EXPECT_EQ( manager->getNodeInfo( ref ).text.selectionEnd, 6 );

	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	editor->getDocument().textInput( "x" );
	bool valueChanged = false;
	for ( const auto& event : manager->getPendingEvents() )
		valueChanged |= event.ref == ref && event.type == AccessibilityEvent::ValueChanged;
	EXPECT_TRUE( valueChanged );

	editor->setLocked( true );
	info = manager->getNodeInfo( ref );
	EXPECT_TRUE( ( static_cast<Uint64>( info.states ) &
				   static_cast<Uint64>( AccessibilityState::ReadOnly ) ) != 0 );
	EXPECT_FALSE( info.actions & accessibilityActionMask( AccessibilityAction::SetText ) );
}

UTEST( Accessibility, TooltipNamesIconOnlyControls ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* iconButton = UIPushButton::New();
	iconButton->setParent( scene->getRoot() );
	iconButton->setTooltipText( "Save" );
	auto iconRef = manager->getNodeRef( iconButton );
	EXPECT_TRUE( manager->getNodeInfo( iconRef ).name == String( "Save" ) );
	EXPECT_TRUE( manager->getNodeInfo( iconRef ).description.empty() );

	auto* textButton = UIPushButton::New();
	textButton->setParent( scene->getRoot() );
	textButton->setText( "Build" );
	textButton->setTooltipText( "Builds the project" );
	auto textRef = manager->getNodeRef( textButton );
	EXPECT_TRUE( manager->getNodeInfo( textRef ).name == String( "Build" ) );
	EXPECT_TRUE( manager->getNodeInfo( textRef ).description == String( "Builds the project" ) );

	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	iconButton->setTooltipText( "Save all" );
	iconButton->setTooltipText( "Save all" );
	size_t nameChanges = 0;
	for ( const auto& event : manager->getPendingEvents() )
		nameChanges += event.ref == iconRef && event.type == AccessibilityEvent::NameChanged;
	EXPECT_EQ( nameChanges, 1u );
}

UTEST( Accessibility, LabelledByAndDescribedByResolveFromTheScene ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* label = UITextView::New();
	label->setParent( scene->getRoot() );
	label->setId( "user-label" );
	label->setText( "User name" );
	auto* hint = UITextView::New();
	hint->setParent( scene->getRoot() );
	hint->setId( "user-hint" );
	hint->setText( "Your login" );
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	input->applyProperty( CSS::StyleSheetProperty( "aria-labelledby", "user-label" ) );
	input->applyProperty( CSS::StyleSheetProperty( "aria-describedby", "user-hint" ) );
	auto ref = manager->getNodeRef( input );
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == String( "User name" ) );
	EXPECT_TRUE( manager->getNodeInfo( ref ).description == String( "Your login" ) );
	// A missing target falls back to the widget's own name.
	input->setAccessibilityLabelledBy( "missing" );
	input->setAccessibilityLabel( "Login" );
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == String( "Login" ) );
	// Mutual references cannot recurse.
	label->setAccessibilityLabelledBy( "loop-input" );
	input->setId( "loop-input" );
	input->setAccessibilityLabelledBy( "user-label" );
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == String( "User name" ) );
}

UTEST( Accessibility, AnnouncementsAndLiveRegionsCoalescePerFrame ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	// Dormant: nothing is queued without a client.
	scene->announceForAccessibility( "Build finished" );
	EXPECT_EQ( manager->getPendingAnnouncements().size(), 0u );

	manager->onNativeClientObserved();
	scene->announceForAccessibility( "Build finished" );
	scene->announceForAccessibility( "" );
	scene->announceForAccessibility( "Ignored", AccessibilityLive::Off );
	ASSERT_EQ( manager->getPendingAnnouncements().size(), 1u );
	EXPECT_TRUE( manager->getPendingAnnouncements()[0].message == String( "Build finished" ) );
	manager->clearPendingEvents();

	auto* status = UILinearLayout::NewVertical();
	status->setParent( scene->getRoot() );
	status->applyProperty( CSS::StyleSheetProperty( "aria-live", "assertive" ) );
	auto* text = UITextView::New();
	text->setParent( status );
	text->setText( "Indexing 1%" );
	text->setText( "Indexing 2%" );
	ASSERT_EQ( manager->getPendingAnnouncements().size(), 1u );
	EXPECT_TRUE( manager->getPendingAnnouncements()[0].message == String( "Indexing 2%" ) );
	EXPECT_EQ( manager->getPendingAnnouncements()[0].priority, AccessibilityLive::Assertive );

	// Widgets outside a live region stay silent.
	manager->clearPendingEvents();
	auto* plain = UITextView::New();
	plain->setParent( scene->getRoot() );
	plain->setText( "Not live" );
	EXPECT_EQ( manager->getPendingAnnouncements().size(), 0u );

	// Content hidden from accessibility stays silent, whether the hiding is on the changed
	// widget or on an ancestor of the live region.
	auto* hiddenText = UITextView::New();
	hiddenText->setParent( status );
	hiddenText->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "true" ) );
	hiddenText->setText( "Hidden detail" );
	EXPECT_EQ( manager->getPendingAnnouncements().size(), 0u );
	auto* hiddenHost = UILinearLayout::NewVertical();
	hiddenHost->setParent( scene->getRoot() );
	hiddenHost->applyProperty( CSS::StyleSheetProperty( "aria-hidden", "true" ) );
	status->setParent( hiddenHost );
	manager->clearPendingEvents();
	text->setText( "Indexing 3%" );
	EXPECT_EQ( manager->getPendingAnnouncements().size(), 0u );
}

UTEST( Accessibility, MenuItemExposesItsShortcut ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* menu = UIPopUpMenu::New();
	menu->setParent( scene->getRoot() );
	auto* save = menu->add( "Save", {}, "ctrl+shift+s" );
	auto* plain = menu->add( "About" );
	auto info = manager->getNodeInfo( manager->getNodeRef( save ) );
	EXPECT_FALSE( info.shortcut.empty() );
	EXPECT_TRUE( String::toLower( info.shortcut.key ) == "s" );
	EXPECT_EQ( info.shortcut.modifiers,
			   AccessibilityShortcut::Control | AccessibilityShortcut::Shift );
	EXPECT_TRUE( manager->getNodeInfo( manager->getNodeRef( plain ) ).shortcut.empty() );
}

UTEST( Accessibility, ScrollToBringsScrollViewContentIntoView ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* scrollView = UIScrollView::New();
	scrollView->setParent( scene->getRoot() );
	scrollView->setPixelsSize( 100, 100 );
	auto* content = UILinearLayout::NewVertical();
	content->setParent( scrollView );
	content->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
	content->setPixelsSize( 80, 0 );
	UIPushButton* last = nullptr;
	for ( int i = 0; i < 20; ++i ) {
		last = UIPushButton::New();
		last->setParent( content );
		last->setText( "Item " + String::toString( i ) );
		last->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		last->setPixelsSize( 80, 30 );
	}
	scene->update( Time::Zero );
	auto ref = manager->getNodeRef( last );
	auto info = manager->getNodeInfo( ref );
	ASSERT_TRUE( info.actions & accessibilityActionMask( AccessibilityAction::ScrollTo ) );
	const auto viewBounds = scrollView->getWorldBounds();
	EXPECT_FALSE( viewBounds.contains( last->getWorldBounds() ) );
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::ScrollTo, {} } ) );
	EXPECT_GT( scrollView->getVerticalScrollBar()->getValue(), 0.f );
	const auto bounds = last->getWorldBounds();
	EXPECT_GE( bounds.Top, viewBounds.Top - 1.f );
	EXPECT_LE( bounds.Bottom, viewBounds.Bottom + 1.f );
	// Widgets outside any scroll view do not offer the action.
	auto* outside = UIPushButton::New();
	outside->setParent( scene->getRoot() );
	EXPECT_FALSE( manager->getNodeInfo( manager->getNodeRef( outside ) ).actions &
				  accessibilityActionMask( AccessibilityAction::ScrollTo ) );
}

UTEST( Accessibility, AuditReportsUnnamedControlsAndBrokenRelations ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* unnamed = UIPushButton::New();
	unnamed->setParent( scene->getRoot() );
	auto* tooltipNamed = UIPushButton::New();
	tooltipNamed->setParent( scene->getRoot() );
	tooltipNamed->setTooltipText( "Open" );
	auto* broken = UITextInput::New();
	broken->setParent( scene->getRoot() );
	broken->setAccessibilityLabelledBy( "nowhere" );
	auto* hidden = UIPushButton::New();
	hidden->setParent( scene->getRoot() );
	hidden->setAccessibilityHidden( true );

	auto issues = AccessibilityWidgetResolver::audit( scene->getRoot() );
	bool unnamedReported = false;
	bool brokenReported = false;
	for ( const auto& issue : issues ) {
		EXPECT_TRUE( issue.widget != tooltipNamed );
		EXPECT_TRUE( issue.widget != hidden );
		unnamedReported |= issue.widget == unnamed;
		brokenReported |= issue.widget == broken;
	}
	EXPECT_TRUE( unnamedReported );
	EXPECT_TRUE( brokenReported );
}

UTEST( Accessibility, LeafControlsHideTheirImplementationChildren ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* button = UIPushButton::New();
	button->setParent( scene->getRoot() );
	button->setText( "Save" );
	auto buttonRef = manager->getNodeRef( button );
	// The button's text view is its name, not a child label.
	EXPECT_EQ( manager->getChildCount( buttonRef ), 0u );
	EXPECT_TRUE( manager->getNodeInfo( buttonRef ).name == String( "Save" ) );

	auto* menu = UIPopUpMenu::New();
	menu->setParent( scene->getRoot() );
	auto* item = menu->add( "Open", {}, "ctrl+o" );
	auto itemRef = manager->getNodeRef( item );
	EXPECT_EQ( manager->getChildCount( itemRef ), 0u );
	EXPECT_FALSE( manager->getNodeInfo( itemRef ).shortcut.empty() );

	// Changing the inner text reports a name change on the button itself.
	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	button->setText( "Save all" );
	bool buttonRenamed = false;
	for ( const auto& event : manager->getPendingEvents() ) {
		EXPECT_TRUE( event.ref == buttonRef || event.type != AccessibilityEvent::NameChanged );
		buttonRenamed |= event.ref == buttonRef && event.type == AccessibilityEvent::NameChanged;
	}
	EXPECT_TRUE( buttonRenamed );
	EXPECT_TRUE( AccessibilityWidgetResolver::getLeafOwner( button->getTextView() ) == button );

	// Containers still expose their element children.
	auto* group = UILinearLayout::NewVertical();
	group->setParent( scene->getRoot() );
	group->setAccessibilityRole( AccessibilityRole::Group );
	auto* label = UITextView::New();
	label->setParent( group );
	label->setText( "Label" );
	EXPECT_EQ( manager->getChildCount( manager->getNodeRef( group ) ), 1u );
}

UTEST( Accessibility, RangedTextQueriesMatchTheDocument ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* editor = UICodeEditor::New();
	editor->setParent( scene->getRoot() );
	editor->getDocument().textInput( "first\nsecond\nthird" );
	auto ref = manager->getNodeRef( editor );
	const String value = manager->getNodeInfo( ref ).value;
	ASSERT_EQ( manager->getTextLength( ref ), static_cast<Int32>( value.size() ) );
	EXPECT_EQ( manager->getTextLength( ref ), 18 );
	EXPECT_TRUE( manager->getTextRange( ref, 6, 13 ) == String( "second\n" ) );
	EXPECT_TRUE( manager->getTextRange( ref, 13, 99 ) == String( "third" ) );
	EXPECT_TRUE( manager->getTextRange( ref, 4, 4 ).empty() );
	Int32 start = -1;
	Int32 end = -1;
	ASSERT_TRUE( manager->getTextLineBounds( ref, 8, start, end ) );
	EXPECT_EQ( start, 6 );
	EXPECT_EQ( end, 13 );
	// The last line's newline is not part of the exposed text.
	ASSERT_TRUE( manager->getTextLineBounds( ref, 18, start, end ) );
	EXPECT_EQ( start, 13 );
	EXPECT_EQ( end, 18 );

	// Revisions change with the text, including wholesale resets, and only then.
	const auto revision = manager->getTextRevision( ref );
	EXPECT_TRUE( revision.isValid() );
	EXPECT_TRUE( manager->getTextRevision( ref ) == revision );
	editor->getDocument().textInput( "!" );
	const auto edited = manager->getTextRevision( ref );
	EXPECT_TRUE( edited != revision );
	editor->getDocument().reset();
	EXPECT_TRUE( manager->getTextRevision( ref ) != edited );
	EXPECT_EQ( manager->getTextLength( ref ), 0 );

	// Password inputs never expose their text.
	auto* password = UITextInput::New();
	password->setParent( scene->getRoot() );
	password->setText( "secret" );
	password->setMode( UITextInput::TextInputMode::Password );
	auto passwordRef = manager->getNodeRef( password );
	EXPECT_EQ( manager->getTextLength( passwordRef ), 0 );
	EXPECT_TRUE( manager->getTextRange( passwordRef, 0, 6 ).empty() );
	EXPECT_FALSE( manager->getTextRevision( passwordRef ).isValid() );
}

UTEST( Accessibility, DocumentRemovalsExposeTheRemovedTextDuringNotification ) {
	Doc::TextDocument document;
	document.textInput( "hello world" );
	struct Seen {
		String inserted;
		String removed;
	};
	std::vector<Seen> seen;
	class Recorder : public Doc::TextDocument::Client {
	  public:
		Recorder( Doc::TextDocument& document, std::vector<Seen>& seen ) :
			mDocument( document ), mSeen( seen ) {}

		void onDocumentTextChanged( const Doc::DocumentContentChange& change ) override {
			// An edit made from a callback is notified while the outer one is still in progress.
			if ( nestedEdit ) {
				nestedEdit = false;
				mDocument.insert( 0, mDocument.endOfDoc(), "!" );
			}
			mSeen.push_back( { change.text, mDocument.getNotifiedRemovedText() } );
		}

		void onDocumentUndoRedo( const Doc::TextDocument::UndoRedo& ) override {}

		void onDocumentCursorChange( const Doc::TextPosition& ) override {}

		void onDocumentSelectionChange( const Doc::TextRange& ) override {}

		void onDocumentLineCountChange( const size_t&, const size_t& ) override {}

		void onDocumentLineChanged( const Int64& ) override {}

		void onDocumentSaved( Doc::TextDocument* ) override {}

		void onDocumentClosed( Doc::TextDocument* ) override {}

		void onDocumentDirtyOnFileSystem( Doc::TextDocument* ) override {}

		void onDocumentMoved( Doc::TextDocument* ) override {}

		void onDocumentReset( Doc::TextDocument* ) override {}

		Type getTextDocumentClientType() override { return Type::Auxiliary; }

		bool nestedEdit{ false };

	  private:
		Doc::TextDocument& mDocument;
		std::vector<Seen>& mSeen;
	} recorder( document, seen );
	document.registerClient( &recorder );
	document.remove( 0, { { 0, 5 }, { 0, 11 } } );
	ASSERT_EQ( seen.size(), 1u );
	EXPECT_TRUE( seen[0].removed == String( " world" ) );
	EXPECT_TRUE( seen[0].inserted.empty() );
	EXPECT_TRUE( document.getText() == String( "hello" ) );
	// Outside a notification there is nothing to report.
	EXPECT_TRUE( document.getNotifiedRemovedText().empty() );

	// A nested insertion sees no removed text, and the outer removal sees its own afterwards.
	seen.clear();
	recorder.nestedEdit = true;
	document.remove( 0, { { 0, 0 }, { 0, 2 } } );
	document.unregisterClient( &recorder );
	ASSERT_EQ( seen.size(), 2u );
	EXPECT_TRUE( seen[0].inserted == String( "!" ) );
	EXPECT_TRUE( seen[0].removed.empty() );
	EXPECT_TRUE( seen[1].removed == String( "he" ) );
	EXPECT_TRUE( document.getText() == String( "llo!" ) );
}

namespace {

/** Records what the manager delivers. While ignoring, it rejects text changes the way a backend
 * does when it has nothing to report them to. */
class RecordingBackend : public AccessibilityBackend {
  public:
	bool ignoring{ false };
	std::vector<AccessibilityTextChange> changes;
	std::vector<String> announcements;

	std::vector<AccessibilityPendingEvent> events;
	bool active{ true };

	bool isAvailable() const override { return true; }

	bool hasActiveClients() const override { return active; }

	void onEvent( const AccessibilityPendingEvent& event ) override { events.push_back( event ); }

	bool onTextChanged( AccessibilityNodeRef, const AccessibilityTextChange& change ) override {
		if ( ignoring )
			return false;
		changes.push_back( change );
		return true;
	}

	void announce( const String& message, AccessibilityLive ) override {
		announcements.push_back( message );
	}
};

bool hasPendingEvent( const AccessibilityManager* manager, AccessibilityNodeRef ref,
					  AccessibilityEvent event ) {
	for ( const auto& pending : manager->getPendingEvents() ) {
		if ( pending.ref == ref && pending.type == event )
			return true;
	}
	return false;
}

} // namespace

UTEST( Accessibility, IgnoredTextChangesDoNotConsumeTheFrameBudget ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<RecordingBackend>();
	auto* recorder = backend.get();
	manager->setBackend( std::move( backend ) );
	manager->onNativeClientObserved();
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	input->setText( "eepp" );
	manager->update();
	recorder->changes.clear();

	// The ignored replacement includes a whole-text reset; it must not silence later edits.
	recorder->ignoring = true;
	input->setText( "replaced" );
	recorder->ignoring = false;
	ASSERT_TRUE( recorder->changes.empty() );
	for ( int i = 0; i < 6; ++i )
		input->getDocument().textInput( "!" );
	// The frame still has its full budget: four exact edits, then one whole-text change that
	// covers the rest of the frame.
	ASSERT_EQ( recorder->changes.size(), 5u );
	for ( size_t i = 0; i < 4; ++i ) {
		EXPECT_EQ( recorder->changes[i].offset, static_cast<Int32>( 8 + i ) );
		EXPECT_TRUE( recorder->changes[i].inserted == String( "!" ) );
	}
	EXPECT_TRUE( recorder->changes[4].isWholeText() );

	// The budget resets every frame.
	manager->update();
	recorder->changes.clear();
	input->getDocument().textInput( "?" );
	ASSERT_EQ( recorder->changes.size(), 1u );
	EXPECT_FALSE( recorder->changes[0].isWholeText() );
}

UTEST( Accessibility, QueuedLiveAnnouncementsAreRevalidatedOnDelivery ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<RecordingBackend>();
	auto* recorder = backend.get();
	manager->setBackend( std::move( backend ) );
	manager->onNativeClientObserved();
	auto* status = UILinearLayout::NewVertical();
	status->setParent( scene->getRoot() );
	status->applyProperty( CSS::StyleSheetProperty( "aria-live", "polite" ) );
	auto* text = UITextView::New();
	text->setParent( status );
	text->setText( "Saved" );
	manager->update();
	ASSERT_EQ( recorder->announcements.size(), 1u );

	// Hidden after the change was queued, before delivery.
	text->setText( "Saving" );
	status->setAccessibilityHidden( true );
	manager->update();
	EXPECT_EQ( recorder->announcements.size(), 1u );

	// Gone before delivery.
	status->setAccessibilityHidden( false );
	text->setText( "Saved again" );
	eeDelete( text );
	manager->update();
	EXPECT_EQ( recorder->announcements.size(), 1u );
}

UTEST( Accessibility, RelationTargetChangesNotifyDependents ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	manager->onNativeClientObserved();
	auto* label = UITextView::New();
	label->setParent( scene->getRoot() );
	label->setId( "name_label" );
	label->setText( "Name" );
	auto* hint = UITextView::New();
	hint->setParent( scene->getRoot() );
	hint->setId( "name_hint" );
	hint->setText( "Your full name" );
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	input->applyProperty( CSS::StyleSheetProperty( "aria-labelledby", "name_label" ) );
	input->applyProperty( CSS::StyleSheetProperty( "aria-describedby", "name_hint" ) );
	const auto ref = manager->getNodeRef( input );
	// A client reads the input, caching its name and description.
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == String( "Name" ) );
	manager->clearPendingEvents();

	label->setText( "Full name" );
	EXPECT_TRUE( hasPendingEvent( manager, ref, AccessibilityEvent::NameChanged ) );
	EXPECT_FALSE( hasPendingEvent( manager, ref, AccessibilityEvent::DescriptionChanged ) );
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == String( "Full name" ) );
	manager->clearPendingEvents();

	hint->setText( "As on your passport" );
	EXPECT_TRUE( hasPendingEvent( manager, ref, AccessibilityEvent::DescriptionChanged ) );
	EXPECT_FALSE( hasPendingEvent( manager, ref, AccessibilityEvent::NameChanged ) );
	manager->clearPendingEvents();

	// Renaming the target's id breaks the relation; removing it does too.
	label->setId( "other_label" );
	EXPECT_TRUE( hasPendingEvent( manager, ref, AccessibilityEvent::NameChanged ) );
	manager->clearPendingEvents();
	eeDelete( hint );
	EXPECT_TRUE( hasPendingEvent( manager, ref, AccessibilityEvent::DescriptionChanged ) );
	manager->clearPendingEvents();

	// Unrelated widgets with ids do not touch the input.
	auto* other = UITextView::New();
	other->setParent( scene->getRoot() );
	other->setId( "unrelated" );
	other->setText( "Other" );
	EXPECT_FALSE( hasPendingEvent( manager, ref, AccessibilityEvent::NameChanged ) );
}

UTEST( Accessibility, SuppressedTextChangesStayWithinTheirWindow ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* secondary = app.createWindow(
		WindowSettings( 320, 240, "Accessibility Shared Document", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );
	ASSERT_NE( secondary, nullptr );
	// One document shown in both windows.
	auto document = std::make_shared<Doc::TextDocument>();
	document->textInput( "eepp" );
	UISceneNode* scenes[2] = { app.getUI(), secondary };
	AccessibilityManager* managers[2]{};
	RecordingBackend* recorders[2]{};
	AccessibilityNodeRef refs[2];
	UICodeEditor* editors[2]{};
	auto addEditor = [&]( size_t i ) {
		editors[i] = UICodeEditor::New();
		editors[i]->setParent( scenes[i]->getRoot() );
		editors[i]->setDocument( document );
		refs[i] = managers[i]->getNodeRef( editors[i] );
	};
	for ( size_t i = 0; i < 2; ++i ) {
		managers[i] = scenes[i]->getAccessibilityManager();
		auto backend = std::make_unique<RecordingBackend>();
		recorders[i] = backend.get();
		managers[i]->setBackend( std::move( backend ) );
		managers[i]->onNativeClientObserved();
		addEditor( i );
	}
	// Node refs are manager-local and each manager allocates its own sequence. Recreate the
	// editor of the window that is behind until both editors get the same ref, the case where a
	// shared identity would collide. Ids are never reused, so each retry advances that sequence.
	for ( int attempt = 0; attempt < 64 && !( refs[0] == refs[1] ); ++attempt ) {
		const size_t behind = refs[0].id < refs[1].id ? 0 : 1;
		eeDelete( editors[behind] );
		addEditor( behind );
	}
	ASSERT_TRUE( refs[0] == refs[1] );
	for ( size_t i = 0; i < 2; ++i ) {
		managers[i]->update();
		recorders[i]->changes.clear();
	}

	// A client replaces the text through the first window, which reports the change itself.
	managers[0]->setSuppressedTextChanges( refs[0] );
	EXPECT_TRUE( managers[0]->performAction( refs[0], { AccessibilityAction::SetText, "eepp2" } ) );
	managers[0]->setSuppressedTextChanges( {} );
	EXPECT_TRUE( recorders[0]->changes.empty() );
	// The second window's editor changed too and must say so.
	ASSERT_EQ( recorders[1]->changes.size(), 2u );
	EXPECT_TRUE( recorders[1]->changes[0].removed == String( "eepp" ) );
	EXPECT_TRUE( recorders[1]->changes[1].inserted == String( "eepp2" ) );

	// Suppression ended, and it consumed none of the first window's budget.
	for ( int i = 0; i < 4; ++i )
		document->textInput( "!" );
	EXPECT_EQ( recorders[0]->changes.size(), 4u );
	for ( const auto& change : recorders[0]->changes )
		EXPECT_FALSE( change.isWholeText() );
}

UTEST( Accessibility, ExactRemovalRecordsCarryTheRemovedText ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* manager = app.getUI()->getAccessibilityManager();
	auto backend = std::make_unique<RecordingBackend>();
	auto* recorder = backend.get();
	manager->setBackend( std::move( backend ) );
	manager->onNativeClientObserved();
	auto* editor = UICodeEditor::New();
	editor->setParent( app.getUI()->getRoot() );
	auto& document = editor->getDocument();
	document.textInput( "hello" );
	auto nextFrame = [&] {
		manager->update();
		recorder->changes.clear();
	};
	nextFrame();

	document.deleteToPreviousChar();
	ASSERT_EQ( recorder->changes.size(), 1u );
	EXPECT_EQ( recorder->changes[0].offset, 4 );
	EXPECT_TRUE( recorder->changes[0].removed == String( "o" ) );
	EXPECT_TRUE( recorder->changes[0].inserted.empty() );
	nextFrame();

	// The quick typing and the deletion form one undo group: undo reinserts "o", then removes
	// "hello"; redo replays both in the original order.
	document.undo();
	ASSERT_EQ( recorder->changes.size(), 2u );
	EXPECT_TRUE( recorder->changes[0].inserted == String( "o" ) );
	EXPECT_TRUE( recorder->changes[0].removed.empty() );
	EXPECT_EQ( recorder->changes[1].offset, 0 );
	EXPECT_TRUE( recorder->changes[1].removed == String( "hello" ) );
	EXPECT_TRUE( document.isEmpty() );
	nextFrame();
	document.redo();
	ASSERT_EQ( recorder->changes.size(), 2u );
	EXPECT_TRUE( recorder->changes[0].inserted == String( "hello" ) );
	EXPECT_TRUE( recorder->changes[1].removed == String( "o" ) );
	EXPECT_TRUE( document.getText() == String( "hell" ) );
	nextFrame();

	// A replacement is a removal followed by an insertion.
	document.setSelection( { { 0, 0 }, { 0, 4 } } );
	document.textInput( "J" );
	ASSERT_EQ( recorder->changes.size(), 2u );
	EXPECT_TRUE( recorder->changes[0].removed == String( "hell" ) );
	EXPECT_TRUE( recorder->changes[1].inserted == String( "J" ) );
	EXPECT_TRUE( document.getText() == String( "J" ) );
}

namespace {

/** Reads the node back from inside each name and description notification, the way a synchronous
 * native observer would. */
class ObservingBackend : public AccessibilityBackend {
  public:
	explicit ObservingBackend( AccessibilityManager& manager ) : mManager( manager ) {}

	std::vector<std::pair<AccessibilityEvent, String>> observed;

	bool isAvailable() const override { return true; }

	bool hasActiveClients() const override { return true; }

	void onEvent( const AccessibilityPendingEvent& event ) override {
		if ( event.type == AccessibilityEvent::NameChanged )
			observed.emplace_back( event.type, mManager.getNodeInfo( event.ref ).name );
		else if ( event.type == AccessibilityEvent::DescriptionChanged )
			observed.emplace_back( event.type, mManager.getNodeInfo( event.ref ).description );
	}

  private:
	AccessibilityManager& mManager;
};

} // namespace

UTEST( Accessibility, IdChangesNotifyOnlyRememberedRelationTargets ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<ObservingBackend>( *manager );
	auto* observer = backend.get();
	manager->setBackend( std::move( backend ) );
	scene->update( Time::Zero );
	ASSERT_TRUE( scene->hasActiveAccessibilityClients() );
	// Ids longer than any small-string buffer.
	const std::string labelId( "field_label_" + std::string( 64, 'l' ) );
	const std::string hintId( "field_hint_" + std::string( 64, 'h' ) );
	const std::string movedId( "moved_" + std::string( 64, 'm' ) );
	const std::string unrelatedId( "unrelated_" + std::string( 64, 'u' ) );
	auto* label = UITextView::New();
	label->setParent( scene->getRoot() );
	label->setId( labelId );
	label->setText( "Name" );
	auto* hint = UITextView::New();
	hint->setParent( scene->getRoot() );
	hint->setId( hintId );
	hint->setText( "Hint" );
	auto* unrelated = UIWidget::New();
	unrelated->setParent( scene->getRoot() );
	unrelated->setId( unrelatedId );
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	input->setAccessibilityLabelledBy( labelId );
	input->setAccessibilityDescribedBy( hintId );
	const auto ref = manager->getNodeRef( input );
	const auto info = manager->getNodeInfo( ref );
	EXPECT_TRUE( info.name == String( "Name" ) );
	EXPECT_TRUE( info.description == String( "Hint" ) );
	EXPECT_TRUE( manager->isRelationTarget( String::hash( labelId ) ) );
	EXPECT_TRUE( manager->isRelationTarget( String::hash( hintId ) ) );
	EXPECT_FALSE( manager->isRelationTarget( String::hash( unrelatedId ) ) );
	observer->observed.clear();

	// An unreferenced long id changes without notifying anyone.
	unrelated->setId( unrelatedId + "_renamed" );
	unrelated->setId( unrelatedId );
	EXPECT_TRUE( observer->observed.empty() );

	// The observer reads the relation after the rename, not before it.
	label->setId( movedId );
	ASSERT_EQ( observer->observed.size(), 1u );
	EXPECT_EQ( observer->observed[0].first, AccessibilityEvent::NameChanged );
	EXPECT_FALSE( observer->observed[0].second == String( "Name" ) );
	EXPECT_TRUE( observer->observed[0].second == manager->getNodeInfo( ref ).name );
	observer->observed.clear();

	// A missing target becoming available again renames the input through the new id.
	label->setId( labelId );
	ASSERT_EQ( observer->observed.size(), 1u );
	EXPECT_EQ( observer->observed[0].first, AccessibilityEvent::NameChanged );
	EXPECT_TRUE( observer->observed[0].second == String( "Name" ) );
	observer->observed.clear();

	hint->setId( movedId );
	ASSERT_EQ( observer->observed.size(), 1u );
	EXPECT_EQ( observer->observed[0].first, AccessibilityEvent::DescriptionChanged );
	EXPECT_FALSE( observer->observed[0].second == String( "Hint" ) );
	observer->observed.clear();
	hint->setId( hintId );
	ASSERT_EQ( observer->observed.size(), 1u );
	EXPECT_TRUE( observer->observed[0].second == String( "Hint" ) );
}

UTEST( Accessibility, RootOnlyDeletionStillNotifiesRelationDependents ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* root = scene->getRoot();
	auto* label = UITextView::New();
	label->setParent( root );
	label->setId( "root_label" );
	label->setText( "Window name" );
	auto* hint = UITextView::New();
	hint->setParent( root );
	hint->setId( "root_hint" );
	hint->setText( "Window hint" );
	root->setAccessibilityLabelledBy( "root_label" );
	root->setAccessibilityDescribedBy( "root_hint" );
	auto* manager = scene->getAccessibilityManager();
	manager->onNativeClientObserved();
	// The client reads only the root: it is the single widget with an identity.
	const auto ref = manager->getNodeRef( root );
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == String( "Window name" ) );
	manager->clearPendingEvents();

	eeDelete( label );
	EXPECT_TRUE( hasPendingEvent( manager, ref, AccessibilityEvent::NameChanged ) );
	manager->clearPendingEvents();
	eeDelete( hint );
	EXPECT_TRUE( hasPendingEvent( manager, ref, AccessibilityEvent::DescriptionChanged ) );
}

UTEST( Accessibility, SceneLifecycleNeverCreatesAManager ) {
	auto hasManager = []( const UISceneNode* scene ) {
		return scene->getAccessibilityManager() != nullptr;
	};
	for ( auto policy : { AccessibilityPolicy::Disabled, AccessibilityPolicy::Auto } ) {
		auto settings = accessibilityTestSettings();
		settings.accessibilityPolicy = policy;
		UIApplication app( accessibilityTestWindow(), settings );
		ASSERT_NE( app.getUI(), nullptr );
		auto* host = app.getUI();
		auto* secondHost = app.createWindow( accessibilityTestWindow() );
		ASSERT_NE( secondHost, nullptr );
		// Neither host has been updated yet, so neither has a manager.
		ASSERT_FALSE( hasManager( host ) );
		ASSERT_FALSE( hasManager( secondHost ) );

		// Two nesting levels, populated, moved to another host and destroyed.
		auto* nested = UISceneNode::New( app.getWindow() );
		nested->setParent( host->getRoot() );
		auto* inner = UISceneNode::New( app.getWindow() );
		inner->setParent( nested->getRoot() );
		for ( auto* scene : { nested, inner } ) {
			auto* widget = UIWidget::New();
			widget->setParent( scene->getRoot() );
			eeDelete( widget );
			UIPushButton::New()->setParent( scene->getRoot() );
		}
		EXPECT_FALSE( hasManager( host ) );
		nested->setParent( secondHost->getRoot() );
		EXPECT_FALSE( hasManager( host ) );
		EXPECT_FALSE( hasManager( secondHost ) );
		eeDelete( inner );
		eeDelete( nested );
		EXPECT_FALSE( hasManager( host ) );
		EXPECT_FALSE( hasManager( secondHost ) );

		// A Disabled host never gets one, not even from its own updates.
		host->update( Time::Zero );
		EXPECT_EQ( hasManager( host ), policy != AccessibilityPolicy::Disabled );
	}
}

UTEST( Accessibility, DisconnectedClientsOnlyInvalidateOnRemoval ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<RecordingBackend>();
	auto* recorder = backend.get();
	manager->setBackend( std::move( backend ) );
	scene->update( Time::Zero );
	ASSERT_TRUE( scene->hasActiveAccessibilityClients() );
	auto* group = UIWidget::New();
	group->setParent( scene->getRoot() );
	auto* button = UIPushButton::New();
	button->setParent( group );
	auto* edited = UIPushButton::New();
	edited->setParent( group );
	// The client knows the buttons, not their parent.
	const auto buttonRef = manager->getNodeRef( button );
	const auto editedRef = manager->getNodeRef( edited );

	recorder->active = false;
	scene->update( Time::Zero );
	ASSERT_FALSE( scene->hasActiveAccessibilityClients() );
	recorder->events.clear();
	eeDelete( button );
	manager->onSubtreeRemoved( edited );
	// Identities are gone and the backend detached their wrappers...
	EXPECT_FALSE( manager->isValid( buttonRef ) );
	EXPECT_FALSE( manager->isValid( editedRef ) );
	ASSERT_EQ( recorder->events.size(), 2u );
	for ( const auto& event : recorder->events ) {
		EXPECT_EQ( event.type, AccessibilityEvent::Destroyed );
		EXPECT_FALSE( event.related.isValid() );
	}
	// ...but nothing was queued for a client, and the parent was not resolved to announce it.
	EXPECT_TRUE( manager->getPendingEvents().empty() );

	// A reconnecting client starts from a consistent tree.
	recorder->active = true;
	scene->update( Time::Zero );
	const auto groupRef = manager->getNodeRef( group );
	ASSERT_EQ( manager->getChildCount( groupRef ), 1u );
	EXPECT_TRUE( manager->getChild( groupRef, 0 ) == manager->getNodeRef( edited ) );
	EXPECT_FALSE( manager->getNodeRef( edited ) == editedRef );
}

UTEST( Accessibility, SpinBoxExposesOnlyItselfWithoutHidingItsInput ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto* spin = UISpinBox::New();
	spin->setParent( scene->getRoot() );
	spin->setPixelsSize( 120, 32 );
	spin->setMinValue( 0 );
	spin->setMaxValue( 10 );
	spin->setValue( 4 );
	spin->setAccessibilityLabel( "Copies" );
	scene->update( Time::Zero );
	// The leaf role hides the input; it needs no accessibility metadata of its own.
	EXPECT_FALSE( spin->getTextInput()->isAccessibilityHidden() );
	const auto ref = manager->getNodeRef( spin );
	const auto info = manager->getNodeInfo( ref );
	EXPECT_EQ( info.role, AccessibilityRole::SpinButton );
	EXPECT_TRUE( info.name == String( "Copies" ) );
	EXPECT_TRUE( info.range.valid );
	EXPECT_EQ( info.range.maximum, 10. );
	EXPECT_EQ( std::stod( info.value.toUtf8() ), 4. );
	EXPECT_EQ( manager->getChildCount( ref ), 0u );
	EXPECT_TRUE( manager->hitTest( spin->getTextInput()->getWorldBounds().getCenter() ) == ref );
	spin->getTextInput()->setFocus();
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == ref );
}

UTEST( Accessibility, AriaPropertiesSurviveUntilAClientAttaches ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	// Ids longer than the inline string capacity.
	const std::string labelId( "accessibility-relation-label-with-a-long-identifier" );
	const std::string hintId( "accessibility-relation-hint-with-a-long-identifier" );
	auto* label = UITextView::New();
	label->setParent( scene->getRoot() );
	label->setId( labelId );
	label->setText( "Destination" );
	auto* hint = UITextView::New();
	hint->setParent( scene->getRoot() );
	hint->setId( hintId );
	hint->setText( "Where files are copied" );
	auto* input = UITextInput::New();
	input->setParent( scene->getRoot() );
	auto* status = UITextView::New();
	status->setParent( scene->getRoot() );
	auto* button = UIPushButton::New();
	button->setParent( scene->getRoot() );

	// Set while no client exists.
	input->applyProperty( CSS::StyleSheetProperty( "aria-labelledby", labelId ) );
	input->applyProperty( CSS::StyleSheetProperty( "aria-describedby", hintId ) );
	status->applyProperty( CSS::StyleSheetProperty( "aria-live", "ASSERTIVE" ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-label", "@string(copy_now, Copy now)" ) );
	EXPECT_TRUE( input->getAccessibilityLabelledBy() == labelId );
	EXPECT_EQ( status->getAccessibilityLive(), AccessibilityLive::Assertive );
	status->applyProperty( CSS::StyleSheetProperty( "aria-live", "Polite" ) );
	EXPECT_EQ( status->getAccessibilityLive(), AccessibilityLive::Polite );

	// A client attaching later reads all of it.
	manager->onNativeClientObserved();
	auto info = manager->getNodeInfo( manager->getNodeRef( input ) );
	EXPECT_TRUE( info.name == String( "Destination" ) );
	EXPECT_TRUE( info.description == String( "Where files are copied" ) );
	EXPECT_TRUE( manager->getNodeInfo( manager->getNodeRef( button ) ).name ==
				 String( "Copy now" ) );

	// Reapplying unchanged values reports nothing.
	manager->clearPendingEvents();
	input->applyProperty( CSS::StyleSheetProperty( "aria-labelledby", labelId ) );
	input->applyProperty( CSS::StyleSheetProperty( "aria-describedby", hintId ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-label", "@string(copy_now, Copy now)" ) );
	status->applyProperty( CSS::StyleSheetProperty( "aria-live", "polite" ) );
	EXPECT_TRUE( manager->getPendingEvents().empty() );

	// Resetting to empty removes the metadata.
	input->applyProperty( CSS::StyleSheetProperty( "aria-labelledby", "" ) );
	button->applyProperty( CSS::StyleSheetProperty( "aria-label", "" ) );
	status->applyProperty( CSS::StyleSheetProperty( "aria-live", "off" ) );
	EXPECT_TRUE( input->getAccessibilityLabelledBy().empty() );
	EXPECT_EQ( status->getAccessibilityLive(), AccessibilityLive::Off );
	EXPECT_TRUE( manager->getNodeInfo( manager->getNodeRef( button ) ).name.empty() );
}

UTEST( Accessibility, DisconnectReleasesQueriedRowsAndReconnectsFresh ) {
	UIApplication app( accessibilityTestWindow(), accessibilityTestSettings() );
	ASSERT_NE( app.getUI(), nullptr );
	auto* scene = app.getUI();
	auto* manager = scene->getAccessibilityManager();
	auto backend = std::make_unique<RecordingBackend>();
	auto* recorder = backend.get();
	manager->setBackend( std::move( backend ) );
	scene->update( Time::Zero );
	auto model = std::make_shared<AccessibilityRowsModel>( 6 );
	auto* list = UIListView::New();
	list->setParent( scene->getRoot() );
	list->setModel( model );
	const auto listRef = manager->getNodeRef( list );
	ASSERT_EQ( manager->getChildCount( listRef ), 6u );
	for ( size_t row = 0; row < 6; ++row )
		manager->getChild( listRef, row );
	const auto staleRow = manager->getChild( listRef, 3 );
	ASSERT_TRUE( manager->isValid( staleRow ) );
	// A registration made outside accessibility, sharing row 3's handle.
	Models::PersistentModelIndex probe( model->index( 3 ) );
	EXPECT_EQ( model->persistentCount(), 6u );

	recorder->active = false;
	scene->update( Time::Zero );
	// The client's rows are gone and their handles released; the probe's survives.
	EXPECT_FALSE( manager->isValid( staleRow ) );
	EXPECT_EQ( model->persistentCount(), 1u );
	model->insertAt( 0, 5 );
	EXPECT_EQ( probe.row(), 4 );

	recorder->active = true;
	scene->update( Time::Zero );
	ASSERT_EQ( manager->getChildCount( listRef ), 7u );
	const auto freshRow = manager->getChild( listRef, 4 );
	EXPECT_TRUE( manager->isValid( freshRow ) );
	EXPECT_FALSE( freshRow == staleRow );
	EXPECT_TRUE( manager->getNodeInfo( freshRow ).name == String( "40" ) );
	EXPECT_FALSE( manager->isValid( staleRow ) );
}
