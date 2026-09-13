#include "utest.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;

class ScrollingTestEditor : public UICodeEditor {
  public:
	ScrollingTestEditor() : UICodeEditor() {}

	using UICodeEditor::onMouseWheel;
};

UTEST( UIScrolling, SceneSmoothScrollingDefaultsCanBeInheritedSnapshottedAndAppliedNow ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - scrolling test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* editor = UICodeEditor::New();
	editor->setPixelsSize( 160, 80 );
	editor->setParent( scene->getRoot() );
	editor->getDocument().textInput( String( 100, '\n' ) );
	scene->flushDirtyStyleAndLayout();

	EXPECT_FALSE( editor->isSmoothScrollEnabled() );
	scene->setSmoothScrollEnabled( true );
	EXPECT_TRUE( scene->isSmoothScrollEnabled() );
	EXPECT_FALSE( editor->isSmoothScrollEnabled() );

	auto* newEditor = UICodeEditor::New();
	newEditor->setParent( scene->getRoot() );
	EXPECT_TRUE( newEditor->isSmoothScrollEnabled() );
	newEditor->setSmoothScrollEnabled( false );
	EXPECT_FALSE( newEditor->isSmoothScrollEnabled() );

	auto* embeddedHost = UIWidget::New();
	embeddedHost->setParent( scene->getRoot() );
	auto* embeddedScene = UISceneNode::New();
	embeddedScene->setParent( embeddedHost );
	EXPECT_TRUE( embeddedScene->isSmoothScrollEnabled() );
	UICodeEditor* embeddedEditor;
	{
		auto context = embeddedScene->makeCurrent();
		embeddedEditor = UICodeEditor::New();
		embeddedEditor->setParent( embeddedScene->getRoot() );
	}
	EXPECT_TRUE( embeddedEditor->isSmoothScrollEnabled() );

	embeddedScene->setSmoothScrollEnabled( false, true );
	EXPECT_FALSE( scene->isSmoothScrollEnabled() );
	EXPECT_FALSE( embeddedScene->isSmoothScrollEnabled() );
	EXPECT_FALSE( editor->isSmoothScrollEnabled() );
	EXPECT_FALSE( newEditor->isSmoothScrollEnabled() );
	EXPECT_FALSE( embeddedEditor->isSmoothScrollEnabled() );

	editor->setSmoothScrollEnabled( true );
	EXPECT_TRUE( editor->isSmoothScrollEnabled() );
	ASSERT_GT( editor->getMaxScroll().y, 10.75f );
	editor->setScroll( { 0.f, 10.75f } );
	EXPECT_NEAR( 10.f, editor->getScroll().y, 0.0001f );

	eeDelete( editor );
}

UTEST( UIScrolling, CodeEditorKeepsLegacyWheelStepButSoftensFractionalMomentumTail ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - wheel precision test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* editor = eeNew( ScrollingTestEditor, () );
	editor->setPixelsSize( 160, 80 );
	editor->setParent( app.getUI()->getRoot() );
	editor->getDocument().textInput( String( 100, '\n' ) );
	editor->setMouseWheelScroll( 50.f );
	app.getUI()->flushDirtyStyleAndLayout();

	editor->setScrollY( 100.f );
	EXPECT_EQ( 1u, editor->onMouseWheel( { 0.f, -0.1f }, false ) );
	EXPECT_NEAR( 105.f, editor->getScroll().y, 0.0001f );

	EXPECT_EQ( 1u, editor->onMouseWheel( { 0.f, -4.f }, false ) );
	EXPECT_NEAR( 155.f, editor->getScroll().y, 0.0001f );

	eeDelete( editor );
}
