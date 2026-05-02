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
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;
using namespace EE::Graphics;

static void init_float_test() {
	Engine::instance()->createWindow(
		WindowSettings( 800, 600, "Float Layout Test", WindowStyle::Default, WindowBackend::Default,
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

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( container );
	child2->setPixelsSize( 150, 30 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

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

	sceneNode->updateDirtyLayouts();

	Vector2f fpos = floatChild->convertToWorldSpace( { 0, 0 } );
	Vector2f ipos = inlineChild->convertToWorldSpace( { 0, 0 } );

	EXPECT_NEAR( fpos.y, ipos.y, 1.f );
	Float fRightEdge = fpos.x + floatChild->getPixelsSize().getWidth();
	EXPECT_LT( ipos.x + inlineChild->getPixelsSize().getWidth(), fRightEdge + 1.f );

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

UTEST( UIHTMLFloat, floatNotAffectedByTextAlignCenter ) {
	Engine::instance()->createWindow(
		WindowSettings( 800, 600, "Float + TextAlign Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
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
