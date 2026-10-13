#include "../../eepp/ui/accessibility/accessibilitybackend.hpp"
#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uihtmlimage.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

namespace {

UIApplication htmlApp() {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	return UIApplication( WindowSettings( 640, 480, "HTML accessibility tests",
										  WindowStyle::Default | WindowStyle::Hidden,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  settings );
}

void collect( AccessibilityManager* manager, AccessibilityNodeRef ref,
			  std::vector<AccessibilityNodeRef>& nodes ) {
	nodes.push_back( ref );
	const auto count = manager->getChildCount( ref );
	for ( size_t i = 0; i < count; ++i )
		collect( manager, manager->getChild( ref, i ), nodes );
}

bool notified( AccessibilityManager* manager, AccessibilityNodeRef ref, AccessibilityEvent type ) {
	for ( const auto& event : manager->getPendingEvents() ) {
		if ( event.ref == ref && event.type == type )
			return true;
	}
	return false;
}

} // namespace

UTEST( AccessibilityHTML, DocumentRolesKeepFormattingTransparentAndControlsReachable ) {
	auto app = htmlApp();
	auto* scene = app.getUI();
	scene->loadLayoutFromString( R"xml(<richtext id="doc">
		<h2 id="heading">Heading <b>bold</b> <a id="heading-link" href="relative.html">link</a><input id="protected" type="password" value="secret" /></h2>
		<p id="paragraph">Before <span id="format"><b id="bold">bold text</b></span> after</p>
		<ul id="list"><li id="item">First <a id="item-link" href="item.html">item link</a><ul id="nested"><li>Nested item</li></ul></li><li>Second</li></ul>
		<p aria-hidden="true">Hidden paragraph</p>
	</richtext>)xml" );
	scene->update( Time::Zero );
	auto* manager = scene->getAccessibilityManager();
	auto* heading = scene->find<UIRichText>( "heading" );
	auto* link = scene->find<UIAnchorSpan>( "heading-link" );
	auto* list = scene->find<UIRichText>( "list" );
	auto* item = scene->find<UIRichText>( "item" );
	ASSERT_TRUE( heading && link && list && item );
	const auto headingRef = manager->getNodeRef( heading );
	const auto linkRef = manager->getNodeRef( link );
	const auto listRef = manager->getNodeRef( list );
	const auto itemRef = manager->getNodeRef( item );
	EXPECT_EQ( manager->getNodeRole( headingRef ), AccessibilityRole::Heading );
	EXPECT_EQ( manager->getNodeInfo( headingRef ).headingLevel, 2 );
	EXPECT_TRUE( manager->getNodeInfo( headingRef ).name == "Heading bold link" );
	EXPECT_EQ( manager->getNodeRole( linkRef ), AccessibilityRole::Link );
	EXPECT_TRUE( manager->getNodeInfo( linkRef ).name == "link" );
	EXPECT_TRUE( manager->getParent( linkRef ) == headingRef );
	EXPECT_EQ( manager->getNodeRole( listRef ), AccessibilityRole::List );
	EXPECT_EQ( manager->getNodeRole( itemRef ), AccessibilityRole::ListItem );
	EXPECT_TRUE( manager->getParent( itemRef ) == listRef );
	EXPECT_TRUE( manager->getParent( manager->getNodeRef( scene->find<UIRichText>( "nested" ) ) ) ==
				 itemRef );
	EXPECT_FALSE(
		hasState( manager->getNodeState( listRef ), AccessibilityState::SelectionContainer ) );
	EXPECT_FALSE( scene->find<UIRichText>( "paragraph" )->isAccessibilityElement() );
	EXPECT_FALSE( scene->find<UITextSpan>( "format" )->isAccessibilityElement() );
	EXPECT_FALSE( AccessibilityWidgetResolver::isLeafRole( AccessibilityRole::Heading ) );
	EXPECT_FALSE( AccessibilityWidgetResolver::isLeafRole( AccessibilityRole::Link ) );

	std::vector<AccessibilityNodeRef> nodes;
	collect( manager, manager->getRoot(), nodes );
	size_t boldOccurrences = 0;
	for ( size_t i = 0; i < nodes.size(); ++i ) {
		for ( size_t j = 0; j < i; ++j )
			EXPECT_FALSE( nodes[i] == nodes[j] );
		const auto info = manager->getNodeInfo( nodes[i] );
		EXPECT_FALSE( info.name == "Hidden paragraph" );
		if ( info.role == AccessibilityRole::Text && info.name == "bold text" )
			++boldOccurrences;
	}
	EXPECT_EQ( boldOccurrences, 1u );
	auto* password = scene->find<UIHTMLInput>( "protected" )->getChildWidget();
	const auto passwordRef = manager->getNodeRef( password );
	EXPECT_TRUE( manager->getParent( passwordRef ) == headingRef );
	EXPECT_TRUE( hasState( manager->getNodeState( passwordRef ), AccessibilityState::Protected ) );
	EXPECT_TRUE( manager->getNodeInfo( passwordRef ).value.empty() );
	auto* modelList = UIListView::New();
	EXPECT_TRUE(
		hasState( modelList->getAccessibilityState(), AccessibilityState::SelectionContainer ) );
}

UTEST( AccessibilityHTML, LinksActivateThroughNavigationWithoutOpeningOnFocus ) {
	auto app = htmlApp();
	auto* scene = app.getUI();
	scene->loadLayoutFromString(
		R"xml(<richtext><a id="link" href="relative.html">Read <strong id="fragment">more</strong><input id="embedded" type="text" /></a><a id="plain">Plain anchor</a></richtext>)xml" );
	auto* link = scene->find<UIAnchorSpan>( "link" );
	ASSERT_TRUE( link != nullptr );
	auto* manager = scene->getAccessibilityManager();
	const auto ref = manager->getNodeRef( link );
	EXPECT_TRUE( manager->getNodeInfo( ref ).name == "Read more" );
	EXPECT_TRUE( hasAction( manager->getNodeInfo( ref ).actions, AccessibilityAction::Press ) );
	EXPECT_FALSE( manager->getNodeInfo( ref ).text.valid );
	int activations = 0;
	const Node* activatedSource = nullptr;
	std::string activatedURI;
	scene->setNavigationInterceptorCb( [&]( const NavigationRequest& request ) {
		++activations;
		activatedSource = request.source;
		activatedURI = request.uri.toString();
		return true;
	} );
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Focus, {} } ) );
	EXPECT_EQ( activations, 0 );
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == ref );
	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	auto* fragment = scene->find<UITextSpan>( "fragment" );
	fragment->setFocus( NodeFocusReason::Click );
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == ref );
	EXPECT_TRUE( hasState( manager->getNodeState( ref ), AccessibilityState::Focused ) );
	EXPECT_TRUE( notified( manager, ref, AccessibilityEvent::FocusChanged ) );
	EXPECT_FALSE(
		notified( manager, manager->getNodeRef( fragment ), AccessibilityEvent::FocusChanged ) );
	auto* embedded = scene->find<UIHTMLInput>( "embedded" )->getChildWidget();
	embedded->setFocus();
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == manager->getNodeRef( embedded ) );
	EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Press, {} } ) );
	EXPECT_EQ( activations, 1 );
	EXPECT_TRUE( activatedSource == link );
	EXPECT_TRUE( activatedURI == "relative.html" );
	link->setEnabled( false );
	EXPECT_FALSE( manager->performAction( ref, { AccessibilityAction::Press, {} } ) );
	EXPECT_EQ( activations, 1 );
	auto* plain = scene->find<UIAnchorSpan>( "plain" );
	EXPECT_EQ( plain->getAccessibilityRole(), AccessibilityRole::Text );
	EXPECT_FALSE( hasAction( plain->getAccessibilityActions(), AccessibilityAction::Press ) );
}

UTEST( AccessibilityHTML, TextAndHrefChangesUpdateQueriedAncestorsAndKeepIdentity ) {
	auto app = htmlApp();
	auto* scene = app.getUI();
	scene->loadLayoutFromString(
		R"xml(<richtext><h1 id="heading"><a id="link" href="first.html"><b id="text">Before</b></a></h1></richtext>)xml" );
	auto* manager = scene->getAccessibilityManager();
	auto* heading = scene->find<UIRichText>( "heading" );
	auto* link = scene->find<UIAnchorSpan>( "link" );
	auto* text = scene->find<UITextSpan>( "text" );
	const auto headingRef = manager->getNodeRef( heading );
	const auto linkRef = manager->getNodeRef( link );
	const auto textRef = manager->getNodeRef( text );
	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	text->setText( "After" );
	EXPECT_TRUE( manager->getNodeInfo( headingRef ).name == "After" );
	EXPECT_TRUE( manager->getNodeInfo( linkRef ).name == "After" );
	EXPECT_TRUE( notified( manager, textRef, AccessibilityEvent::NameChanged ) );
	EXPECT_TRUE( notified( manager, linkRef, AccessibilityEvent::NameChanged ) );
	EXPECT_TRUE( notified( manager, headingRef, AccessibilityEvent::NameChanged ) );
	link->setHref( "" );
	EXPECT_EQ( manager->getNodeRole( linkRef ), AccessibilityRole::None );
	EXPECT_TRUE( notified( manager, linkRef, AccessibilityEvent::RoleChanged ) );
	EXPECT_TRUE( manager->getParent( textRef ) == headingRef );
	link->setHref( "again.html" );
	EXPECT_TRUE( manager->getNodeRef( link ) == linkRef );
	EXPECT_EQ( manager->getNodeRole( linkRef ), AccessibilityRole::Link );
	EXPECT_TRUE( manager->getParent( textRef ) == linkRef );
}

UTEST( AccessibilityHTML, WhitespaceAndNonASCIITextExposureUpdatesTheCachedHierarchy ) {
	auto app = htmlApp();
	auto* scene = app.getUI();
	auto* heading = UIRichText::NewH3();
	auto* text = UITextNode::New();
	text->setParent( heading );
	text->setText( " \n\t" );
	auto* manager = scene->getAccessibilityManager();
	const auto ref = manager->getNodeRef( heading );
	manager->onNativeClientObserved();
	EXPECT_EQ( manager->getChildCount( ref ), 0u );
	// U+10020 has a low byte of ASCII space; whitespace checks must not narrow code points.
	text->setText( String::fromUtf8( std::string( "\xf0\x90\x80\xa0" ) ) );
	EXPECT_EQ( manager->getChildCount( ref ), 1u );
	const auto textRef = manager->getChild( ref, 0 );
	EXPECT_EQ( manager->getNodeRole( textRef ), AccessibilityRole::Text );
	EXPECT_EQ( manager->getNodeInfo( textRef ).name.size(), 1u );
	text->setText( "Replacement" );
	EXPECT_TRUE( manager->getChild( ref, 0 ) == textRef );
	text->setText( "" );
	EXPECT_EQ( manager->getChildCount( ref ), 0u );
}

UTEST( AccessibilityHTML, HeadingLevelsAreScalarAndExtensible ) {
	auto app = htmlApp();
	auto* manager = app.getUI()->getAccessibilityManager();
	for ( int level = 1; level <= 6; ++level ) {
		auto* heading = UIRichText::NewWithTag( "h" + std::to_string( level ) );
		const auto ref = manager->getNodeRef( heading );
		EXPECT_EQ( manager->getNodeRole( ref ), AccessibilityRole::Heading );
		EXPECT_EQ( manager->getNodeHeadingLevel( ref ), level );
		EXPECT_EQ( manager->getNodeInfo( ref ).headingLevel, level );
	}
	class CustomHeading : public UIWidget {
	  public:
		CustomHeading() : UIWidget( "custom-heading" ) {}

		AccessibilityRole getAccessibilityRole() const override {
			return AccessibilityRole::Heading;
		}

		Uint8 getAccessibilityHeadingLevel() const override { return 4; }
	};
	auto* custom = eeNew( CustomHeading, () );
	const auto ref = manager->getNodeRef( custom );
	EXPECT_EQ( manager->getNodeHeadingLevel( ref ), 4 );
	EXPECT_EQ( manager->getNodeInfo( ref ).headingLevel, 4 );
}

UTEST( AccessibilityHTML, OwnTextDoesNotHideInteractiveChildrenAndAltNamesUpdateLinks ) {
	auto app = htmlApp();
	auto* scene = app.getUI();
	auto* span = UITextSpan::New();
	span->setText( "Prefix" );
	auto* input = UIHTMLInput::New();
	input->setParent( span );
	auto* manager = scene->getAccessibilityManager();
	const auto spanRef = manager->getNodeRef( span );
	EXPECT_EQ( manager->getNodeRole( spanRef ), AccessibilityRole::Group );
	EXPECT_EQ( manager->getChildCount( spanRef ), 1u );
	EXPECT_TRUE( manager->getParent( manager->getNodeRef( input->getChildWidget() ) ) == spanRef );
	auto* link = UIAnchorSpan::New();
	link->setHref( "relative.html" );
	auto* image = UIHTMLImage::New();
	image->setParent( link );
	image->setAlt( "Image link" );
	const auto linkRef = manager->getNodeRef( link );
	const auto imageRef = manager->getNodeRef( image );
	EXPECT_TRUE( manager->getNodeInfo( linkRef ).name == "Image link" );
	EXPECT_EQ( manager->getNodeRole( imageRef ), AccessibilityRole::Image );
	manager->onNativeClientObserved();
	manager->clearPendingEvents();
	image->setAlt( "Updated image" );
	EXPECT_TRUE( manager->getNodeInfo( linkRef ).name == "Updated image" );
	EXPECT_TRUE( notified( manager, linkRef, AccessibilityEvent::NameChanged ) );
	image->setAlt( "" );
	EXPECT_EQ( manager->getNodeRole( imageRef ), AccessibilityRole::None );
	EXPECT_TRUE( notified( manager, imageRef, AccessibilityEvent::RoleChanged ) );
	EXPECT_EQ( manager->getChildCount( linkRef ), 0u );
}

UTEST( AccessibilityHTML, DormantConstructionAndEditsDoNotCreateAManager ) {
	auto app = htmlApp();
	auto* scene = app.getUI();
	const auto* constScene = static_cast<const UISceneNode*>( scene );
	ASSERT_TRUE( constScene->getAccessibilityManager() == nullptr );
	scene->loadLayoutFromString(
		R"xml(<richtext><h1>Heading</h1><p>Text <a id="link" href="first.html">Link</a></p><ul><li>Item</li></ul></richtext>)xml" );
	auto* link = scene->find<UIAnchorSpan>( "link" );
	link->setText( "Updated" );
	link->setHref( "second.html" );
	scene->update( Time::Zero );
	EXPECT_TRUE( constScene->getAccessibilityManager() == nullptr );
	EXPECT_FALSE( scene->hasActiveAccessibilityClients() );
}
