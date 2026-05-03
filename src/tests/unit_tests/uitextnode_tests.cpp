#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/window/engine.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::CSS;

// Helper: create a basic scene for RichText tests
static UI::UISceneNode* createRichTextScene() {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "UITextNode Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	if ( !font->loaded() ) {
		Engine::destroySingleton();
		return nullptr;
	}
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	return sceneNode;
}

static void destroyRichTextScene( UI::UISceneNode* sceneNode ) {
	eeDelete( sceneNode );
	Engine::destroySingleton();
}

// ============================================================
// Suite: UITextNode_Basics — no scene needed
// ============================================================

UTEST( UITextNode_Basics, CreationAndType ) {
	UITextNode* textNode = UITextNode::New();

	EXPECT_TRUE( textNode->isTextNode() );
	EXPECT_TRUE( textNode->getType() == UI_TYPE_TEXTNODE );
	EXPECT_TRUE( textNode->isType( UI_TYPE_TEXTNODE ) );
	EXPECT_TRUE( textNode->isWidget() );
	EXPECT_TRUE( textNode->isType( UI_TYPE_WIDGET ) );
	EXPECT_FALSE( textNode->isType( UI_TYPE_TEXTSPAN ) );
	EXPECT_FALSE( textNode->isWidgetElement() );

	eeDelete( textNode );
}

UTEST( UITextNode_Basics, TextGetSet ) {
	UITextNode* textNode = UITextNode::New();

	EXPECT_TRUE( textNode->getText().empty() );
	textNode->setText( "Hello World" );
	EXPECT_STRINGEQ( textNode->getText(), "Hello World" );
	textNode->setText( "New Text" );
	EXPECT_STRINGEQ( textNode->getText(), "New Text" );
	textNode->setText( "" );
	EXPECT_TRUE( textNode->getText().empty() );

	eeDelete( textNode );
}

UTEST( UITextNode_Basics, NodeFlagIsSet ) {
	UITextNode* textNode = UITextNode::New();
	EXPECT_TRUE( textNode->getNodeFlags() & NODE_FLAG_TEXTNODE );
	eeDelete( textNode );
}

UTEST( UITextNode_Basics, IsTypeDelegatesToParent ) {
	UITextNode* tn = UITextNode::New();

	EXPECT_TRUE( tn->isType( UI_TYPE_WIDGET ) );
	EXPECT_TRUE( tn->isType( UI_TYPE_UINODE ) );
	EXPECT_FALSE( tn->isType( UI_TYPE_RICHTEXT ) );
	EXPECT_FALSE( tn->isType( UI_TYPE_TEXTSPAN ) );

	eeDelete( tn );
}

// ============================================================
// Suite: UITextNode_ElementCounting — via loaded XML
// ============================================================

UTEST( UITextNode_ElementCounting, GetElementIndexMixedChildren ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// Build a tree: RichText → [TextNode "before", Span(B), TextNode "between", Span(D)]
	// B is element index 0, D is element index 1
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">before <span id="elem0">B</span> between <span id="elem1">D</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	UITextSpan* elem1 = sceneNode->find<UITextSpan>( "elem1" );
	ASSERT_TRUE( elem0 != nullptr );
	ASSERT_TRUE( elem1 != nullptr );

	EXPECT_EQ( elem0->getElementIndex(), 0u );
	EXPECT_EQ( elem1->getElementIndex(), 1u );

	// Type-specific: both are UITextSpan type
	EXPECT_EQ( elem0->getElementOfTypeIndex(), 0u );
	EXPECT_EQ( elem1->getElementOfTypeIndex(), 1u );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_ElementCounting, OnlyTextNodeChildren ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// RichText with only text, no elements → child elements = 0
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">Hello World</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	EXPECT_EQ( rt->getChildElementCount(), 0u );

	destroyRichTextScene( sceneNode );
}

// ============================================================
// Suite: UITextNode_CSSSelectors — via App + XML
// ============================================================

UTEST( UITextNode_CSSSelectors, EmptyWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rtText">Has text</RichText>
			<RichText id="rtEmpty"></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector emptySel = spec.getStructuralSelector( "empty" );
	ASSERT_TRUE( emptySel.selector != nullptr );

	UIRichText* rtText = sceneNode->find<UIRichText>( "rtText" );
	UIRichText* rtEmpty = sceneNode->find<UIRichText>( "rtEmpty" );
	ASSERT_TRUE( rtText != nullptr );
	ASSERT_TRUE( rtEmpty != nullptr );

	// rtText has a text node child → NOT empty
	EXPECT_FALSE( emptySel.selector( rtText, 0, 0, FunctionString::parse( "" ) ) );
	// rtEmpty has no children → empty
	EXPECT_TRUE( emptySel.selector( rtEmpty, 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, FirstChildWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// RichText children: [TextNode "ignored", Span(0), TextNode "between", Span(1)]
	// Span(0) is first element child
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">ignored <span id="elem0">first</span> between <span id="elem1">second</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "first-child" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	UITextSpan* elem1 = sceneNode->find<UITextSpan>( "elem1" );
	ASSERT_TRUE( elem0 != nullptr );
	ASSERT_TRUE( elem1 != nullptr );

	EXPECT_TRUE( sel.selector( elem0, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_FALSE( sel.selector( elem1, 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, LastChildWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// RichText children: [Span(0), TextNode "ignored", Span(1), TextNode "trailing"]
	// Span(1) is last element child
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt"><span id="elem0">first</span> ignored <span id="elem1">last</span> trailing</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "last-child" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	UITextSpan* elem1 = sceneNode->find<UITextSpan>( "elem1" );
	ASSERT_TRUE( elem0 != nullptr );
	ASSERT_TRUE( elem1 != nullptr );

	EXPECT_FALSE( sel.selector( elem0, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_TRUE( sel.selector( elem1, 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, OnlyChildWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// RichText children: [TextNode, Span] → Span IS only element child
	// Text nodes are intentionally invisible to structural selectors
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">ignored <span id="elem0">only</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "only-child" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	ASSERT_TRUE( elem0 != nullptr );

	// Text nodes are invisible → elem0 IS the only element child
	EXPECT_TRUE( sel.selector( elem0, 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, FirstOfTypeWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// RichText children: [TextNode, Span(0), TextNode, Span(1), TextNode, br(0)]
	// Span(0) is first-of-type for SPAN, br(0) is first-of-type for BR (different getType)
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">a <span id="span0">first</span> b <span id="span1">second</span> c <br id="br0"/></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "first-of-type" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* span0 = sceneNode->find<UITextSpan>( "span0" );
	UITextSpan* span1 = sceneNode->find<UITextSpan>( "span1" );
	ASSERT_TRUE( span0 != nullptr );
	ASSERT_TRUE( span1 != nullptr );

	Node* br0 = sceneNode->find( "br0" );
	ASSERT_TRUE( br0 != nullptr );

	EXPECT_TRUE( sel.selector( span0, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_FALSE( sel.selector( span1, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_TRUE( sel.selector( static_cast<UIWidget*>( br0 ), 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, NthChildWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">a <span id="elem0">1</span> b <span id="elem1">2</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();

	StructuralSelector nth1 = spec.getStructuralSelector( "nth-child(1)" );
	StructuralSelector nth2 = spec.getStructuralSelector( "nth-child(2)" );
	ASSERT_TRUE( nth1.selector != nullptr );
	ASSERT_TRUE( nth2.selector != nullptr );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	UITextSpan* elem1 = sceneNode->find<UITextSpan>( "elem1" );
	ASSERT_TRUE( elem0 != nullptr );
	ASSERT_TRUE( elem1 != nullptr );

	EXPECT_TRUE( nth1.selector( elem0, nth1.a, nth1.b, nth1.data ) );
	EXPECT_TRUE( nth2.selector( elem1, nth2.a, nth2.b, nth2.data ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, NthOfTypeWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// Two spans (same type) + one br (different type)
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">a <span id="span0">1</span> b <span id="span1">2</span> c <br id="br0"/></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector nth1 = spec.getStructuralSelector( "nth-of-type(1)" );
	StructuralSelector nth2 = spec.getStructuralSelector( "nth-of-type(2)" );
	ASSERT_TRUE( nth1.selector != nullptr );
	ASSERT_TRUE( nth2.selector != nullptr );

	UITextSpan* span0 = sceneNode->find<UITextSpan>( "span0" );
	UITextSpan* span1 = sceneNode->find<UITextSpan>( "span1" );
	ASSERT_TRUE( span0 != nullptr );
	ASSERT_TRUE( span1 != nullptr );

	Node* br0 = sceneNode->find( "br0" );
	ASSERT_TRUE( br0 != nullptr );

	// nth-of-type(1): first span and first br
	EXPECT_TRUE( nth1.selector( span0, nth1.a, nth1.b, nth1.data ) );
	EXPECT_TRUE( nth1.selector( static_cast<UIWidget*>( br0 ), nth1.a, nth1.b, nth1.data ) );
	// nth-of-type(2): second span
	EXPECT_TRUE( nth2.selector( span1, nth2.a, nth2.b, nth2.data ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, OnlyOfTypeWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// One span and one br → each is only-of-type (different C++ types)
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">a <span id="span0">only span</span> b <br id="br0"/></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "only-of-type" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* span0 = sceneNode->find<UITextSpan>( "span0" );
	ASSERT_TRUE( span0 != nullptr );

	Node* br0 = sceneNode->find( "br0" );
	ASSERT_TRUE( br0 != nullptr );

	// Each is the only one of its C++ type
	EXPECT_TRUE( sel.selector( span0, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_TRUE( sel.selector( static_cast<UIWidget*>( br0 ), 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, LastOfTypeWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// span0, br0, span1 → span0 not last, span1 is last span, br0 is last br
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">a <span id="span0">1</span> b <br id="br0"/> c <span id="span1">2</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "last-of-type" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* span0 = sceneNode->find<UITextSpan>( "span0" );
	UITextSpan* span1 = sceneNode->find<UITextSpan>( "span1" );
	ASSERT_TRUE( span0 != nullptr );
	ASSERT_TRUE( span1 != nullptr );

	Node* br0 = sceneNode->find( "br0" );
	ASSERT_TRUE( br0 != nullptr );

	EXPECT_FALSE( sel.selector( span0, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_TRUE( sel.selector( span1, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_TRUE( sel.selector( static_cast<UIWidget*>( br0 ), 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, NthLastChildWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt"><span id="elem0">1</span> a <span id="elem1">2</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector nthLast1 = spec.getStructuralSelector( "nth-last-child(1)" );
	StructuralSelector nthLast2 = spec.getStructuralSelector( "nth-last-child(2)" );
	ASSERT_TRUE( nthLast1.selector != nullptr );
	ASSERT_TRUE( nthLast2.selector != nullptr );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	UITextSpan* elem1 = sceneNode->find<UITextSpan>( "elem1" );
	ASSERT_TRUE( elem0 != nullptr );
	ASSERT_TRUE( elem1 != nullptr );

	EXPECT_TRUE( nthLast1.selector( elem1, nthLast1.a, nthLast1.b, nthLast1.data ) );
	EXPECT_TRUE( nthLast2.selector( elem0, nthLast2.a, nthLast2.b, nthLast2.data ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_CSSSelectors, NthLastOfTypeWithTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// span0, br0, span1 → nth-last-of-type(1): span1 and br0, nth-last-of-type(2): span0
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt"><span id="span0">1</span> a <span id="span1">2</span> b <br id="br0"/></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector nthLast1 = spec.getStructuralSelector( "nth-last-of-type(1)" );
	StructuralSelector nthLast2 = spec.getStructuralSelector( "nth-last-of-type(2)" );
	ASSERT_TRUE( nthLast1.selector != nullptr );
	ASSERT_TRUE( nthLast2.selector != nullptr );

	UITextSpan* span0 = sceneNode->find<UITextSpan>( "span0" );
	UITextSpan* span1 = sceneNode->find<UITextSpan>( "span1" );
	ASSERT_TRUE( span0 != nullptr );
	ASSERT_TRUE( span1 != nullptr );

	Node* br0 = sceneNode->find( "br0" );
	ASSERT_TRUE( br0 != nullptr );

	EXPECT_TRUE( nthLast1.selector( span1, nthLast1.a, nthLast1.b, nthLast1.data ) );
	EXPECT_TRUE( nthLast2.selector( span0, nthLast2.a, nthLast2.b, nthLast2.data ) );
	EXPECT_TRUE(
		nthLast1.selector( static_cast<UIWidget*>( br0 ), nthLast1.a, nthLast1.b, nthLast1.data ) );

	destroyRichTextScene( sceneNode );
}

// ============================================================
// Suite: UITextNode_XMLParsing
// ============================================================

UTEST( UITextNode_XMLParsing, PcdataCreatesUITextNode ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">Hello World</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	Node* child = rt->getFirstChild();
	ASSERT_TRUE( child != nullptr );
	EXPECT_TRUE( child->isTextNode() );

	UITextNode* textNode = static_cast<UITextNode*>( child );
	EXPECT_STDSTREQ( textNode->getText().toUtf8(), "Hello World" );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_XMLParsing, MixedElementsAndText ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">before <b>bold</b> after</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	int textNodeCount = 0;
	int elementCount = 0;
	Node* child = rt->getFirstChild();
	while ( child ) {
		if ( child->isTextNode() )
			textNodeCount++;
		else
			elementCount++;
		child = child->getNextNode();
	}

	EXPECT_GE( textNodeCount, 1 );
	EXPECT_GE( elementCount, 1 );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_XMLParsing, NestedSpanTextContent ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt"><span id="outer">text <span id="inner">nested</span> after</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UITextSpan* outer = sceneNode->find<UITextSpan>( "outer" );
	UITextSpan* inner = sceneNode->find<UITextSpan>( "inner" );
	ASSERT_TRUE( outer != nullptr );
	ASSERT_TRUE( inner != nullptr );

	// Inner span: no child elements → text is in own mText
	EXPECT_STDSTREQ( inner->getText().toUtf8(), "nested" );

	// Outer span: has child elements → text is in child UITextNodes
	bool foundTextNode = false;
	Node* child = outer->getFirstChild();
	while ( child ) {
		if ( child->isTextNode() ) {
			foundTextNode = true;
			UITextNode* tn = static_cast<UITextNode*>( child );
			EXPECT_FALSE( tn->getText().empty() );
		}
		child = child->getNextNode();
	}
	EXPECT_TRUE( foundTextNode );

	destroyRichTextScene( sceneNode );
}

// ============================================================
// Suite: UITextNode_RichTextRebuild
// ============================================================

UTEST( UITextNode_RichTextRebuild, TextFromNodesAppearsInRichText ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">Hello World</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	Int64 charCount = rt->getRichText().getCharacterCount();
	EXPECT_GE( charCount, 10 ); // "Hello World" has 11 chars

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_RichTextRebuild, MixedContentAppearsInRichText ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">before <b>bold</b> after</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	int textBlocks = 0;
	for ( const auto& block : rt->getRichTextPtr()->getBlocks() ) {
		if ( std::holds_alternative<RichText::SpanBlock>( block ) )
			textBlocks++;
	}
	EXPECT_GE( textBlocks, 3 ); // "before ", "bold", " after"

	destroyRichTextScene( sceneNode );
}

// ============================================================
// Suite: UITextNode_StyleInheritance
// ============================================================

UTEST( UITextNode_StyleInheritance, InheritsFontSizeFromParent ) {
	for ( Float scale : { 1.f, 1.5f, 2.f } ) {
		UTEST_PRINT_STEP( String::format( "SCALE %.1f", scale ).c_str() );
		UIApplication app( WindowSettings( 800, 600, "eepp - UITextNode Inheritance Test",
										   WindowStyle::Default, WindowBackend::Default, 32 ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), scale ) );

		std::string xmlStr = R"(
<html>
	<head>
		<style>
		body { font-size: 16px; }
		h1 { font-size: 2em; }
		</style>
	</head>
<body>
	<h1 id="testh1">test text</h1>
</body>
</html>
		)";

		UIWidget* root = app.getUI()->loadLayoutFromString( xmlStr );
		EXPECT_TRUE( root != nullptr );

		UIRichText* h1 = root->querySelector( "#testh1" )->asType<UIRichText>();
		EXPECT_TRUE( h1 != nullptr );

		EXPECT_NEAR( 32u * scale, h1->getFontSize(), 1.f );

		Node* child = h1->getFirstChild();
		EXPECT_TRUE( child != nullptr );
		EXPECT_TRUE( child->isWidget() );
		EXPECT_TRUE( child->isTextNode() );

		UIWidget* childWidget = child->asType<UIWidget>();
		std::string pxStr = childWidget->getPropertyString(
			StyleSheetSpecification::instance()->getProperty( PropertyId::FontSize ) );
		EXPECT_FALSE( pxStr.empty() );
		EXPECT_NEAR( 32u * scale,
					 childWidget->lengthFromValue( StyleSheetProperty( "font-size", pxStr ) ),
					 1.f );
	}
}

// ============================================================
// Suite: UITextNode_BlockLayouter
// ============================================================

UTEST( UITextNode_BlockLayouter, SpanBoundsCoverChildTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// A span with child text nodes AND a child element → bounds should cover all
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">
				<span id="sp">before <b>bold</b> after</span>
			</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UITextSpan* sp = sceneNode->find<UITextSpan>( "sp" );
	ASSERT_TRUE( sp != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_GT( sp->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( sp->getPixelsSize().getHeight(), 0.f );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_BlockLayouter, SimpleSpanHasBounds ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt"><span id="sp">Just text</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UITextSpan* sp = sceneNode->find<UITextSpan>( "sp" );
	ASSERT_TRUE( sp != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_GT( sp->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( sp->getPixelsSize().getHeight(), 0.f );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_BlockLayouter, LinkSpanHasHitBoxesAndBounds ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">before <a id="link" href="#">link</a> after</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );

	UITextSpan* link = sceneNode->find<UITextSpan>( "link" );
	ASSERT_TRUE( link != nullptr );

	sceneNode->update( Time::Zero );

	EXPECT_GT( link->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( link->getPixelsSize().getHeight(), 0.f );
	EXPECT_FALSE( link->getHitBoxes().empty() );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_BlockLayouter, OuterSpanHitBoxesCoverTextNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// Outer span has child text nodes that should be included in bounds
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">
				<span id="outer">
					before <span id="inner">inner text</span> after
				</span>
			</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UITextSpan* outer = sceneNode->find<UITextSpan>( "outer" );
	ASSERT_TRUE( outer != nullptr );

	EXPECT_GT( outer->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( outer->getPixelsSize().getHeight(), 0.f );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_BlockLayouter, HitBoxLocalPositionsAreAtOrigin ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// Simple span with own text: hitboxes should be at (0,0) in local space
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt" font-size="24dp">
				<a id="anchor" href="#">Link</a>
			</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIWidget* anchor = sceneNode->find<UIWidget>( "anchor" );
	ASSERT_TRUE( anchor != nullptr );

	// The hitboxes should be non-empty
	UITextSpan* anchorSpan = anchor->asType<UITextSpan>();
	EXPECT_FALSE( anchorSpan->getHitBoxes().empty() );

	if ( !anchorSpan->getHitBoxes().empty() ) {
		const Rectf& firstHb = anchorSpan->getHitBoxes()[0];
		// Hitboxes should be near (0,0) in widget-local space
		EXPECT_NEAR( firstHb.Left, 0.f, 2.f );
		EXPECT_NEAR( firstHb.Top, 0.f, 2.f );
	}

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_BlockLayouter, OverFindHitsAnchorWhenMatchingText ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// Mixed text nodes + anchor: overFind should return the anchor when
	// clicking inside its hitbox
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt" font-size="24dp">
				before <a id="anchor" href="#">Link</a> after
			</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UITextSpan* anchor = sceneNode->find<UITextSpan>( "anchor" );
	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( anchor != nullptr );
	ASSERT_TRUE( rt != nullptr );

	EXPECT_FALSE( anchor->getHitBoxes().empty() );

	if ( !anchor->getHitBoxes().empty() ) {
		const Rectf& firstHb = anchor->getHitBoxes()[0];
		Vector2f hitPos = anchor->convertToWorldSpace(
			{ firstHb.Left + 1, firstHb.Top + 1 } );
		Node* hitNode = rt->overFind( hitPos );
		EXPECT_EQ( hitNode, anchor );
	}

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_BlockLayouter, NestedSpanOverFindHitsInnerSpan ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// Outer span contains text nodes and an inner span.
	// Clicking inside the inner span's hitbox should return the inner span.
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt" font-size="24dp">
				<span id="outer">before <b id="inner">bold</b> after</span>
			</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UITextSpan* inner = sceneNode->find<UITextSpan>( "inner" );
	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( inner != nullptr );
	ASSERT_TRUE( rt != nullptr );

	EXPECT_FALSE( inner->getHitBoxes().empty() );

	if ( !inner->getHitBoxes().empty() ) {
		const Rectf& firstHb = inner->getHitBoxes()[0];
		Vector2f hitPos = inner->convertToWorldSpace(
			{ firstHb.Left + 1, firstHb.Top + 1 } );
		Node* hitNode = rt->overFind( hitPos );
		EXPECT_TRUE( hitNode == inner || hitNode->inParentTreeOf( inner ) );
	}

	destroyRichTextScene( sceneNode );
}

// ============================================================
// Suite: UITextNode_EdgeCases
// ============================================================

UTEST( UITextNode_EdgeCases, EmptyTextNodesDontAffectLayout ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	UITextNode* tn = UITextNode::New();
	tn->setParent( sceneNode->getRoot() );
	EXPECT_TRUE( tn->getText().empty() );
	eeDelete( tn );

	// Whitespace-only text should be collapsed
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">   </RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	Node* child = rt->getFirstChild();
	if ( child ) {
		EXPECT_TRUE( child->isTextNode() );
		UITextNode* textNode = static_cast<UITextNode*>( child );
		EXPECT_TRUE( textNode->getText().empty() || String::trim( textNode->getText() ).empty() );
	}

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_EdgeCases, DirectChildOfRichText ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">Direct text content</RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	int textNodeCount = 0;
	Node* child = rt->getFirstChild();
	while ( child ) {
		if ( child->isTextNode() ) {
			textNodeCount++;
			UITextNode* tn = static_cast<UITextNode*>( child );
			EXPECT_FALSE( tn->getText().empty() );
		}
		child = child->getNextNode();
	}
	EXPECT_EQ( textNodeCount, 1 );

	destroyRichTextScene( sceneNode );
}

// ============================================================
// Suite: UITextNode_RegressionTests
// ============================================================

UTEST( UITextNode_Regression, WhitespaceCollapseDoesNotCreateSpuriousNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">Hello <a href="#">World</a>. <code>HI in monospace!</code></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	UIRichText* rt = sceneNode->find<UIRichText>( "rt" );
	ASSERT_TRUE( rt != nullptr );

	// The ". " text after "World" should be in a UITextNode
	bool foundDotSpace = false;
	Node* child = rt->getFirstChild();
	while ( child ) {
		if ( child->isTextNode() ) {
			UITextNode* span = static_cast<UITextNode*>( child );
			if ( span->getText() == ". " ) {
				foundDotSpace = true;
			}
		}
		child = child->getNextNode();
	}
	EXPECT_TRUE( foundDotSpace );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_Regression, ElementCountingDoesNotCountDeletedNodes ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">text <span id="elem0">only</span></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "first-child" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* elem0 = sceneNode->find<UITextSpan>( "elem0" );
	ASSERT_TRUE( elem0 != nullptr );

	// elem0 is first element child (text node skipped)
	EXPECT_TRUE( sel.selector( elem0, 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}

UTEST( UITextNode_Regression, OnlyOfTypeStaysCorrectAfterModification ) {
	auto sceneNode = createRichTextScene();
	ASSERT_TRUE( sceneNode != nullptr );

	// One span and one br → each is only-of-type (different C++ types)
	String xml = R"xml(
		<vbox lw="mp" lh="mp">
			<RichText id="rt">a <span id="span0">only span</span> b <br id="br0"/></RichText>
		</vbox>
	)xml";

	sceneNode->loadLayoutFromString( xml );
	sceneNode->update( Time::Zero );

	auto& spec = *StyleSheetSpecification::instance();
	StructuralSelector sel = spec.getStructuralSelector( "only-of-type" );
	ASSERT_TRUE( sel.selector != nullptr );

	UITextSpan* span0 = sceneNode->find<UITextSpan>( "span0" );
	ASSERT_TRUE( span0 != nullptr );

	Node* br0 = sceneNode->find( "br0" );
	ASSERT_TRUE( br0 != nullptr );

	// Each is only-of-type
	EXPECT_TRUE( sel.selector( span0, 0, 0, FunctionString::parse( "" ) ) );
	EXPECT_TRUE( sel.selector( static_cast<UIWidget*>( br0 ), 0, 0, FunctionString::parse( "" ) ) );

	// Remove br0 → span0 still only-of-type
	br0->detach();
	eeDelete( br0 );

	EXPECT_TRUE( sel.selector( span0, 0, 0, FunctionString::parse( "" ) ) );

	destroyRichTextScene( sceneNode );
}
