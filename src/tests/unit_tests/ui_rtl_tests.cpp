#include "utest.hpp"

#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/graphics/textlayout.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiflowlayout.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Graphics;
using namespace EE::UI::CSS;

namespace {

UIApplication::Settings rtlTestSettings() {
	return UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
}

WindowSettings rtlWindowSettings() {
	return WindowSettings( 640, 480, "eepp - RTL Test", WindowStyle::Default,
						   WindowBackend::Default, 32 );
}

UIWidget* fixedWidget( Node* parent, Float w, Float h, const std::string& id = "" ) {
	UIWidget* widget = UIWidget::New();
	widget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	widget->setPixelsSize( w, h );
	if ( !id.empty() )
		widget->setId( id );
	widget->setParent( parent );
	return widget;
}

UIWidget* findWidget( UIApplication& app, const std::string& id ) {
	Node* node = app.getUI()->getRoot()->find( id );
	return node && node->isWidget() ? node->asType<UIWidget>() : nullptr;
}

class ParentChangeWidgetProbe : public UIWidget {
  public:
	ParentChangeWidgetProbe() : UIWidget( "parent-change-probe" ) {}

	using Node::childAddAt;

	int parentTreeChanges{ 0 };
	UIDirection observedDirection{ UIDirection::LeftToRight };

  protected:
	void onParentTreeChange() {
		++parentTreeChanges;
		UIWidget::onParentTreeChange();
	}

	void onParentChange() {
		// Deliberately omit the base hook: inherited state must already be current.
		observedDirection = getDirection();
	}
};

class ParentChangeNodeProbe : public Node {
  public:
	UINode* observedChild{ nullptr };
	int parentTreeChanges{ 0 };
	UIDirection observedDirection{ UIDirection::LeftToRight };

  protected:
	void onParentTreeChange() {
		++parentTreeChanges;
		Node::onParentTreeChange();
	}

	void onParentChange() {
		// A plain node's callback must also see its UI descendants' updated direction.
		if ( observedChild )
			observedDirection = observedChild->getDirection();
	}
};

class LogicalBoxWidgetProbe : public UIWidget {
  public:
	LogicalBoxWidgetProbe() : UIWidget( "logical-box-probe" ) {}

	bool hasLogicalBox() const { return mLogicalBox != nullptr; }
};

} // namespace

// --- Direction model --------------------------------------------------------------------------

UTEST( UIDirection, helpersResolveLogicalValues ) {
	EXPECT_EQ( (Uint32)TEXT_ALIGN_LEFT, resolveHorizontalAlignment( UIHorizontalAlignment::Start,
																	UIDirection::LeftToRight ) );
	EXPECT_EQ( (Uint32)TEXT_ALIGN_RIGHT, resolveHorizontalAlignment( UIHorizontalAlignment::Start,
																	 UIDirection::RightToLeft ) );
	EXPECT_EQ( (Uint32)TEXT_ALIGN_RIGHT,
			   resolveHorizontalAlignment( UIHorizontalAlignment::End, UIDirection::LeftToRight ) );
	EXPECT_EQ( (Uint32)TEXT_ALIGN_LEFT,
			   resolveHorizontalAlignment( UIHorizontalAlignment::End, UIDirection::RightToLeft ) );
	// Physical values never change with the direction.
	for ( auto dir : { UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
		EXPECT_EQ( (Uint32)TEXT_ALIGN_LEFT,
				   resolveHorizontalAlignment( UIHorizontalAlignment::Left, dir ) );
		EXPECT_EQ( (Uint32)TEXT_ALIGN_RIGHT,
				   resolveHorizontalAlignment( UIHorizontalAlignment::Right, dir ) );
		EXPECT_EQ( (Uint32)TEXT_ALIGN_CENTER,
				   resolveHorizontalAlignment( UIHorizontalAlignment::Center, dir ) );
	}
	EXPECT_TRUE( resolveInlineEdge( UIInlineEdge::Start, UIDirection::RightToLeft ) ==
				 UIPhysicalEdge::Right );
	EXPECT_TRUE( resolveInlineEdge( UIInlineEdge::End, UIDirection::RightToLeft ) ==
				 UIPhysicalEdge::Left );
	EXPECT_TRUE( resolveInlineEdge( UIInlineEdge::Start, UIDirection::LeftToRight ) ==
				 UIPhysicalEdge::Left );

	UIHorizontalAlignment align;
	EXPECT_TRUE( UIHorizontalAlignmentHelper::fromString( "end", align ) );
	EXPECT_TRUE( align == UIHorizontalAlignment::End );
	EXPECT_FALSE( UIHorizontalAlignmentHelper::fromString( "justify", align ) );
	EXPECT_TRUE( UIDirectionHelper::fromString( "RTL" ) == UIDirectionValue::RightToLeft );
	EXPECT_TRUE( UIDirectionHelper::fromString( "auto" ) == UIDirectionValue::Inherit );
}

UTEST( UIDirection, localeHelperMapsRightToLeftLanguages ) {
	for ( const char* locale : { "ar", "ar_EG.UTF-8", "he-IL", "fa", "ur_PK", "yi", "ckb" } )
		EXPECT_TRUE( directionForLocale( locale ) == UIDirection::RightToLeft );
	for ( const char* locale : { "en", "en_US", "es-AR", "hi", "zh_CN", "", "arn" } )
		EXPECT_TRUE( directionForLocale( locale ) == UIDirection::LeftToRight );
}

UTEST( UIDirection, inheritanceOverridesAndRuntimeChanges ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	UIWidget* root = app.getUI()->getRoot();
	UIWidget* parent = fixedWidget( root, 100, 100 );
	UIWidget* child = fixedWidget( parent, 10, 10 );
	UIWidget* grandChild = fixedWidget( child, 5, 5 );

	EXPECT_TRUE( parent->isLTR() );
	EXPECT_FALSE( parent->isDirectionExplicit() );
	EXPECT_TRUE( grandChild->getTextBaseDirection() == TextDirection::Unspecified );

	app.getUI()->setDirection( UIDirection::RightToLeft );
	EXPECT_TRUE( parent->isRTL() );
	EXPECT_TRUE( grandChild->isRTL() );
	EXPECT_TRUE( grandChild->isDirectionExplicit() );
	EXPECT_TRUE( grandChild->getTextBaseDirection() == TextDirection::RightToLeft );

	// Nested LTR override applies to the subtree.
	child->setDirection( UIDirection::LeftToRight );
	EXPECT_TRUE( parent->isRTL() );
	EXPECT_TRUE( child->isLTR() );
	EXPECT_TRUE( grandChild->isLTR() );
	EXPECT_TRUE( grandChild->getTextBaseDirection() == TextDirection::LeftToRight );

	// The override survives ancestor changes.
	root->setDirection( UIDirection::LeftToRight );
	root->setDirection( UIDirection::RightToLeft );
	EXPECT_TRUE( grandChild->isLTR() );

	child->clearDirectionOverride();
	EXPECT_TRUE( child->isRTL() );
	EXPECT_TRUE( grandChild->isRTL() );

	// Reparenting into a differently directed parent updates the moved subtree.
	UIWidget* ltrParent = fixedWidget( root, 100, 100 );
	ltrParent->setDirection( UIDirection::LeftToRight );
	child->setParent( ltrParent );
	EXPECT_TRUE( child->isLTR() );
	EXPECT_TRUE( grandChild->isLTR() );
	child->setParent( parent );
	EXPECT_TRUE( grandChild->isRTL() );

	root->clearDirectionOverride();
	EXPECT_TRUE( grandChild->isLTR() );
	EXPECT_FALSE( grandChild->isDirectionExplicit() );
}

UTEST( UIDirection, secondaryWindowAttachmentAndReparentingRefreshDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* primary = app.getUI();
	auto* secondary = app.createWindow( rtlWindowSettings() );
	ASSERT_TRUE( secondary != nullptr );
	secondary->setDirection( UIDirection::RightToLeft );
	EXPECT_TRUE( primary->getDirection() == UIDirection::LeftToRight );

	// Widgets must be created in their destination window's scene: cross-window rebinding is
	// not supported.
	auto context = secondary->makeCurrent();
	auto* parent = fixedWidget( secondary->getRoot(), 100, 100 );
	auto* ltrHost = fixedWidget( secondary->getRoot(), 100, 100 );
	ltrHost->setDirection( UIDirection::LeftToRight );
	auto* child = UITextInput::New();
	child->setParent( parent );
	EXPECT_TRUE( parent->getUISceneNode() == secondary );
	EXPECT_TRUE( child->getUISceneNode() == secondary );
	EXPECT_TRUE( child->isRTL() );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::RightToLeft );

	parent->setParent( ltrHost );
	EXPECT_TRUE( child->isLTR() );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::LeftToRight );

	parent->setParent( secondary->getRoot() );
	EXPECT_TRUE( child->getUISceneNode() == secondary );
	EXPECT_TRUE( child->isRTL() );
	secondary->setDirection( UIDirection::LeftToRight );
	EXPECT_TRUE( child->isLTR() );
	EXPECT_TRUE( child->isDirectionExplicit() );
}

UTEST( UIDirection, cssDirectionAndDirAttribute ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox id="outer" direction="rtl" layout_width="200dp" layout_height="100dp">
			<widget id="inherits" layout_width="10dp" layout_height="10dp" />
			<widget id="attr" dir="ltr" layout_width="10dp" layout_height="10dp" />
			<widget id="styled" class="ltr" layout_width="10dp" layout_height="10dp" />
		</vbox>
	)xml" );
	app.getUI()->combineStyleSheet( ".ltr { direction: ltr; }" );
	app.getUI()->updateDirtyLayouts();

	EXPECT_TRUE( findWidget( app, "outer" )->isRTL() );
	EXPECT_TRUE( findWidget( app, "inherits" )->isRTL() );
	EXPECT_TRUE( findWidget( app, "attr" )->isLTR() );
	EXPECT_TRUE( findWidget( app, "styled" )->isLTR() );
	EXPECT_STDSTREQ( "rtl",
					 findWidget( app, "outer" )
						 ->getPropertyString(
							 StyleSheetSpecification::instance()->getProperty( "direction" ) ) );
}

// --- Logical alignment ------------------------------------------------------------------------

UTEST( UILogicalAlignment, textAlignStartEndResolveAgainstDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox layout_width="200dp" layout_height="200dp">
			<TextView id="start" text="start" text-align="start" />
			<TextView id="end" text="end" text-align="end" />
			<TextView id="left" text="left" text-align="left" />
			<TextView id="right" text="right" text-align="right" />
			<TextView id="default" text="default" />
		</vbox>
	)xml" );
	auto halign = [&]( const char* id ) { return findWidget( app, id )->getHorizontalAlign(); };
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, halign( "start" ) );
	EXPECT_EQ( (Uint32)UI_HALIGN_RIGHT, halign( "end" ) );
	EXPECT_TRUE( findWidget( app, "start" )->getTextAlignment() == UIHorizontalAlignment::Start );

	app.getUI()->setDirection( UIDirection::RightToLeft );
	EXPECT_EQ( (Uint32)UI_HALIGN_RIGHT, halign( "start" ) );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, halign( "end" ) );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, halign( "left" ) );
	EXPECT_EQ( (Uint32)UI_HALIGN_RIGHT, halign( "right" ) );
	// Widgets default to the logical start alignment.
	EXPECT_EQ( (Uint32)UI_HALIGN_RIGHT, halign( "default" ) );
	EXPECT_STDSTREQ( "start",
					 findWidget( app, "start" )
						 ->getPropertyString(
							 StyleSheetSpecification::instance()->getProperty( "text-align" ) ) );

	app.getUI()->setDirection( UIDirection::LeftToRight );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, halign( "start" ) );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, halign( "default" ) );
}

UTEST( UILogicalAlignment, physicalSettersReplaceLogicalAlignment ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	UIWidget* widget = fixedWidget( app.getUI()->getRoot(), 10, 10 );
	widget->setHorizontalAlignment( UIHorizontalAlignment::End );
	widget->setHorizontalAlign( UI_HALIGN_LEFT );
	app.getUI()->setDirection( UIDirection::RightToLeft );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, widget->getHorizontalAlign() );
	EXPECT_TRUE( widget->getHorizontalAlignment() == UIHorizontalAlignment::Left );
}

UTEST( UILogicalAlignment, layoutGravityStartEndInVerticalLayout ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox id="box" layout_width="200dp" layout_height="200dp" padding="0dp 4dp 0dp 2dp">
			<widget id="default" layout_width="20dp" layout_height="10dp" />
			<widget id="end" layout_width="20dp" layout_height="10dp" layout_gravity="end" />
			<widget id="left" layout_width="20dp" layout_height="10dp" layout_gravity="left" />
			<widget id="right" layout_width="20dp" layout_height="10dp" layout_gravity="right" />
		</vbox>
	)xml" );
	app.getUI()->updateDirtyLayouts();
	auto x = [&]( const char* id ) { return findWidget( app, id )->getPixelsPosition().x; };
	EXPECT_NEAR( 2.f, x( "default" ), 0.1f );
	EXPECT_NEAR( 176.f, x( "end" ), 0.1f );
	EXPECT_NEAR( 2.f, x( "left" ), 0.1f );
	EXPECT_NEAR( 176.f, x( "right" ), 0.1f );

	app.getUI()->setDirection( UIDirection::RightToLeft );
	app.getUI()->updateDirtyLayouts();
	EXPECT_NEAR( 176.f, x( "default" ), 0.1f );
	EXPECT_NEAR( 2.f, x( "end" ), 0.1f );
	EXPECT_NEAR( 2.f, x( "left" ), 0.1f );
	EXPECT_NEAR( 176.f, x( "right" ), 0.1f );
	EXPECT_STDSTREQ(
		"end|top", findWidget( app, "end" )
					   ->getPropertyString(
						   StyleSheetSpecification::instance()->getProperty( "layout-gravity" ) ) );
}

// --- Logical margins and padding ----------------------------------------------------------------

UTEST( UILogicalBox, marginInlineResolvesAndFollowsDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox layout_width="200dp" layout_height="200dp">
			<widget id="a" layout_width="20dp" layout_height="10dp"
					margin-inline-start="8dp" margin-inline-end="3dp" />
			<widget id="p" layout_width="20dp" layout_height="10dp"
					padding-inline-start="6dp" padding-left="1dp" />
		</vbox>
	)xml" );
	UIWidget* a = findWidget( app, "a" );
	UIWidget* p = findWidget( app, "p" );
	EXPECT_NEAR( 8.f, a->getLayoutMargin().Left, 0.01f );
	EXPECT_NEAR( 3.f, a->getLayoutMargin().Right, 0.01f );
	// padding-left is declared after padding-inline-start, so it wins the left edge in LTR.
	EXPECT_NEAR( 1.f, p->getPadding().Left, 0.01f );

	app.getUI()->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 3.f, a->getLayoutMargin().Left, 0.01f );
	EXPECT_NEAR( 8.f, a->getLayoutMargin().Right, 0.01f );
	EXPECT_NEAR( 6.f, p->getPadding().Right, 0.01f );
	EXPECT_NEAR( 1.f, p->getPadding().Left, 0.01f );

	app.getUI()->setDirection( UIDirection::LeftToRight );
	EXPECT_NEAR( 8.f, a->getLayoutMargin().Left, 0.01f );
	EXPECT_NEAR( 3.f, a->getLayoutMargin().Right, 0.01f );
	EXPECT_NEAR( 1.f, p->getPadding().Left, 0.01f );
	// The right edge is restored once the logical declaration no longer resolves to it.
	EXPECT_NEAR( 0.f, p->getPadding().Right, 0.01f );
}

UTEST( UILogicalBox, physicalAndLogicalDeclarationsCascade ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox layout_width="200dp" layout_height="200dp">
			<widget id="later" layout_width="20dp" layout_height="10dp" />
			<widget id="earlier" layout_width="20dp" layout_height="10dp" />
			<widget id="specific" class="spec" layout_width="20dp" layout_height="10dp" />
		</vbox>
	)xml" );
	app.getUI()->combineStyleSheet( R"css(
		#later { margin-left: 4dp; margin-inline-start: 8dp; }
		#earlier { margin-inline-start: 8dp; margin-left: 4dp; }
		widget.spec { margin-left: 4dp; }
		.spec { margin-inline-start: 8dp; }
	)css" );
	app.getUI()->updateDirtyLayouts();

	// Same specificity: the declaration that appears later wins.
	EXPECT_NEAR( 8.f, findWidget( app, "later" )->getLayoutMargin().Left, 0.01f );
	EXPECT_NEAR( 4.f, findWidget( app, "earlier" )->getLayoutMargin().Left, 0.01f );
	// Higher specificity wins regardless of order.
	EXPECT_NEAR( 4.f, findWidget( app, "specific" )->getLayoutMargin().Left, 0.01f );

	// In RTL the logical start edge is the right edge, so there is no conflict.
	app.getUI()->setDirection( UIDirection::RightToLeft );
	for ( const char* id : { "later", "earlier", "specific" } ) {
		EXPECT_NEAR( 4.f, findWidget( app, id )->getLayoutMargin().Left, 0.01f );
		EXPECT_NEAR( 8.f, findWidget( app, id )->getLayoutMargin().Right, 0.01f );
	}

	app.getUI()->setDirection( UIDirection::LeftToRight );
	EXPECT_NEAR( 8.f, findWidget( app, "later" )->getLayoutMargin().Left, 0.01f );
	EXPECT_NEAR( 0.f, findWidget( app, "later" )->getLayoutMargin().Right, 0.01f );
	EXPECT_NEAR( 4.f, findWidget( app, "earlier" )->getLayoutMargin().Left, 0.01f );
}

UTEST( UILogicalBox, marginInlineShorthand ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox layout_width="200dp" layout_height="200dp" direction="rtl">
			<widget id="w" layout_width="20dp" layout_height="10dp" margin-inline="5dp 7dp" />
		</vbox>
	)xml" );
	UIWidget* w = findWidget( app, "w" );
	EXPECT_NEAR( 5.f, w->getLayoutMargin().Right, 0.01f );
	EXPECT_NEAR( 7.f, w->getLayoutMargin().Left, 0.01f );
}

UTEST( UILogicalBox, directDeclarationsOwnValuesAndRestoreNativeEdges ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10 );
	widget->setPaddingLeft( 6.25f );
	widget->setPaddingRight( 2.5f );
	widget->setLayoutMarginLeft( 4.f );
	widget->setLayoutMarginRight( 7.f );
	widget->setLayoutMarginLeftAuto( true );
	{
		// Both declarations outlive their temporary source properties. An explicit zero is a
		// declaration, not absence, and must replace the native auto margin.
		StyleSheetProperty padding( "padding-inline-start", "20dp" );
		StyleSheetProperty margin( "margin-inline-start", "0dp" );
		widget->applyProperty( padding );
		widget->applyProperty( margin );
	}
	for ( int round = 0; round < 2; ++round ) {
		EXPECT_NEAR( 20.f, widget->getPadding().Left, 0.01f );
		EXPECT_NEAR( 2.5f, widget->getPadding().Right, 0.01f );
		EXPECT_NEAR( 0.f, widget->getLayoutMargin().Left, 0.01f );
		EXPECT_NEAR( 7.f, widget->getLayoutMargin().Right, 0.01f );
		EXPECT_FALSE( widget->hasLayoutMarginLeftAuto() );
		widget->setDirection( UIDirection::RightToLeft );
		EXPECT_NEAR( 6.25f, widget->getPadding().Left, 0.01f );
		EXPECT_NEAR( 20.f, widget->getPadding().Right, 0.01f );
		EXPECT_TRUE( widget->hasLayoutMarginLeftAuto() );
		EXPECT_FALSE( widget->hasLayoutMarginRightAuto() );
		EXPECT_NEAR( 0.f, widget->getLayoutMargin().Right, 0.01f );
		EXPECT_STREQ( "20dp",
					  widget->getBoxEdgeProperty( PropertyId::PaddingRight )->value().c_str() );
		widget->setDirection( UIDirection::LeftToRight );
	}
}

UTEST( UILogicalBox, styleDeclarationsFollowVariablesAndReplacement ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10, "variables" );
	app.getUI()->combineStyleSheet( R"css(
		#variables { --spacing: 12dp; padding-inline-start: var(--spacing); }
	)css" );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 12.f, widget->getPadding().Left, 0.01f );
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 0.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 12.f, widget->getPadding().Right, 0.01f );
	widget->getUIStyle()->setStyleSheetVariable( StyleSheetVariable( "--spacing", "18dp" ) );
	widget->setDirection( UIDirection::LeftToRight );
	EXPECT_NEAR( 18.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 0.f, widget->getPadding().Right, 0.01f );
	// Replace the source definition while the logical edge block remains in use.
	app.getUI()->setStyleSheet( std::string( "#variables { padding-inline-start: 24dp; }" ) );
	app.getUI()->flushDirtyStyleAndLayout();
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 0.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 24.f, widget->getPadding().Right, 0.01f );
}

UTEST( UILogicalBox, intermediateValuesRemainOwnedUntilStyleRefresh ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10, "intermediate" );
	app.getUI()->combineStyleSheet( R"css(
		#intermediate { padding-inline-start: 20dp; }
		#intermediate:selected { padding-inline-start: 20dp; }
	)css" );
	app.getUI()->flushDirtyStyleAndLayout();
	{
		// An animation's intermediate value must not be replaced by its CSS target on a
		// direction change, even when it carries the same cascade metadata.
		StyleSheetProperty intermediate(
			*widget->getUIStyle()->getCascadedProperty( PropertyId::PaddingInlineStart ) );
		// setValue() does not refresh the value hash by default. Sharing must compare the
		// actual value rather than mistake this intermediate value for its original target.
		intermediate.setValue( "8dp" );
		widget->applyProperty( intermediate );
	}
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 0.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 8.f, widget->getPadding().Right, 0.01f );
	widget->pushState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 20.f, widget->getPadding().Right, 0.01f );
}

UTEST( UILogicalBox, removedLastDeclarationReleasesStorage ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = eeNew( LogicalBoxWidgetProbe, () );
	widget->setId( "temporary" );
	widget->setParent( app.getUI()->getRoot() );
	app.getUI()->combineStyleSheet( "#temporary:selected { padding-inline-start: 10dp; }" );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_FALSE( widget->hasLogicalBox() );
	widget->pushState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_TRUE( widget->hasLogicalBox() );
	widget->popState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_FALSE( widget->hasLogicalBox() );
	EXPECT_NEAR( 0.f, widget->getPadding().Left, 0.01f );
	widget->pushState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_TRUE( widget->hasLogicalBox() );
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 0.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 10.f, widget->getPadding().Right, 0.01f );
}

// --- Horizontal flow --------------------------------------------------------------------------

UTEST( UIHorizontalFlow, horizontalLinearLayoutMirrorsWithoutReorderingNodes ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	UILinearLayout* row = UILinearLayout::NewHorizontal();
	row->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	row->setPixelsSize( 200, 40 );
	row->setPadding( { 3, 0, 5, 0 } );
	row->setParent( app.getUI()->getRoot() );
	UIWidget* a = fixedWidget( row, 20, 10, "a" );
	UIWidget* b = fixedWidget( row, 30, 10, "b" );
	UIWidget* c = fixedWidget( row, 40, 10, "c" );
	b->setLayoutMargin( { 2, 0, 6, 0 } );
	app.getUI()->updateDirtyLayouts();

	const Float ax = a->getPixelsPosition().x;
	const Float bx = b->getPixelsPosition().x;
	const Float cx = c->getPixelsPosition().x;
	EXPECT_NEAR( 3.f, ax, 0.1f );
	EXPECT_NEAR( 25.f, bx, 0.1f );
	EXPECT_NEAR( 61.f, cx, 0.1f );

	app.getUI()->setDirection( UIDirection::RightToLeft );
	app.getUI()->updateDirtyLayouts();
	// Child zero reaches the right content edge; physical padding and margins stay physical.
	EXPECT_NEAR( 97.f, c->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 139.f, b->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 175.f, a->getPixelsPosition().x, 0.1f );
	EXPECT_TRUE( row->getFirstChild() == a );
	EXPECT_TRUE( row->getLastChild() == c );

	app.getUI()->setDirection( UIDirection::LeftToRight );
	app.getUI()->updateDirtyLayouts();
	EXPECT_NEAR( ax, a->getPixelsPosition().x, 0.01f );
	EXPECT_NEAR( bx, b->getPixelsPosition().x, 0.01f );
	EXPECT_NEAR( cx, c->getPixelsPosition().x, 0.01f );
}

UTEST( UIHorizontalFlow, relativeLayoutStartAndEndOf ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<RelativeLayout id="rl" layout_width="200dp" layout_height="100dp">
			<widget id="anchor" layout_width="20dp" layout_height="10dp"
					layout_gravity="center" />
			<widget id="before" layout_width="10dp" layout_height="10dp"
					layout-to-start-of="anchor" />
			<widget id="after" layout_width="10dp" layout_height="10dp"
					layout-to-end-of="anchor" />
		</RelativeLayout>
	)xml" );
	app.getUI()->updateDirtyLayouts();
	UIWidget* anchor = findWidget( app, "anchor" );
	UIWidget* before = findWidget( app, "before" );
	UIWidget* after = findWidget( app, "after" );
	EXPECT_LT( before->getPixelsPosition().x, anchor->getPixelsPosition().x );
	EXPECT_GT( after->getPixelsPosition().x, anchor->getPixelsPosition().x );

	app.getUI()->setDirection( UIDirection::RightToLeft );
	app.getUI()->updateDirtyLayouts();
	EXPECT_NEAR( anchor->getPixelsPosition().x + 20.f, before->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( anchor->getPixelsPosition().x - 10.f, after->getPixelsPosition().x, 0.1f );
	EXPECT_TRUE( before->getLayoutPositionPolicy() == PositionPolicy::StartOf );
	EXPECT_TRUE( before->getResolvedLayoutPositionPolicy() == PositionPolicy::RightOf );
}

UTEST( UIHorizontalFlow, htmlFlexRowFollowsDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( R"html(
		<html><head><style>
			.row { display: flex; width: 300px; }
			.row div { width: 50px; height: 10px; }
		</style></head><body>
			<div id="ltr" class="row"><div id="l1"></div><div id="l2"></div></div>
			<div id="rtl" class="row" dir="rtl"><div id="r1"></div><div id="r2"></div></div>
			<div id="rtlrev" class="row" style="flex-direction: row-reverse" dir="rtl">
				<div id="v1"></div><div id="v2"></div></div>
			<div id="rtlauto" class="row" dir="rtl">
				<div id="a1"></div><div id="a2" style="margin-inline-start: auto"></div></div>
		</body></html>
	)html" ) );
	app.getUI()->updateDirtyLayouts();
	auto x = [&]( const char* id ) { return findWidget( app, id )->getPixelsPosition().x; };
	EXPECT_NEAR( 0.f, x( "l1" ), 0.5f );
	EXPECT_NEAR( 50.f, x( "l2" ), 0.5f );
	EXPECT_NEAR( 250.f, x( "r1" ), 0.5f );
	EXPECT_NEAR( 200.f, x( "r2" ), 0.5f );
	// row-reverse in RTL starts at the left edge.
	EXPECT_NEAR( 0.f, x( "v1" ), 0.5f );
	EXPECT_NEAR( 50.f, x( "v2" ), 0.5f );
	// An inline-start auto margin pushes the item to the inline-end (left) side.
	EXPECT_NEAR( 250.f, x( "a1" ), 0.5f );
	EXPECT_NEAR( 0.f, x( "a2" ), 0.5f );
}

UTEST( UIHorizontalFlow, htmlGridPlacesColumnsFromInlineStart ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( R"html(
		<html><body>
			<div id="grid" dir="rtl" style="display: grid; width: 300px;
				grid-template-columns: 100px 100px 100px;">
				<div id="g1" style="height: 10px"></div>
				<div id="g2" style="height: 10px"></div>
				<div id="g3" style="height: 10px; width: 40px; justify-self: start"></div>
				<div id="g4" style="height: 10px; width: 40px; justify-self: end"></div>
			</div>
		</body></html>
	)html" ) );
	app.getUI()->updateDirtyLayouts();
	auto x = [&]( const char* id ) { return findWidget( app, id )->getPixelsPosition().x; };
	EXPECT_NEAR( 200.f, x( "g1" ), 0.5f );
	EXPECT_NEAR( 100.f, x( "g2" ), 0.5f );
	EXPECT_NEAR( 60.f, x( "g3" ), 0.5f );
	EXPECT_NEAR( 200.f, x( "g4" ), 0.5f );
}

// --- HTML integration ---------------------------------------------------------------------------

UTEST( UIHTMLDirection, dirAttributeAndTextAlignStart ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( R"html(
		<html><body dir="rtl" style="width: 400px">
			<p id="p" style="width: 400px">abc</p>
			<p id="ltr" dir="ltr" style="width: 400px">abc</p>
			<p id="left" style="width: 400px; text-align: left">abc</p>
		</body></html>
	)html" ) );
	app.getUI()->updateDirtyLayouts();
	auto lineX = [&]( const char* id ) {
		auto* rt = findWidget( app, id )->asType<UIRichText>();
		const auto& lines = rt->getRichText().getLines();
		return lines.empty() || lines[0].spans.empty() ? -1.f : lines[0].spans[0].position.x;
	};
	EXPECT_TRUE( findWidget( app, "p" )->isRTL() );
	EXPECT_TRUE( findWidget( app, "ltr" )->isLTR() );
	EXPECT_GT( lineX( "p" ), 300.f );
	EXPECT_NEAR( 0.f, lineX( "ltr" ), 0.5f );
	EXPECT_NEAR( 0.f, lineX( "left" ), 0.5f );
	EXPECT_TRUE( findWidget( app, "p" )->asType<UIRichText>()->getRichText().getBaseDirection() ==
				 TextDirection::RightToLeft );
}

// --- Rich text line ordering --------------------------------------------------------------------

UTEST( UIRichTextDirection, rightToLeftLinesReverseItemsButKeepLatinRuns ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	EXPECT_TRUE( TextLayout::getFirstStrongDirection( String( "  123 abc" ).view() ) ==
				 TextDirection::LeftToRight );
	EXPECT_TRUE(
		TextLayout::getFirstStrongDirection(
			String::fromUtf8( std::string( "١٢ \xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d" ) ).view() ) ==
		TextDirection::RightToLeft );
	EXPECT_TRUE( TextLayout::getFirstStrongDirection( String( " 42 !" ).view() ) ==
				 TextDirection::Unspecified );

	RichText richText;
	FontStyleConfig style;
	style.Font = app.getUI()->getUIThemeManager()->getDefaultFont();
	style.CharacterSize = 12;
	richText.setFontStyleConfig( style );
	// Logical order: Latin, Latin, Hebrew.
	richText.addSpan( String( "one " ), style );
	richText.addSpan( String( "two " ), style );
	richText.addSpan( String::fromUtf8( std::string( "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d" ) ),
					  style );
	richText.setMaxWidth( 500 );
	richText.setAlign( TEXT_ALIGN_RIGHT );
	richText.setBaseDirection( TextDirection::RightToLeft );
	richText.updateLayout();
	const auto& lines = richText.getLines();
	ASSERT_EQ( (size_t)1, lines.size() );
	ASSERT_EQ( (size_t)3, lines[0].spans.size() );
	const auto& one = lines[0].spans[0];
	const auto& two = lines[0].spans[1];
	const auto& hebrew = lines[0].spans[2];
	// The Hebrew item is visually first (leftmost) and the Latin run keeps its logical order.
	EXPECT_LT( hebrew.position.x, one.position.x );
	EXPECT_LT( one.position.x, two.position.x );
	// The line is aligned against the right edge.
	EXPECT_NEAR( 500.f, two.position.x + two.size.getWidth(), 0.5f );

	richText.setBaseDirection( TextDirection::Unspecified );
	richText.updateLayout();
	EXPECT_LT( richText.getLines()[0].spans[0].position.x,
			   richText.getLines()[0].spans[2].position.x );
}

// --- Standard controls ------------------------------------------------------------------------

UTEST( UIRTLControls, checkBoxIndicatorAtInlineStart ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	for ( int type = 0; type < 2; ++type ) {
		UITextView* box = type == 0 ? static_cast<UITextView*>( UICheckBox::New() )
									: static_cast<UITextView*>( UIRadioButton::New() );
		box->setParent( app.getUI()->getRoot() );
		box->setText( "Option" );
		box->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		box->setPixelsSize( 200, 24 );
		box->setPadding( Rectf() );
		UIWidget* indicator = type == 0 ? box->asType<UICheckBox>()->getCurrentButton()
										: box->asType<UIRadioButton>()->getInactiveButton();
		app.getUI()->updateDirtyLayouts();
		EXPECT_NEAR( 0.f, indicator->getPixelsPosition().x, 0.5f );
		EXPECT_GT( box->getRealAlignOffset().x, indicator->getPixelsSize().getWidth() - 0.5f );

		box->setDirection( UIDirection::RightToLeft );
		app.getUI()->updateDirtyLayouts();
		const Float indicatorLeft = indicator->getPixelsPosition().x;
		EXPECT_NEAR( 200.f, indicatorLeft + indicator->getPixelsSize().getWidth(), 0.5f );
		// Start-aligned text ends just before the indicator.
		const Float textRight = box->getRealAlignOffset().x + box->getTextWidth();
		EXPECT_LE( textRight, indicatorLeft + 0.5f );
		EXPECT_GT( textRight, indicatorLeft - 16.f );
	}
}

UTEST( UIRTLControls, pushButtonIconFollowsInlineOrder ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	UIPushButton* button = UIPushButton::New();
	button->setParent( app.getUI()->getRoot() );
	button->setText( "Save" );
	button->getIcon()->setPixelsSize( 16, 16 );
	button->getIcon()->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	button->getIcon()->setVisible( true );
	button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	button->setPixelsSize( 160, 24 );
	app.getUI()->updateDirtyLayouts();
	EXPECT_LT( button->getIcon()->getPixelsPosition().x,
			   button->getTextView()->getPixelsPosition().x );

	button->setDirection( UIDirection::RightToLeft );
	app.getUI()->updateDirtyLayouts();
	EXPECT_GT( button->getIcon()->getPixelsPosition().x,
			   button->getTextView()->getPixelsPosition().x );
}

UTEST( UIRTLControls, windowTitlePreservesLogicalAlignment ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	for ( auto alignment : { UIHorizontalAlignment::Start, UIHorizontalAlignment::End,
							 UIHorizontalAlignment::Left, UIHorizontalAlignment::Center } ) {
		auto* window = UIWindow::New();
		window->setHorizontalAlignment( alignment );
		window->setTitle( "Window title" );
		auto* title = window->getTitleTextBox();
		ASSERT_TRUE( title != nullptr );
		EXPECT_TRUE( title->getTextAlignment() == alignment );
		for ( auto direction : { UIDirection::RightToLeft, UIDirection::LeftToRight } ) {
			window->setDirection( direction );
			EXPECT_EQ( resolveHorizontalAlignment( alignment, direction ),
					   title->getHorizontalAlign() );
			EXPECT_TRUE( title->getTextAlignment() == alignment );
		}
	}
}

UTEST( UIRTLControls, menuShortcutAndSubMenuArrowAtInlineEnd ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->setDirection( UIDirection::RightToLeft );
	UIPopUpMenu* menu = UIPopUpMenu::New();
	UIMenuItem* item = menu->add( "Save", nullptr, "Ctrl+S" );
	UIMenuSubMenu* sub = menu->addSubMenu( "More", nullptr, UIPopUpMenu::New() );
	menu->showAtScreenPosition( { 300, 100 } );
	app.getUI()->updateDirtyLayouts();

	ASSERT_TRUE( item->getShortcutView() != nullptr );
	EXPECT_LT( item->getShortcutView()->getPixelsPosition().x,
			   item->getTextView()->getPixelsPosition().x );
	EXPECT_LT( sub->getArrow()->getPixelsPosition().x, sub->getTextView()->getPixelsPosition().x );
	EXPECT_TRUE( sub->getArrow()->isMirroredOnRTL() );
	EXPECT_LT( sub->getArrow()->getScale().x, 0.f );

	sub->showSubMenu();
	app.getUI()->updateDirtyLayouts();
	Vector2f subPos( sub->getSubMenu()->getPixelsPosition() );
	sub->getSubMenu()->getParent()->nodeToWorldTranslation( subPos );
	Vector2f menuPos( menu->getPixelsPosition() );
	menu->getParent()->nodeToWorldTranslation( menuPos );
	// The submenu prefers the inline-end (left) side of its parent menu.
	EXPECT_LE( subPos.x + sub->getSubMenu()->getPixelsSize().getWidth(), menuPos.x + 0.5f );
}

UTEST( UIRTLControls, tabsStartAtInlineStartAndKeepLogicalIndexes ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	UITabWidget* tabs = UITabWidget::New();
	tabs->setParent( app.getUI()->getRoot() );
	tabs->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	tabs->setPixelsSize( 400, 200 );
	tabs->add( "One", UIWidget::New() );
	tabs->add( "Two", UIWidget::New() );
	tabs->add( "Three", UIWidget::New() );
	app.getUI()->updateDirtyLayouts();
	EXPECT_LT( tabs->getTab( 0 )->getPixelsPosition().x, tabs->getTab( 1 )->getPixelsPosition().x );

	tabs->setDirection( UIDirection::RightToLeft );
	app.getUI()->updateDirtyLayouts();
	UITab* first = tabs->getTab( 0 );
	EXPECT_GT( first->getPixelsPosition().x, tabs->getTab( 1 )->getPixelsPosition().x );
	EXPECT_GT( tabs->getTab( 1 )->getPixelsPosition().x, tabs->getTab( 2 )->getPixelsPosition().x );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(),
				 first->getPixelsPosition().x + first->getPixelsSize().getWidth(), 0.5f );
	EXPECT_TRUE( tabs->getTabIndex( first ) == 0 );
}

// --- Text editing audit -------------------------------------------------------------------------

namespace {
const char* sHebrewShalom = "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d"; // שלום

FontTrueType* loadHebrewFallback() {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* hebrewFont =
		FontTrueType::New( "NotoSansHebrew-Regular-RTL", "assets/fonts/NotoSansHebrew-Regular.ttf" )
			.get();
	defaultResourceScope().getFontService().addFallbackFont( hebrewFont );
	return hebrewFont;
}
} // namespace

UTEST( UIRTLTextEditing, rightToLeftCaretPositionsAndHitTesting ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	loadHebrewFallback();

	Text text;
	text.setFont( app.getUI()->getUIThemeManager()->getDefaultFont() );
	text.setFontSize( 16 );
	text.setDirection( TextDirection::RightToLeft );
	text.setString( String::fromUtf8( std::string( sHebrewShalom ) ) );
	const size_t len = text.getString().size();
	ASSERT_GT( text.getTextWidth(), 0.f );

	// Logical order advances leftwards: the first caret is at the right edge, the last at x=0.
	for ( size_t i = 0; i < len; ++i )
		EXPECT_GT( text.findCharacterPos( i ).x, text.findCharacterPos( i + 1 ).x );
	EXPECT_NEAR( text.getTextWidth(), text.findCharacterPos( 0 ).x, 1.f );
	EXPECT_NEAR( 0.f, text.findCharacterPos( len ).x, 1.f );

	// Hit testing maps caret positions back to the same logical index.
	for ( size_t i = 0; i <= len; ++i ) {
		Vector2f pos = text.findCharacterPos( i );
		EXPECT_EQ( (Int32)i, text.findCharacterFromPos( Vector2i( (int)pos.x, (int)pos.y + 2 ) ) );
	}
}

UTEST( UIRTLTextEditing, mixedDirectionRunsKeepLogicalOrderInsideRuns ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	loadHebrewFallback();

	Text text;
	text.setFont( app.getUI()->getUIThemeManager()->getDefaultFont() );
	text.setFontSize( 16 );
	text.setDirection( TextDirection::RightToLeft );
	// Logical: "abc " followed by Hebrew. In an RTL paragraph the Latin run is visually first
	// from the right and keeps its internal left-to-right order.
	text.setString( String::fromUtf8( std::string( "abc " ) + sHebrewShalom ) );
	EXPECT_LT( text.findCharacterPos( 0 ).x, text.findCharacterPos( 1 ).x );
	EXPECT_LT( text.findCharacterPos( 1 ).x, text.findCharacterPos( 2 ).x );
	EXPECT_GT( text.findCharacterPos( 0 ).x, text.findCharacterPos( 5 ).x );
	EXPECT_GT( text.findCharacterPos( 5 ).x, text.findCharacterPos( 6 ).x );
}

UTEST( UIRTLTextEditing, textInputUsesDirectionAsBaseAndStartAlignment ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	loadHebrewFallback();
	app.getUI()->setDirection( UIDirection::RightToLeft );
	UITextInput* input = UITextInput::New();
	input->setParent( app.getUI()->getRoot() );
	input->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	input->setPixelsSize( 200, 24 );
	input->setText( String::fromUtf8( std::string( sHebrewShalom ) ) );
	app.getUI()->updateDirtyLayouts();
	EXPECT_TRUE( input->getTextCache()->getDirection() == TextDirection::RightToLeft );
	EXPECT_EQ( (Uint32)UI_HALIGN_RIGHT, input->getHorizontalAlign() );

	input->setDirection( UIDirection::LeftToRight );
	EXPECT_TRUE( input->getTextCache()->getDirection() == TextDirection::LeftToRight );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, input->getHorizontalAlign() );
}

UTEST( UIRTLTextEditing, tooltipUsesInheritedTextBaseDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->setDirection( UIDirection::RightToLeft );
	auto* tooltip = UITooltip::New();
	tooltip->setText( "Tooltip text" );
	EXPECT_TRUE( tooltip->getTextCache()->getDirection() == TextDirection::RightToLeft );
	app.getUI()->setDirection( UIDirection::LeftToRight );
	EXPECT_TRUE( tooltip->getTextCache()->getDirection() == TextDirection::LeftToRight );
	tooltip->setDirection( UIDirection::RightToLeft );
	app.getUI()->getRoot()->clearDirectionOverride();
	EXPECT_TRUE( tooltip->getTextCache()->getDirection() == TextDirection::RightToLeft );
	tooltip->clearDirectionOverride();
	EXPECT_TRUE( tooltip->getTextCache()->getDirection() == TextDirection::Unspecified );
}

UTEST( UIHTMLDirection, tablesListsAndFormsFollowDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( Tools::HTMLFormatter::HTMLtoXML( R"html(
		<html><body dir="rtl" style="width: 400px">
			<table id="table" cellspacing="0"><tr>
				<td id="c1" style="width: 50px">a</td>
				<td id="c2" style="width: 80px">b</td>
			</tr></table>
			<ul id="list"><li id="li">item</li></ul>
			<input id="input" type="text" value="abc" />
		</body></html>
	)html" ) );
	app.getUI()->updateDirtyLayouts();
	UIWidget* c1 = findWidget( app, "c1" );
	UIWidget* c2 = findWidget( app, "c2" );
	ASSERT_TRUE( c1 != nullptr && c2 != nullptr );
	// The first column is at the inline-start (right) side of the table.
	EXPECT_GT( c1->getPixelsPosition().x, c2->getPixelsPosition().x );
	EXPECT_NEAR( c2->getPixelsPosition().x + c2->getPixelsSize().getWidth(),
				 c1->getPixelsPosition().x, 0.5f );

	// List indentation uses padding-inline-start: the right padding in RTL.
	UIWidget* list = findWidget( app, "list" );
	EXPECT_GT( list->getPixelsPadding().Right, 0.f );
	EXPECT_NEAR( 0.f, list->getPixelsPadding().Left, 0.01f );
	EXPECT_TRUE( findWidget( app, "li" )->isRTL() );
	EXPECT_TRUE( findWidget( app, "input" )->isRTL() );
}

UTEST( UIRTLTextEditing, shapedGlyphsAfterComplexScriptRunsStayOnPixelGrid ) {
	BoolScopedOp op( Text::TextShaperEnabled, true );
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	loadHebrewFallback();
	Font* font = app.getUI()->getUIThemeManager()->getDefaultFont();
	// Complex-script runs advance the pen by fractional HarfBuzz advances; Latin glyphs placed
	// after them must still be snapped to whole pixels or they render blurred.
	for ( auto direction : { TextDirection::LeftToRight, TextDirection::RightToLeft } ) {
		auto layout = TextLayout::layout(
			String::fromUtf8( std::string( sHebrewShalom ) + " / Second Third" ), font, 13,
			Text::Regular, 4, 0.f, {}, 0, direction );
		size_t glyphs = 0;
		for ( const auto& paragraph : layout->paragraphs ) {
			for ( const auto& glyph : paragraph.shapedGlyphs ) {
				EXPECT_EQ( glyph.position.x, std::round( glyph.position.x ) );
				glyphs++;
			}
		}
		EXPECT_GT( glyphs, (size_t)10 );
	}
}

UTEST( UIDirection, reparentingNonUIAncestorRefreshesDescendants ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* ltr = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* rtl = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	rtl->setDirection( UIDirection::RightToLeft );
	auto* bridge = Node::New();
	bridge->setParent( ltr );
	auto* child = fixedWidget( bridge, 10, 10 );
	auto* nested = fixedWidget( child, 10, 10 );
	EXPECT_TRUE( child->isLTR() );
	bridge->setParent( rtl );
	EXPECT_TRUE( child->isRTL() );
	EXPECT_TRUE( nested->isRTL() );
	bridge->setParent( ltr );
	EXPECT_TRUE( child->isLTR() );
	EXPECT_TRUE( nested->isLTR() );
}

UTEST( UIDirection, parentChangeOverrideObservesUpdatedDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* ltr = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* rtl = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	rtl->setDirection( UIDirection::RightToLeft );
	auto* child = eeNew( ParentChangeWidgetProbe, () );
	Node* node = child;
	node->setParent( ltr );
	auto* nested = fixedWidget( child, 10, 10 );
	node->setParent( rtl );
	EXPECT_TRUE( child->observedDirection == UIDirection::RightToLeft );
	EXPECT_TRUE( nested->isRTL() );
	node->setParent( ltr );
	EXPECT_TRUE( child->observedDirection == UIDirection::LeftToRight );
	EXPECT_TRUE( nested->isLTR() );
}

UTEST( UIDirection, nonUIParentChangeOverrideObservesUpdatedDescendants ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* ltr = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* rtl = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	rtl->setDirection( UIDirection::RightToLeft );
	auto* bridge = eeNew( ParentChangeNodeProbe, () );
	bridge->setParent( ltr );
	auto* innerBridge = Node::New();
	innerBridge->setParent( bridge );
	bridge->observedChild = fixedWidget( innerBridge, 10, 10 );
	auto* nested = fixedWidget( bridge->observedChild, 10, 10 );
	auto* overridden = fixedWidget( innerBridge, 10, 10 );
	overridden->setDirection( UIDirection::LeftToRight );
	auto* overriddenChild = fixedWidget( overridden, 10, 10 );
	bridge->setParent( rtl );
	EXPECT_TRUE( bridge->observedDirection == UIDirection::RightToLeft );
	EXPECT_TRUE( nested->isRTL() );
	EXPECT_TRUE( overridden->isLTR() );
	EXPECT_TRUE( overriddenChild->isLTR() );
	bridge->setParent( ltr );
	EXPECT_TRUE( bridge->observedDirection == UIDirection::LeftToRight );
	EXPECT_TRUE( nested->isLTR() );
}

UTEST( UIDirection, reparentingNonUIAncestorPreservesNestedSceneDirection ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* ltr = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* rtl = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	rtl->setDirection( UIDirection::RightToLeft );
	auto* bridge = Node::New();
	bridge->setParent( ltr );
	auto* scene = UISceneNode::New( app.getUI()->getWindow() );
	scene->setParent( bridge );
	auto* child = fixedWidget( scene->getRoot(), 10, 10 );
	bridge->setParent( rtl );
	EXPECT_TRUE( scene->getRoot()->isLTR() );
	EXPECT_TRUE( child->isLTR() );
	scene->getRoot()->setDirection( UIDirection::RightToLeft );
	bridge->setParent( ltr );
	EXPECT_TRUE( child->isRTL() );
}

UTEST( UIDirection, matchingParentContextsSkipSubtreeNotifications ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* ltr = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* otherLTR = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* explicitLTR = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	explicitLTR->setDirection( UIDirection::LeftToRight );
	auto* otherExplicitLTR = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	otherExplicitLTR->setDirection( UIDirection::LeftToRight );
	auto* rtl = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	rtl->setDirection( UIDirection::RightToLeft );
	// Parent contexts must also resolve through ordinary nodes without visiting the moved tree.
	auto wrap = []( Node* parent ) {
		auto* node = Node::New();
		node->setParent( parent );
		return node;
	};
	auto* ltrBridge = wrap( ltr );
	auto* otherLTRBridge = wrap( otherLTR );
	auto* explicitLTRBridge = wrap( explicitLTR );
	auto* otherExplicitLTRBridge = wrap( otherExplicitLTR );
	auto* rtlBridge = wrap( rtl );
	auto* moved = eeNew( ParentChangeNodeProbe, () );
	moved->setParent( ltrBridge );
	auto* child = eeNew( ParentChangeWidgetProbe, () );
	child->setParent( moved );
	moved->observedChild = child;
	int parentNotifications = moved->parentTreeChanges;
	int childNotifications = child->parentTreeChanges;
	moved->setParent( otherLTRBridge );
	EXPECT_EQ( parentNotifications, moved->parentTreeChanges );
	EXPECT_EQ( childNotifications, child->parentTreeChanges );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::Unspecified );

	// Explicit LTR changes text base direction even though the physical UI direction is the same.
	moved->setParent( explicitLTRBridge );
	++parentNotifications;
	++childNotifications;
	EXPECT_EQ( parentNotifications, moved->parentTreeChanges );
	EXPECT_EQ( childNotifications, child->parentTreeChanges );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::LeftToRight );
	moved->setParent( otherExplicitLTRBridge );
	EXPECT_EQ( parentNotifications, moved->parentTreeChanges );
	EXPECT_EQ( childNotifications, child->parentTreeChanges );
	moved->setParent( rtlBridge );
	++parentNotifications;
	++childNotifications;
	EXPECT_EQ( parentNotifications, moved->parentTreeChanges );
	EXPECT_EQ( childNotifications, child->parentTreeChanges );
	EXPECT_TRUE( moved->observedDirection == UIDirection::RightToLeft );
	EXPECT_TRUE( child->isRTL() );
	moved->setParent( ltrBridge );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::Unspecified );
}

UTEST( UIDirection, matchingParentContextsSkipLeafRefresh ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* first = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* second = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	first->setDirection( UIDirection::RightToLeft );
	second->setDirection( UIDirection::RightToLeft );
	auto* child = eeNew( ParentChangeWidgetProbe, () );
	child->setParent( first );
	const int notifications = child->parentTreeChanges;
	child->setParent( second );
	EXPECT_EQ( notifications, child->parentTreeChanges );
	EXPECT_TRUE( child->observedDirection == UIDirection::RightToLeft );
}

UTEST( UIDirection, indexedReparentingRefreshesInheritedContextBeforeParentCallbacks ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* ltr = eeNew( ParentChangeWidgetProbe, () );
	ltr->setParent( app.getUI()->getRoot() );
	auto* rtl = eeNew( ParentChangeWidgetProbe, () );
	rtl->setParent( app.getUI()->getRoot() );
	rtl->setDirection( UIDirection::RightToLeft );
	auto* moved = eeNew( ParentChangeWidgetProbe, () );
	moved->setParent( ltr );
	auto* nested = fixedWidget( moved, 10, 10 );
	rtl->childAddAt( moved, 0 );
	EXPECT_TRUE( moved->observedDirection == UIDirection::RightToLeft );
	EXPECT_TRUE( nested->isRTL() );
	EXPECT_FALSE( moved->isDestroying() );
	const int notifications = moved->parentTreeChanges;
	rtl->childAddAt( moved, 0 );
	EXPECT_EQ( notifications, moved->parentTreeChanges );
	ltr->childAddAt( moved, 0 );
	EXPECT_TRUE( moved->observedDirection == UIDirection::LeftToRight );
	EXPECT_TRUE( nested->isLTR() );

	// Plain scene nodes must propagate the inherited context to their UI descendants too.
	auto* bridge = eeNew( ParentChangeNodeProbe, () );
	bridge->setParent( ltr );
	bridge->observedChild = fixedWidget( bridge, 10, 10 );
	rtl->childAddAt( bridge, 0 );
	EXPECT_TRUE( bridge->observedDirection == UIDirection::RightToLeft );
	EXPECT_TRUE( bridge->observedChild->isRTL() );
	bridge->detach();
	ltr->childAddAt( bridge, 0 );
	EXPECT_TRUE( bridge->observedDirection == UIDirection::LeftToRight );
	EXPECT_TRUE( bridge->observedChild->isLTR() );
}

UTEST( UIDirection, detachedSubtreeRefreshesOnAttachmentToDefaultContext ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* rtl = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	rtl->setDirection( UIDirection::RightToLeft );
	auto* bridge = Node::New();
	bridge->setParent( rtl );
	auto* child = fixedWidget( bridge, 10, 10 );
	EXPECT_TRUE( child->isRTL() );
	bridge->detach();
	bridge->setParent( app.getUI()->getRoot() );
	EXPECT_TRUE( child->isLTR() );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::Unspecified );
}

UTEST( UIDirection, reparentingDuringDirectionNotificationRefreshesPendingDescendants ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* first = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	auto* second = fixedWidget( app.getUI()->getRoot(), 100, 100 );
	second->setDirection( UIDirection::RightToLeft );
	auto* bridge = Node::New();
	bridge->setParent( first );
	auto* child = fixedWidget( bridge, 10, 10 );
	// The parent's cache changes before its descendants are notified. Its callback moves the
	// still-LTR subtree to an already-RTL parent, so equal parent directions alone are
	// insufficient.
	first->on( Event::OnDirectionChange,
			   [bridge, second]( const Event* ) { bridge->setParent( second ); } );
	first->setDirection( UIDirection::RightToLeft );
	EXPECT_TRUE( child->isRTL() );
	EXPECT_TRUE( child->getTextBaseDirection() == TextDirection::RightToLeft );
}

UTEST( UIDirection, cssInheritPreservesNativeInheritance ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
 <vbox id="parent" direction="rtl"><widget id="child"/></vbox>
 )xml" );
	app.getUI()->combineStyleSheet( "#child {direction: inherit;}" );
	app.getUI()->flushDirtyStyleAndLayout();
	auto* parent = findWidget( app, "parent" );
	auto* child = findWidget( app, "child" );
	EXPECT_TRUE( child->isRTL() );
	EXPECT_FALSE( child->hasDirectionOverride() );
	parent->setDirection( UIDirection::LeftToRight );
	EXPECT_TRUE( child->isLTR() );
	parent->setDirection( UIDirection::RightToLeft );
	EXPECT_TRUE( child->isRTL() );
}

UTEST( UILogicalAlignment, zeroValuedPhysicalLayoutGravityReplacesStart ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10 );
	widget->setLayoutHorizontalAlignment( UIHorizontalAlignment::Start );
	widget->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER );
	app.getUI()->setDirection( UIDirection::RightToLeft );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, Font::getHorizontalAlign( widget->getLayoutGravity() ) );
	EXPECT_EQ( (Uint32)UI_VALIGN_CENTER, Font::getVerticalAlign( widget->getLayoutGravity() ) );
	EXPECT_TRUE( widget->getLayoutHorizontalAlignment() == UIHorizontalAlignment::Left );
}

UTEST( UILogicalAlignment, physicalFlagMutatorsReplaceLogicalGravity ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10 );
	widget->setHorizontalAlignment( UIHorizontalAlignment::Start );
	widget->resetFlags( UI_HALIGN_LEFT | UI_VALIGN_CENTER );
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, widget->getHorizontalAlign() );
	EXPECT_TRUE( widget->getHorizontalAlignment() == UIHorizontalAlignment::Left );
	widget->setHorizontalAlignment( UIHorizontalAlignment::Start );
	widget->unsetFlags( UI_HALIGN_RIGHT );
	widget->setDirection( UIDirection::LeftToRight );
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_EQ( (Uint32)UI_HALIGN_LEFT, widget->getHorizontalAlign() );
	EXPECT_TRUE( widget->getHorizontalAlignment() == UIHorizontalAlignment::Left );
}

UTEST( UILogicalBox, statePriorityChangesRecomputeSharedEdges ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10, "priority" );
	app.getUI()->combineStyleSheet( R"css(
 #priority {padding-left:4dp; padding-inline-start:8dp;}
 #priority:selected {padding-left:4dp;}
 )css" );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 8.f, widget->getPadding().Left, 0.01f );
	widget->pushState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 4.f, widget->getPadding().Left, 0.01f );
	widget->popState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 8.f, widget->getPadding().Left, 0.01f );
}

UTEST( UILogicalBox, removedStateDeclarationDoesNotReturnAfterDirectionChange ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10, "transient" );
	app.getUI()->combineStyleSheet( R"css(
 #transient {padding-left:4dp;}
 #transient:selected {padding-inline-start:10dp;}
 )css" );
	app.getUI()->flushDirtyStyleAndLayout();
	widget->pushState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 10.f, widget->getPadding().Left, 0.01f );
	widget->popState( UIState::StateSelected );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 4.f, widget->getPadding().Left, 0.01f );
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 4.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 0.f, widget->getPadding().Right, 0.01f );
	EXPECT_TRUE( widget->getBoxEdgeProperty( PropertyId::PaddingRight ) == nullptr );
}

UTEST( UILogicalBox, sheetAttachmentOrderWinsOverParsingOrder ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* widget = fixedWidget( app.getUI()->getRoot(), 10, 10, "source" );
	StyleSheetParser logical, physical;
	logical.loadFromString( std::string( "#source {padding-inline-start:9dp;}" ) );
	physical.loadFromString( std::string( "#source {padding-left:6dp;}" ) );
	app.getUI()->combineStyleSheet( physical.getStyleSheet() );
	app.getUI()->combineStyleSheet( logical.getStyleSheet() );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 9.f, widget->getPadding().Left, 0.01f );
	widget->setDirection( UIDirection::RightToLeft );
	EXPECT_NEAR( 6.f, widget->getPadding().Left, 0.01f );
	EXPECT_NEAR( 9.f, widget->getPadding().Right, 0.01f );
}

UTEST( UIHorizontalFlow, rtlFlowHonorsPhysicalRightPadding ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* flow = UIFlowLayout::New();
	flow->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flow->setPixelsSize( 200, 40 );
	flow->setPadding( { 3, 0, 5, 0 } );
	flow->setParent( app.getUI()->getRoot() );
	auto* first = fixedWidget( flow, 20, 10 );
	auto* second = fixedWidget( flow, 30, 10 );
	flow->setDirection( UIDirection::RightToLeft );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 175.f, first->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 145.f, second->getPixelsPosition().x, 0.1f );
	flow->setDirection( UIDirection::LeftToRight );
	app.getUI()->flushDirtyStyleAndLayout();
	EXPECT_NEAR( 3.f, first->getPixelsPosition().x, 0.1f );
	EXPECT_NEAR( 23.f, second->getPixelsPosition().x, 0.1f );
}

UTEST( UIHorizontalFlow, wrapContentFlowAlignsInsideFinalWidth ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* flow = UIFlowLayout::New();
	flow->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::Fixed );
	flow->setPixelsSize( 200, 40 );
	flow->setPadding( { 3, 0, 5, 0 } );
	flow->setParent( app.getUI()->getRoot() );
	auto* first = fixedWidget( flow, 20, 10 );
	auto* second = fixedWidget( flow, 30, 10 );
	for ( auto direction :
		  { UIDirection::RightToLeft, UIDirection::LeftToRight, UIDirection::RightToLeft } ) {
		flow->setDirection( direction );
		app.getUI()->flushDirtyStyleAndLayout();
		EXPECT_NEAR( 58.f, flow->getPixelsSize().getWidth(), 0.1f );
		EXPECT_NEAR( direction == UIDirection::RightToLeft ? 33.f : 3.f,
					 first->getPixelsPosition().x, 0.1f );
		EXPECT_NEAR( direction == UIDirection::RightToLeft ? 3.f : 23.f,
					 second->getPixelsPosition().x, 0.1f );
	}
}

UTEST( UIRichTextDirection, numericStyleSpansKeepTheirLogicalOrder ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	RichText text;
	FontStyleConfig style;
	style.Font = app.getUI()->getUIThemeManager()->getDefaultFont();
	style.CharacterSize = 12;
	text.setFontStyleConfig( style );
	text.addSpan( String( "12" ), style );
	text.addSpan( String( "34" ), style );
	text.setMaxWidth( 200 );
	text.setAlign( TEXT_ALIGN_RIGHT );
	text.setBaseDirection( TextDirection::RightToLeft );
	text.updateLayout();
	ASSERT_EQ( (size_t)1, text.getLines().size() );
	ASSERT_EQ( (size_t)2, text.getLines()[0].spans.size() );
	const auto& first = text.getLines()[0].spans[0];
	const auto& second = text.getLines()[0].spans[1];
	EXPECT_LT( first.position.x, second.position.x );
	EXPECT_NEAR( 200.f, second.position.x + second.size.getWidth(), 0.01f );
}

UTEST( UIRTLControls, popupInheritsOwnerDirectionAfterReparenting ) {
	UIApplication app( rtlWindowSettings(), rtlTestSettings() );
	auto* bar = UIMenuBar::New();
	bar->setParent( app.getUI()->getRoot() );
	bar->setDirection( UIDirection::RightToLeft );
	auto* menu = UIPopUpMenu::New();
	auto* item = menu->add( "Item" );
	bar->addMenuButton( "File", menu );
	EXPECT_TRUE( menu->isRTL() );
	bar->showMenu( 0 );
	EXPECT_TRUE( app.getUI()->getRoot()->isLTR() );
	EXPECT_TRUE( menu->isRTL() );
	EXPECT_TRUE( item->isRTL() );
	EXPECT_FALSE( menu->hasDirectionOverride() );
	bar->setDirection( UIDirection::LeftToRight );
	EXPECT_TRUE( menu->isLTR() );
	EXPECT_TRUE( item->isLTR() );
	menu->setDirection( UIDirection::LeftToRight );
	bar->setDirection( UIDirection::RightToLeft );
	EXPECT_TRUE( menu->isLTR() );
	menu->clearDirectionOverride();
	EXPECT_TRUE( menu->isRTL() );
}
