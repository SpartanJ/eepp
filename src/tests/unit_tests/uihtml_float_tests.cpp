#include "utest.h"
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::Window;
using namespace EE::Graphics;

static void init_float_test() {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Float Layout Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
}

UTEST( UIHTMLFloat, structure_FloatAndClearEnums ) {
	EXPECT_TRUE( CSSFloatHelper::toString( CSSFloat::None ) == "none" );
	EXPECT_TRUE( CSSFloatHelper::toString( CSSFloat::Left ) == "left" );
	EXPECT_TRUE( CSSFloatHelper::toString( CSSFloat::Right ) == "right" );

	EXPECT_EQ( (int)CSSFloat::None, (int)CSSFloatHelper::fromString( "none" ) );
	EXPECT_EQ( (int)CSSFloat::Left, (int)CSSFloatHelper::fromString( "left" ) );
	EXPECT_EQ( (int)CSSFloat::Right, (int)CSSFloatHelper::fromString( "right" ) );
	EXPECT_EQ( (int)CSSFloat::None, (int)CSSFloatHelper::fromString( "invalid" ) );

	EXPECT_TRUE( CSSClearHelper::toString( CSSClear::None ) == "none" );
	EXPECT_TRUE( CSSClearHelper::toString( CSSClear::Left ) == "left" );
	EXPECT_TRUE( CSSClearHelper::toString( CSSClear::Right ) == "right" );
	EXPECT_TRUE( CSSClearHelper::toString( CSSClear::Both ) == "both" );

	EXPECT_EQ( (int)CSSClear::None, (int)CSSClearHelper::fromString( "none" ) );
	EXPECT_EQ( (int)CSSClear::Left, (int)CSSClearHelper::fromString( "left" ) );
	EXPECT_EQ( (int)CSSClear::Right, (int)CSSClearHelper::fromString( "right" ) );
	EXPECT_EQ( (int)CSSClear::Both, (int)CSSClearHelper::fromString( "both" ) );
	EXPECT_EQ( (int)CSSClear::None, (int)CSSClearHelper::fromString( "garbage" ) );
}

UTEST( UIHTMLFloat, property_DefaultsAreNone ) {
	UIHTMLWidget* w = UIHTMLWidget::New();
	EXPECT_EQ( CSSFloat::None, w->getCSSFloat() );
	EXPECT_EQ( CSSClear::None, w->getCSSClear() );
	eeDelete( w );
}

UTEST( UIHTMLFloat, property_SetFloatViaApplyProperty ) {
	UIHTMLWidget* w = UIHTMLWidget::New();
	w->applyProperty( StyleSheetProperty( "float", "left" ) );
	EXPECT_EQ( CSSFloat::Left, w->getCSSFloat() );
	w->applyProperty( StyleSheetProperty( "float", "right" ) );
	EXPECT_EQ( CSSFloat::Right, w->getCSSFloat() );
	w->applyProperty( StyleSheetProperty( "float", "none" ) );
	EXPECT_EQ( CSSFloat::None, w->getCSSFloat() );
	eeDelete( w );
}

UTEST( UIHTMLFloat, property_SetClearViaApplyProperty ) {
	UIHTMLWidget* w = UIHTMLWidget::New();
	w->applyProperty( StyleSheetProperty( "clear", "left" ) );
	EXPECT_EQ( CSSClear::Left, w->getCSSClear() );
	w->applyProperty( StyleSheetProperty( "clear", "right" ) );
	EXPECT_EQ( CSSClear::Right, w->getCSSClear() );
	w->applyProperty( StyleSheetProperty( "clear", "both" ) );
	EXPECT_EQ( CSSClear::Both, w->getCSSClear() );
	w->applyProperty( StyleSheetProperty( "clear", "none" ) );
	EXPECT_EQ( CSSClear::None, w->getCSSClear() );
	eeDelete( w );
}

UTEST( UIHTMLFloat, property_GetPropertyString ) {
	UIHTMLWidget* w = UIHTMLWidget::New();
	w->setCSSFloat( CSSFloat::Left );
	w->setCSSClear( CSSClear::Right );
	auto props = w->getPropertiesImplemented();
	bool hasFloat = false, hasClear = false;
	for ( auto& p : props ) {
		if ( p == PropertyId::Float )
			hasFloat = true;
		if ( p == PropertyId::Clear )
			hasClear = true;
	}
	EXPECT_TRUE( hasFloat );
	EXPECT_TRUE( hasClear );
	eeDelete( w );
}

UTEST( UIHTMLFloat, richtext_NoFloatLayout_NoChange ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( container );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setDisplay( CSSDisplay::InlineBlock );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( container );
	child2->setPixelsSize( 150, 30 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setDisplay( CSSDisplay::InlineBlock );

	sceneNode->updateDirtyLayouts();

	Vector2f pos1 = child1->convertToWorldSpace( { 0, 0 } );
	Vector2f pos2 = child2->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( pos2.x, pos1.x + child1->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatLeft_TextWrapsRight ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatChild = UIHTMLWidget::New();
	floatChild->setParent( container );
	floatChild->setPixelsSize( 100, 50 );
	floatChild->setCSSFloat( CSSFloat::Left );
	floatChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* inlineChild = UIHTMLWidget::New();
	inlineChild->setParent( container );
	inlineChild->setPixelsSize( 80, 30 );
	inlineChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	inlineChild->setDisplay( CSSDisplay::InlineBlock );

	sceneNode->updateDirtyLayouts();

	Vector2f fpos = floatChild->convertToWorldSpace( { 0, 0 } );
	Vector2f ipos = inlineChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( fpos.y, ipos.y, 1.f );
	EXPECT_GE( ipos.x, fpos.x + floatChild->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatRight_TextFlowsLeft ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatChild = UIHTMLWidget::New();
	floatChild->setParent( container );
	floatChild->setPixelsSize( 100, 50 );
	floatChild->setCSSFloat( CSSFloat::Right );
	floatChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* inlineChild = UIHTMLWidget::New();
	inlineChild->setParent( container );
	inlineChild->setPixelsSize( 80, 30 );
	inlineChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	inlineChild->setDisplay( CSSDisplay::InlineBlock );

	sceneNode->updateDirtyLayouts();

	Vector2f fpos = floatChild->convertToWorldSpace( { 0, 0 } );
	Vector2f ipos = inlineChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( fpos.y, ipos.y, 1.f );
	Float fRightEdge = fpos.x + floatChild->getPixelsSize().getWidth();
	EXPECT_LT( ipos.x + inlineChild->getPixelsSize().getWidth(), fRightEdge + 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, rightFloatDoesNotDisplaceFollowingNormalBlock ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* side = UIHTMLWidget::New();
	side->setParent( container );
	side->setPixelsSize( 100, 100 );
	side->setCSSFloat( CSSFloat::Right );
	side->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* content = UIHTMLWidget::New();
	content->setParent( container );
	content->setPixelsSize( 0, 30 );
	content->setLayoutPixelsMarginRight( 120 );
	content->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f sidePos = side->convertToWorldSpace( { 0, 0 } );
	Vector2f contentPos = content->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( sidePos.y, contentPos.y, 1.f );
	EXPECT_NEAR( contentPos.x, container->convertToWorldSpace( { 0, 0 } ).x, 1.f );
	EXPECT_NEAR( content->getPixelsSize().getWidth(), 480.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, leftFloatOverflowHiddenBlockFormattingContextSitsBesideFloat ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* midcol = UIHTMLWidget::New();
	midcol->setParent( container );
	midcol->setPixelsSize( 20, 50 );
	midcol->setCSSFloat( CSSFloat::Left );
	midcol->setLayoutPixelsMarginLeft( 5 );
	midcol->setLayoutPixelsMarginRight( 5 );
	midcol->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* entry = UIHTMLWidget::New();
	entry->setParent( container );
	entry->setPixelsSize( 0, 40 );
	entry->setLayoutPixelsMarginLeft( 3 );
	entry->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
	entry->applyProperty( StyleSheetProperty( "overflow", "hidden" ) );

	sceneNode->updateDirtyLayouts();

	Vector2f midcolPos = midcol->convertToWorldSpace( { 0, 0 } );
	Vector2f entryPos = entry->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( midcolPos.y, entryPos.y, 1.f );
	EXPECT_GE( entryPos.x, midcolPos.x + midcol->getPixelsSize().getWidth() +
							   midcol->getLayoutPixelsMargin().Right - 1.f );
	EXPECT_NEAR( entry->getPixelsSize().getWidth(), 567.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, whitespaceBetweenFloatAndBfcDoesNotPushBfcBelowFloat ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="container" style="width:600px">
				<div id="left" style="float:left;width:140px;height:18px"></div>
				<div id="bfc" style="overflow:hidden;height:18px">top row</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* left = sceneNode->find<UIWidget>( "left" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( left != nullptr );
	ASSERT_TRUE( bfc != nullptr );

	Vector2f leftPos = left->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( leftPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, leftPos.x + left->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, htmlButtonCanFloatBeforeBlockFormattingContext ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="container" style="width:600px">
				<button id="button" style="float:left;width:140px;height:18px">action</button>
				<div id="bfc" style="overflow:hidden;height:18px">top row</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* button = sceneNode->find<UIWidget>( "button" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( button != nullptr );
	ASSERT_TRUE( bfc != nullptr );
	ASSERT_TRUE( button->isType( UI_TYPE_HTML_WIDGET ) );
	EXPECT_EQ( button->asType<UIHTMLWidget>()->getCSSFloat(), CSSFloat::Left );

	Vector2f buttonPos = button->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( buttonPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, buttonPos.x + button->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, bfcAfterFloatOnlyLineStaysOnSameRow ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="container" style="width:600px; line-height:18px">
				<button id="button" style="float:left;width:138px;height:18px">action</button>
				<div id="drop" style="float:left;width:117px;height:18px">menu</div>
				<div id="bfc" style="overflow:hidden;height:18px; white-space:nowrap">top row</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* button = sceneNode->find<UIWidget>( "button" );
	auto* drop = sceneNode->find<UIWidget>( "drop" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( button != nullptr );
	ASSERT_TRUE( drop != nullptr );
	ASSERT_TRUE( bfc != nullptr );

	Vector2f buttonPos = button->convertToWorldSpace( { 0, 0 } );
	Vector2f dropPos = drop->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( buttonPos.y, bfcPos.y, 1.f );
	EXPECT_NEAR( dropPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, dropPos.x + drop->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, autoHeightBfcAfterFloatOnlyLineStaysOnSameRow ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="container" style="width:600px; line-height:18px">
				<button id="button" style="float:left;width:138px;height:18px">action</button>
				<div id="drop" style="float:left;width:117px;height:18px">menu</div>
				<div id="bfc" style="overflow:hidden; white-space:nowrap">
					<span>home</span><span> - popular</span><span> - all</span>
				</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* drop = sceneNode->find<UIWidget>( "drop" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( drop != nullptr );
	ASSERT_TRUE( bfc != nullptr );

	Vector2f dropPos = drop->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( dropPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, dropPos.x + drop->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, inlineListBfcAfterFloatOnlyLineStaysOnSameRow ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="container" style="width:600px; line-height:18px; white-space:nowrap">
				<button id="button" style="float:left;width:138px;height:18px">action</button>
				<div id="drop" style="float:left;width:117px;height:18px">menu</div>
				<div id="bfc" style="overflow:hidden">
					<ul id="list" style="display:inline;list-style:none;margin:0;padding:0">
						<li style="display:inline;white-space:nowrap">home</li>
						<li style="display:inline;white-space:nowrap"> - popular</li>
						<li style="display:inline;white-space:nowrap"> - all</li>
					</ul>
				</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* drop = sceneNode->find<UIWidget>( "drop" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( drop != nullptr );
	ASSERT_TRUE( bfc != nullptr );

	Vector2f dropPos = drop->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( dropPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, dropPos.x + drop->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, absoluteInlineListBfcAfterFloatOnlyLineStaysOnSameRow ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="header" style="position:relative;width:600px;height:18px;line-height:18px;white-space:nowrap">
				<div id="clip" style="position:absolute;left:0;right:0">
					<button id="button" style="float:left;width:138px;height:18px">action</button>
					<div id="drop" style="float:left;width:117px;height:18px">menu</div>
					<div id="bfc" style="overflow:hidden">
						<ul id="list" style="display:inline;list-style:none;margin:0;padding:0">
							<li style="display:inline;white-space:nowrap">home</li>
							<li style="display:inline;white-space:nowrap"> - popular</li>
							<li style="display:inline;white-space:nowrap"> - all</li>
						</ul>
					</div>
				</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* drop = sceneNode->find<UIWidget>( "drop" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( drop != nullptr );
	ASSERT_TRUE( bfc != nullptr );

	Vector2f dropPos = drop->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( dropPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, dropPos.x + drop->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, whitespaceBetweenInlineFloatsDoesNotPushFollowingBfc ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="header" style="position:relative;width:600px;height:18px;line-height:18px;white-space:nowrap">
				<div id="clip" style="position:absolute;left:0;right:0">
					<button id="button" style="display:inline-block;float:left;width:138px;height:18px">action</button>
					<div id="drop" style="display:inline;float:left;width:117px;height:18px">menu</div>
					<div id="bfc" style="overflow:hidden">
						<ul id="list" style="display:inline;list-style:none;margin:0;padding:0">
							<li style="display:inline;white-space:nowrap">home</li>
							<li style="display:inline;white-space:nowrap"> - popular</li>
							<li style="display:inline;white-space:nowrap"> - all</li>
						</ul>
					</div>
				</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* drop = sceneNode->find<UIWidget>( "drop" );
	auto* bfc = sceneNode->find<UIWidget>( "bfc" );
	ASSERT_TRUE( drop != nullptr );
	ASSERT_TRUE( bfc != nullptr );

	Vector2f dropPos = drop->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( dropPos.y, bfcPos.y, 1.f );
	EXPECT_GE( bfcPos.x, dropPos.x + drop->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, rightFloatedInlineSpansAlignAtContainerRightAfterBlock ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="editor" style="width:500px">
				<textarea id="textarea" style="display:block;width:500px;height:100px"></textarea>
				<div id="bottom" style="overflow:hidden;width:100%">
					<span id="help" style="float:right;margin-left:10px">formatting help</span>
					<a id="policy" style="float:right;margin-left:10px">content policy</a>
				</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* editor = sceneNode->find<UIWidget>( "editor" );
	auto* bottom = sceneNode->find<UIWidget>( "bottom" );
	auto* help = sceneNode->find<UIWidget>( "help" );
	auto* policy = sceneNode->find<UIWidget>( "policy" );
	ASSERT_TRUE( editor != nullptr );
	ASSERT_TRUE( bottom != nullptr );
	ASSERT_TRUE( help != nullptr );
	ASSERT_TRUE( policy != nullptr );
	ASSERT_TRUE( help->isType( UI_TYPE_HTML_WIDGET ) );
	ASSERT_TRUE( policy->isType( UI_TYPE_HTML_WIDGET ) );
	EXPECT_EQ( help->asType<UIHTMLWidget>()->getCSSFloat(), CSSFloat::Right );
	EXPECT_EQ( policy->asType<UIHTMLWidget>()->getCSSFloat(), CSSFloat::Right );

	Vector2f bottomPos = bottom->convertToWorldSpace( { 0, 0 } );
	Vector2f helpPos = help->convertToWorldSpace( { 0, 0 } );
	Vector2f policyPos = policy->convertToWorldSpace( { 0, 0 } );
	Float bottomRight = bottomPos.x + bottom->getPixelsSize().getWidth();
	Float helpRight = helpPos.x + help->getPixelsSize().getWidth();
	Float policyRight = policyPos.x + policy->getPixelsSize().getWidth();

	EXPECT_NEAR( helpRight, bottomRight, 1.f );
	EXPECT_LE( policyRight, helpPos.x - help->getLayoutPixelsMargin().Left + 1.f );
	EXPECT_NEAR( helpPos.y, policyPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatedListItemsShrinkToFitBlockAnchors ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="access" style="background:#000;display:block;float:left;width:940px">
				<div id="menu" class="menu" style="font-size:13px;margin-left:12px;width:928px">
					<ul id="top" style="list-style:none;margin:0;padding:0">
						<li id="home" style="float:left;position:relative">
							<a id="home-a" style="display:block;line-height:38px;padding:0 10px">Home</a>
						</li>
						<li id="about" style="float:left;position:relative">
							<a id="about-a" style="display:block;line-height:38px;padding:0 10px">About</a>
						</li>
						<li id="os2" style="float:left;position:relative">
							<a id="os2-a" style="display:block;line-height:38px;padding:0 10px">OS/2 History</a>
							<ul id="os2-sub" style="display:none;position:absolute;top:38px;left:0;float:left;width:180px">
								<li style="min-width:180px"><a style="display:block;line-height:1em;padding:10px;width:160px">OS/2 Beginnings</a></li>
							</ul>
						</li>
					</ul>
				</div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* menu = sceneNode->find<UIWidget>( "menu" );
	auto* home = sceneNode->find<UIWidget>( "home" );
	auto* about = sceneNode->find<UIWidget>( "about" );
	auto* os2 = sceneNode->find<UIWidget>( "os2" );
	auto* os2Anchor = sceneNode->find<UIWidget>( "os2-a" );
	ASSERT_TRUE( menu != nullptr );
	ASSERT_TRUE( home != nullptr );
	ASSERT_TRUE( about != nullptr );
	ASSERT_TRUE( os2 != nullptr );
	ASSERT_TRUE( os2Anchor != nullptr );

	EXPECT_EQ( os2->asType<UIHTMLWidget>()->getCSSFloat(), CSSFloat::Left );
	EXPECT_LT( os2->getPixelsSize().getWidth(), menu->getPixelsSize().getWidth() * 0.5f );
	EXPECT_NEAR( os2->getPixelsSize().getWidth(), os2Anchor->getPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( os2->getPixelsSize().getHeight(), 38.f, 1.f );
	EXPECT_NEAR( os2Anchor->getPixelsSize().getHeight(), 38.f, 1.f );
	auto* os2AnchorRichText = os2Anchor->asType<UIRichText>()->getRichTextPtr();
	ASSERT_TRUE( os2AnchorRichText != nullptr );
	ASSERT_EQ( os2AnchorRichText->getLines().size(), (size_t)1 );
	ASSERT_EQ( os2AnchorRichText->getLines().front().spans.size(), (size_t)1 );
	const auto& os2TextSpan = os2AnchorRichText->getLines().front().spans.front();
	EXPECT_GT( os2TextSpan.position.y, 1.f );
	EXPECT_LT( os2TextSpan.position.y + os2TextSpan.size.getHeight(),
			   os2Anchor->getPixelsSize().getHeight() - 1.f );
	EXPECT_NEAR( home->getPixelsPosition().y, os2->getPixelsPosition().y, 1.f );
	EXPECT_NEAR( about->getPixelsPosition().y, os2->getPixelsPosition().y, 1.f );
	EXPECT_GE( about->getPixelsPosition().x,
			   home->getPixelsPosition().x + home->getPixelsSize().getWidth() - 1.f );
	EXPECT_GE( os2->getPixelsPosition().x,
			   about->getPixelsPosition().x + about->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, autoHorizontalMarginsCenterBlockInsideFloat ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<body style="margin:0">
			<div id="midcol" style="float:left;width:19px;height:50px;overflow:hidden">
				<div id="arrow" style="display:block;width:15px;height:14px;margin-left:auto;margin-right:auto"></div>
			</div>
		</body>
	)html" ) );
	sceneNode->updateDirtyLayouts();

	auto* midcol = sceneNode->find<UIWidget>( "midcol" );
	auto* arrow = sceneNode->find<UIWidget>( "arrow" );
	ASSERT_TRUE( midcol != nullptr );
	ASSERT_TRUE( arrow != nullptr );

	Vector2f midcolPos = midcol->convertToWorldSpace( { 0, 0 } );
	Vector2f arrowPos = arrow->convertToWorldSpace( { 0, 0 } );
	Float midcolCenter = midcolPos.x + midcol->getPixelsSize().getWidth() / 2.f;
	Float arrowCenter = arrowPos.x + arrow->getPixelsSize().getWidth() / 2.f;

	EXPECT_NEAR( midcolCenter, arrowCenter, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, rightFloatConstrainsTextInsideFollowingNormalBlock ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* side = UIHTMLWidget::New();
	side->setParent( container );
	side->setPixelsSize( 100, 100 );
	side->setCSSFloat( CSSFloat::Right );
	side->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIRichText* content = UIRichText::NewDiv();
	content->setParent( container );
	content->setPixelsSize( 0, 120 );
	content->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );

	UITextNode* text = UITextNode::New();
	text->setParent( content );
	text->setText( "one two three four five six seven eight nine ten eleven twelve thirteen "
				   "fourteen fifteen sixteen seventeen eighteen nineteen twenty" );

	sceneNode->updateDirtyLayouts();
	sceneNode->updateDirtyLayouts();

	Vector2f sidePos = side->convertToWorldSpace( { 0, 0 } );
	Vector2f contentPos = content->convertToWorldSpace( { 0, 0 } );
	const auto& lines = content->getRichTextPtr()->getLines();

	ASSERT_FALSE( lines.empty() );
	EXPECT_NEAR( sidePos.y, contentPos.y, 1.f );
	EXPECT_NEAR( contentPos.x, container->convertToWorldSpace( { 0, 0 } ).x, 1.f );
	EXPECT_NEAR( content->getPixelsSize().getWidth(), 600.f, 1.f );
	EXPECT_LE( lines.front().width, 500.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, rightFloatConstrainsNestedBlockFormattingContext ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* side = UIHTMLWidget::New();
	side->setParent( container );
	side->setPixelsSize( 100, 100 );
	side->setCSSFloat( CSSFloat::Right );
	side->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIRichText* content = UIRichText::NewDiv();
	content->setParent( container );
	content->setPixelsSize( 0, 120 );
	content->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );

	UIRichText* normalBlock = UIRichText::NewDiv();
	normalBlock->setParent( content );
	normalBlock->setPixelsSize( 0, 80 );
	normalBlock->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );

	UIRichText* entry = UIRichText::NewDiv();
	entry->setParent( normalBlock );
	entry->setPixelsSize( 0, 40 );
	entry->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
	entry->applyProperty( StyleSheetProperty( "overflow", "hidden" ) );

	UITextNode* entryText = UITextNode::New();
	entryText->setParent( entry );
	entryText->setText( "entry text" );

	sceneNode->updateDirtyLayouts();
	sceneNode->updateDirtyLayouts();
	sceneNode->updateDirtyLayouts();

	Vector2f sidePos = side->convertToWorldSpace( { 0, 0 } );
	Vector2f entryPos = entry->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( sidePos.y, entryPos.y, 1.f );
	EXPECT_NEAR( entryPos.x, container->convertToWorldSpace( { 0, 0 } ).x, 1.f );
	EXPECT_NEAR( entry->getPixelsSize().getWidth(), 500.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, twoFloatsLeft_StackHorizontally ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* float1 = UIHTMLWidget::New();
	float1->setParent( container );
	float1->setPixelsSize( 100, 50 );
	float1->setCSSFloat( CSSFloat::Left );
	float1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* float2 = UIHTMLWidget::New();
	float2->setParent( container );
	float2->setPixelsSize( 120, 40 );
	float2->setCSSFloat( CSSFloat::Left );
	float2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f f1pos = float1->convertToWorldSpace( { 0, 0 } );
	Vector2f f2pos = float2->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( f1pos.y, f2pos.y, 1.f );
	EXPECT_NEAR( f2pos.x, f1pos.x + 100.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, twoFloatsRight_StackHorizontally ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* float1 = UIHTMLWidget::New();
	float1->setParent( container );
	float1->setPixelsSize( 100, 50 );
	float1->setCSSFloat( CSSFloat::Right );
	float1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* float2 = UIHTMLWidget::New();
	float2->setParent( container );
	float2->setPixelsSize( 80, 40 );
	float2->setCSSFloat( CSSFloat::Right );
	float2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f f1pos = float1->convertToWorldSpace( { 0, 0 } );
	Vector2f f2pos = float2->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( f1pos.y, f2pos.y, 1.f );
	EXPECT_GT( f1pos.x, f2pos.x );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, clearBoth_JumpsBelowAllFloats ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( container );
	floatLeft->setPixelsSize( 100, 80 );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* floatRight = UIHTMLWidget::New();
	floatRight->setParent( container );
	floatRight->setPixelsSize( 90, 60 );
	floatRight->setCSSFloat( CSSFloat::Right );
	floatRight->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* clearChild = UIHTMLWidget::New();
	clearChild->setParent( container );
	clearChild->setPixelsSize( 200, 30 );
	clearChild->setCSSClear( CSSClear::Both );
	clearChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f fLeftPos = floatLeft->convertToWorldSpace( { 0, 0 } );
	Vector2f fRightPos = floatRight->convertToWorldSpace( { 0, 0 } );
	Vector2f clearPos = clearChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( clearPos.y, fLeftPos.y + floatLeft->getPixelsSize().getHeight() - 1.f );
	EXPECT_GE( clearPos.y, fRightPos.y + floatRight->getPixelsSize().getHeight() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, clearLeft_OnlyJumpsPastLeftFloats ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( container );
	floatLeft->setPixelsSize( 100, 120 );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* inlineChild = UIHTMLWidget::New();
	inlineChild->setParent( container );
	inlineChild->setPixelsSize( 50, 20 );
	inlineChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	inlineChild->setDisplay( CSSDisplay::InlineBlock );

	UIHTMLWidget* clearLeftChild = UIHTMLWidget::New();
	clearLeftChild->setParent( container );
	clearLeftChild->setPixelsSize( 200, 30 );
	clearLeftChild->setCSSClear( CSSClear::Left );
	clearLeftChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f floatPos = floatLeft->convertToWorldSpace( { 0, 0 } );
	Vector2f clearPos = clearLeftChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( clearPos.y, floatPos.y + floatLeft->getPixelsSize().getHeight() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, clearRight_RespectsRightFloats ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatRight = UIHTMLWidget::New();
	floatRight->setParent( container );
	floatRight->setPixelsSize( 100, 100 );
	floatRight->setCSSFloat( CSSFloat::Right );
	floatRight->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* clearRightChild = UIHTMLWidget::New();
	clearRightChild->setParent( container );
	clearRightChild->setPixelsSize( 200, 30 );
	clearRightChild->setCSSClear( CSSClear::Right );
	clearRightChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f fpos = floatRight->convertToWorldSpace( { 0, 0 } );
	Vector2f clearPos = clearRightChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( clearPos.y, fpos.y + floatRight->getPixelsSize().getHeight() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, clearInsideBlockFormattingContextIgnoresExternalFloat ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* leftFloat = UIHTMLWidget::New();
	leftFloat->setParent( container );
	leftFloat->setPixelsSize( 100, 80 );
	leftFloat->setCSSFloat( CSSFloat::Left );
	leftFloat->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIRichText* bfc = UIRichText::New();
	bfc->setParent( container );
	bfc->setDisplay( CSSDisplay::Block );
	bfc->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	bfc->applyProperty( StyleSheetProperty( "overflow", "hidden" ) );

	UIHTMLWidget* beforeClear = UIHTMLWidget::New();
	beforeClear->setParent( bfc );
	beforeClear->setPixelsSize( 120, 20 );
	beforeClear->setDisplay( CSSDisplay::Block );
	beforeClear->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* clearLeft = UIHTMLWidget::New();
	clearLeft->setParent( bfc );
	clearLeft->setPixelsSize( 120, 20 );
	clearLeft->setDisplay( CSSDisplay::Block );
	clearLeft->setCSSClear( CSSClear::Left );
	clearLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f floatPos = leftFloat->convertToWorldSpace( { 0, 0 } );
	Vector2f bfcPos = bfc->convertToWorldSpace( { 0, 0 } );
	Vector2f clearPos = clearLeft->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( bfcPos.x, floatPos.x + leftFloat->getPixelsSize().getWidth() - 1.f );
	EXPECT_LT( clearPos.y, floatPos.y + leftFloat->getPixelsSize().getHeight() - 1.f );
	EXPECT_NEAR( clearPos.y, bfcPos.y + beforeClear->getPixelsSize().getHeight(), 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, mixedLeftRight_ContentBetween ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( container );
	floatLeft->setPixelsSize( 100, 50 );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* floatRight = UIHTMLWidget::New();
	floatRight->setParent( container );
	floatRight->setPixelsSize( 80, 50 );
	floatRight->setCSSFloat( CSSFloat::Right );
	floatRight->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* middleChild = UIHTMLWidget::New();
	middleChild->setParent( container );
	middleChild->setPixelsSize( 150, 30 );
	middleChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	middleChild->setDisplay( CSSDisplay::InlineBlock );

	sceneNode->updateDirtyLayouts();

	Vector2f fLeftPos = floatLeft->convertToWorldSpace( { 0, 0 } );
	Vector2f fRightPos = floatRight->convertToWorldSpace( { 0, 0 } );
	Vector2f midPos = middleChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( fLeftPos.y, fRightPos.y, 1.f );
	EXPECT_NEAR( fLeftPos.y, midPos.y, 1.f );

	EXPECT_GE( midPos.x, fLeftPos.x + floatLeft->getPixelsSize().getWidth() - 1.f );
	EXPECT_LE( midPos.x + middleChild->getPixelsSize().getWidth(), fRightPos.x + 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatWrapsContentBelowWhenTooWide ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( container );
	floatLeft->setPixelsSize( 350, 30 );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* wideChild = UIHTMLWidget::New();
	wideChild->setParent( container );
	wideChild->setPixelsSize( 400, 25 );
	wideChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	wideChild->setDisplay( CSSDisplay::InlineBlock );

	sceneNode->updateDirtyLayouts();

	Vector2f widePos = wideChild->convertToWorldSpace( { 0, 0 } );
	Vector2f fpos = floatLeft->convertToWorldSpace( { 0, 0 } );

	EXPECT_GT( widePos.y, fpos.y + 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatLeft_InlineBlockBeside ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( container );
	floatLeft->setPixelsSize( 100, 50 );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* inlineBlock = UIHTMLWidget::New();
	inlineBlock->setParent( container );
	inlineBlock->setPixelsSize( 80, 30 );
	inlineBlock->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	inlineBlock->setDisplay( CSSDisplay::InlineBlock );

	sceneNode->updateDirtyLayouts();

	Vector2f fpos = floatLeft->convertToWorldSpace( { 0, 0 } );
	Vector2f ipos = inlineBlock->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( fpos.y, ipos.y, 1.f );
	EXPECT_GE( ipos.x, fpos.x + floatLeft->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatLeft_LargeFloat_PushesContentDown ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( container );
	floatLeft->setPixelsSize( 200, 120 );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* afterFloat = UIHTMLWidget::New();
	afterFloat->setParent( container );
	afterFloat->setPixelsSize( 200, 30 );
	afterFloat->setCSSClear( CSSClear::Both );
	afterFloat->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f fpos = floatLeft->convertToWorldSpace( { 0, 0 } );
	Vector2f afterPos = afterFloat->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( afterPos.y, fpos.y + floatLeft->getPixelsSize().getHeight() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatLeftNonHTMLwidget_NoCrash ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsSize( 600, 400 );
	container->setPixelsPosition( 10, 10 );
	container->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIWidget* plainWidget = UIWidget::New();
	plainWidget->setParent( container );
	plainWidget->setPixelsSize( 100, 50 );
	plainWidget->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIWidget* plainWidget2 = UIWidget::New();
	plainWidget2->setParent( container );
	plainWidget2->setPixelsSize( 80, 30 );
	plainWidget2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	Vector2f pos1 = plainWidget->convertToWorldSpace( { 0, 0 } );
	Vector2f pos2 = plainWidget2->convertToWorldSpace( { 0, 0 } );

	EXPECT_GE( pos2.x, pos1.x + plainWidget->getPixelsSize().getWidth() - 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatOnlyWrapContentParentIncludesPadding ) {
	init_float_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* container = UIRichText::New();
	container->setParent( sceneNode->getRoot() );
	container->setPixelsPosition( 10, 10 );
	container->setPaddingPixels( { 10, 10, 10, 10 } );
	container->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );

	UIHTMLWidget* floatChild = UIHTMLWidget::New();
	floatChild->setParent( container );
	floatChild->setPixelsSize( 10, 10 );
	floatChild->setCSSFloat( CSSFloat::Left );
	floatChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	EXPECT_NEAR( container->getPixelsSize().getWidth(), 30.f, 1.f );
	EXPECT_NEAR( container->getPixelsSize().getHeight(), 30.f, 1.f );
	EXPECT_NEAR( floatChild->getPixelsPosition().x, 10.f, 1.f );
	EXPECT_NEAR( floatChild->getPixelsPosition().y, 10.f, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLFloat, floatNotAffectedByTextAlignCenter ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "Float + TextAlign Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string html;
	FileSystem::fileGet( "assets/html/position_absolute_and_float.html", html );
	sceneNode->loadLayoutFromString( UI::Tools::HTMLFormatter::HTMLtoXML( html ) );

	sceneNode->update( Milliseconds( 16 ) );
	sceneNode->updateDirtyLayouts();

	UIWidget* mainWidget = sceneNode->getRoot()->find<UIWidget>( "main" );
	ASSERT_TRUE( mainWidget != nullptr );

	// The "main" div has two children with class "box"
	// Each "box" has float:left, clear:both, text-align:center
	// Inside the first box: .titlebox (float:left) and .login_inbox (float:left)
	Node* child = mainWidget->getFirstChild();
	UIWidget* firstBox = nullptr;
	while ( child ) {
		if ( child->isWidget() ) {
			UIWidget* w = child->asType<UIWidget>();
			if ( w->isType( UI_TYPE_HTML_WIDGET ) &&
				 w->asType<UIHTMLWidget>()->getCSSFloat() == CSSFloat::Left ) {
				firstBox = w;
				break;
			}
		}
		child = child->getNextNode();
	}
	ASSERT_TRUE( firstBox != nullptr );

	// The box's children (float:left) should not be shifted by text-align:center
	Vector2f boxOrigin = firstBox->convertToWorldSpace( { 0, 0 } );

	Node* boxChild = firstBox->getFirstChild();
	while ( boxChild ) {
		if ( boxChild->isWidget() ) {
			UIWidget* bc = boxChild->asType<UIWidget>();
			if ( bc->isType( UI_TYPE_HTML_WIDGET ) &&
				 bc->asType<UIHTMLWidget>()->getCSSFloat() == CSSFloat::Left ) {
				Vector2f bcWorld = bc->convertToWorldSpace( { 0, 0 } );
				// Float children should be at the left edge of the box (not shifted to center)
				EXPECT_NEAR( bcWorld.x, boxOrigin.x, 1.f );
			}
		}
		boxChild = boxChild->getNextNode();
	}

	Engine::destroySingleton();
}
