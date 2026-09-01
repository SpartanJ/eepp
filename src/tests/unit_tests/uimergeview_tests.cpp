#include "utest.h"
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/uiapplication.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;

UTEST( UIMergeView, ParsesNormalConflictMarkers ) {
	auto blocks = UIMergeView::parseConflictBlocks(
		"before\n<<<<<<< ours\nleft one\nleft two\n=======\nright\n>>>>>>> theirs\nafter\n" );
	ASSERT_EQ( 1u, blocks.size() );
	EXPECT_TRUE( blocks[0].stage2 == "left one\nleft two" );
	EXPECT_TRUE( blocks[0].stage3 == "right" );
	EXPECT_EQ( 1, blocks[0].range.start().line() );
	EXPECT_EQ( 7, blocks[0].range.end().line() );
}

UTEST( UIMergeView, ParsesDiff3WithoutTreatingBaseAsOurs ) {
	auto blocks = UIMergeView::parseConflictBlocks(
		"<<<<<<< ours\nleft\n||||||| base\nold\n=======\nright\n>>>>>>> theirs\n" );
	ASSERT_EQ( 1u, blocks.size() );
	EXPECT_TRUE( blocks[0].stage2 == "left" );
	EXPECT_TRUE( blocks[0].stage3 == "right" );
}

UTEST( UIMergeView, PreservesEmptyLinesAndLineRanges ) {
	auto blocks = UIMergeView::parseConflictBlocks(
		"before\n<<<<<<< ours\n\nleft\n\n=======\n\nright\n\n>>>>>>> theirs\nafter" );
	ASSERT_EQ( 1u, blocks.size() );
	EXPECT_TRUE( blocks[0].stage2 == "\nleft\n" );
	EXPECT_TRUE( blocks[0].stage3 == "\nright\n" );
	EXPECT_EQ( 1, blocks[0].range.start().line() );
	EXPECT_EQ( 10, blocks[0].range.end().line() );
}

UTEST( UIMergeView, SupportsConfiguredMarkerWidthsAndIgnoresMalformedBlocks ) {
	auto blocks = UIMergeView::parseConflictBlocks(
		"<<<<<<<<<< ours\nleft\n==========\nright\n>>>>>>>>>> theirs\n<<<<<<< incomplete\n" );
	ASSERT_EQ( 1u, blocks.size() );
	EXPECT_TRUE( blocks[0].stage2 == "left" );
	EXPECT_TRUE( blocks[0].stage3 == "right" );
}

UTEST( UIMergeView, UsesSharedResultDocumentAndAcceptIsUndoable ) {
	UIApplication app( WindowSettings{ 800, 600, "eepp - unit tests" } );
	auto result = std::make_shared<Doc::TextDocument>();
	result->setSyntaxDefinition(
		Doc::SyntaxDefinitionManager::instance()->getByLanguageName( "JavaScript" ) );
	const String conflicted = "<<<<<<< ours\nleft\n=======\nright\n>>>>>>> theirs\n";
	result->textInput( conflicted );
	result->resetUndoRedo();
	MergeInput input;
	input.resultDocument = result;
	input.stage2.present = true;
	input.stage2.text = "left";
	input.stage2.label = "Ours";
	input.stage3.present = true;
	input.stage3.text = "right";
	input.stage3.label = "Theirs";
	auto* view = UIMergeView::New();
	view->load( std::move( input ) );
	auto* toolbar = view->find<UIStackLayout>( "merge_toolbar" );
	auto* editorsLayout = view->find<UILinearLayout>( "merge_editors" );
	ASSERT_TRUE( toolbar != nullptr );
	ASSERT_TRUE( editorsLayout != nullptr );
	EXPECT_TRUE( view->getFirstChild() == toolbar );
	EXPECT_TRUE( toolbar->getNextNode() == editorsLayout );
	EXPECT_TRUE( view->getLeftEditor()->getParent() == editorsLayout );
	EXPECT_TRUE( view->getResultEditor()->getParent() == editorsLayout );
	EXPECT_TRUE( view->getRightEditor()->getParent() == editorsLayout );
	EXPECT_TRUE( view->getResultEditor()->getDocumentRef() == result );
	EXPECT_EQ( result->getSyntaxDefinition().getLanguageIndex(),
			   view->getLeftEditor()->getDocument().getSyntaxDefinition().getLanguageIndex() );
	EXPECT_EQ( result->getSyntaxDefinition().getLanguageIndex(),
			   view->getRightEditor()->getDocument().getSyntaxDefinition().getLanguageIndex() );
	EXPECT_FALSE( view->getLeftEditor()->getVerticalScrollBarEnabled() );
	EXPECT_FALSE( view->getResultEditor()->getVerticalScrollBarEnabled() );
	EXPECT_TRUE( view->getRightEditor()->getVerticalScrollBarEnabled() );
	EXPECT_TRUE( view->isToolbarVisible() );
	EXPECT_TRUE( view->hasCommand( "merge-accept-left" ) );
	auto* leftEditor = view->getLeftEditor();
	auto& leftDocument = leftEditor->getDocument();
	// Keep this hermetic: the Windows system clipboard can be temporarily owned by another
	// process. The regression is that replacement side documents must retain the copy command.
	EXPECT_TRUE( leftDocument.hasCommand( "copy" ) );
	leftDocument.setSelection( { { 0, 0 }, { 0, 4 } } );
	EXPECT_TRUE( leftDocument.getAllSelectedText() == "left" );
	view->setToolbarVisible( false );
	EXPECT_FALSE( view->isToolbarVisible() );
	view->setToolbarVisible( true );
	EXPECT_TRUE( view->execute( "merge-accept-left" ) );
	EXPECT_TRUE( result->getText() == "left\n" );
	result->undo();
	EXPECT_TRUE( result->getText() == conflicted );
	EXPECT_TRUE( view->execute( "merge-accept-left" ) );
	view->recreateConflict();
	EXPECT_TRUE( result->getText() == conflicted );
	const String twoConflicts =
		"before\n<<<<<<< ours\nleft one\n=======\nright one\n>>>>>>> theirs\nmiddle\n<<<<<<< "
		"ours\nleft two\n=======\nright two\n>>>>>>> theirs\nafter\n";
	result->selectAll();
	result->textInput( twoConflicts );
	result->setSelection( { 10, 0 } );
	EXPECT_TRUE( view->execute( "merge-accept-right" ) );
	EXPECT_TRUE( result->getText() == "before\n<<<<<<< ours\nleft one\n=======\nright one\n>>>>>>> "
									  "theirs\nmiddle\nright two\nafter\n" );
	eeDelete( view );
}
