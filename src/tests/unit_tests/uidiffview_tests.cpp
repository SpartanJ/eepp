#include "utest.h"
#include <eepp/graphics/image.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;

UTEST( UIDiffView, LoadFromStringsAndVerifyDiffLines ) {
	UIApplication app( WindowSettings{ 800, 600, "eepp - unit tests" } );
	UIDiffView* diffView = UIDiffView::New();

	std::string oldText = "line 1\nline 2\nline 3\nline 4";
	std::string newText = "line 1\nline 2 changed\nline 3\nline 4 added\nline 5";

	diffView->loadFromStrings( oldText, newText );

	const auto& lines = diffView->getDiffLines();

	ASSERT_EQ( (size_t)7, lines.size() );

	EXPECT_EQ( UIDiffView::DiffLineType::Common, lines[0].type );
	EXPECT_TRUE( lines[0].text.toUtf8() == "line 1" );

	EXPECT_EQ( UIDiffView::DiffLineType::Added, lines[1].type );
	EXPECT_TRUE( lines[1].text.toUtf8() == "line 2 changed" );

	EXPECT_EQ( UIDiffView::DiffLineType::Removed, lines[2].type );
	EXPECT_TRUE( lines[2].text.toUtf8() == "line 2" );

	EXPECT_EQ( UIDiffView::DiffLineType::Common, lines[3].type );
	EXPECT_TRUE( lines[3].text.toUtf8() == "line 3" );

	EXPECT_EQ( UIDiffView::DiffLineType::Added, lines[4].type );
	EXPECT_TRUE( lines[4].text.toUtf8() == "line 4 added" );

	EXPECT_EQ( UIDiffView::DiffLineType::Added, lines[5].type );
	EXPECT_TRUE( lines[5].text.toUtf8() == "line 5" );

	EXPECT_EQ( UIDiffView::DiffLineType::Removed, lines[6].type );
	EXPECT_TRUE( lines[6].text.toUtf8() == "line 4" );

	const auto& text = diffView->getEditor()->getDocument().getText();

	std::string expectedCleanText =
		"line 1\nline 2 changed\nline 2\nline 3\nline 4 added\nline 5\nline 4\n";

	std::string textUtf8 = text.toUtf8();
	EXPECT_TRUE( expectedCleanText == textUtf8 );

	eeDelete( diffView );
}

UTEST( UIDiffView, LoadFromPatchAndVerifyCleanText ) {
	UIApplication app( WindowSettings{ 800, 600, "eepp - unit tests" } );
	UIDiffView* diffView = UIDiffView::New();

	std::string patchText = R"patch(+++ b/src/main.cpp
--- a/src/main.cpp
@@ -1,3 +1,3 @@
 int main() {
-  return 0;
+  return 1;
 }
)patch";

	diffView->loadFromPatch( patchText );

	const auto& lines = diffView->getDiffLines();

	ASSERT_EQ( (size_t)5, lines.size() );

	EXPECT_EQ( UIDiffView::DiffLineType::Common, lines[0].type );
	EXPECT_TRUE( lines[0].text.toUtf8() == "int main() {" );
	EXPECT_EQ( 1, lines[0].oldLineNum );
	EXPECT_EQ( 1, lines[0].newLineNum );

	EXPECT_EQ( UIDiffView::DiffLineType::Removed, lines[1].type );
	EXPECT_TRUE( lines[1].text.toUtf8() == "  return 0;" );
	EXPECT_EQ( 2, lines[1].oldLineNum );

	EXPECT_EQ( UIDiffView::DiffLineType::Added, lines[2].type );
	EXPECT_TRUE( lines[2].text.toUtf8() == "  return 1;" );
	EXPECT_EQ( 2, lines[2].newLineNum );

	EXPECT_EQ( UIDiffView::DiffLineType::Common, lines[3].type );
	EXPECT_TRUE( lines[3].text.toUtf8() == "}" );
	EXPECT_EQ( 3, lines[3].oldLineNum );
	EXPECT_EQ( 3, lines[3].newLineNum );

	const auto& text = diffView->getEditor()->getDocument().getText();

	std::string expectedCleanText = "int main() {\n  return 0;\n  return 1;\n}\n\n";

	std::string textUtf8 = text.toUtf8();
	EXPECT_TRUE( expectedCleanText == textUtf8 );

	eeDelete( diffView );
}

UTEST( UIDiffView, LoadFromFileImageDiffUsesImageViewers ) {
	UIApplication app( WindowSettings{ 800, 600, "eepp - unit tests" } );

	std::string oldImagePath( "/tmp/eepp-uidiffview-old.png" );
	std::string newImagePath( "/tmp/eepp-uidiffview-new.png" );
	Graphics::Image oldImage( 8, 8, 4, Color::Red );
	Graphics::Image newImage( 8, 8, 4, Color::Blue );
	ASSERT_TRUE( oldImage.saveToFile( oldImagePath, Graphics::Image::SaveType::PNG ) );
	ASSERT_TRUE( newImage.saveToFile( newImagePath, Graphics::Image::SaveType::PNG ) );

	UIDiffView* diffView = UIDiffView::New();
	diffView->setPixelsSize( 400, 200 );
	diffView->loadFromFile( oldImagePath, newImagePath );

	EXPECT_TRUE( diffView->isImageDiff() );
	EXPECT_TRUE( diffView->getLeftImageViewer()->isVisible() );
	EXPECT_TRUE( diffView->getRightImageViewer()->isVisible() );
	EXPECT_FALSE( diffView->getEditor()->isVisible() );
	EXPECT_FALSE( diffView->getLeftEditor()->isVisible() );
	EXPECT_FALSE( diffView->getRightEditor()->isVisible() );
	EXPECT_TRUE( diffView->getFileName().toUtf8() == "eepp-uidiffview-new.png" );

	eeDelete( diffView );
	FileSystem::fileRemove( oldImagePath );
	FileSystem::fileRemove( newImagePath );
}

UTEST( UIDiffView, LoadFromPatchImageDiffUsesProvidedOldFile ) {
	UIApplication app( WindowSettings{ 800, 600, "eepp - unit tests" } );

	std::string oldImagePath( "/tmp/eepp-uidiffview-patch-old.png" );
	std::string newImagePath( "/tmp/eepp-uidiffview-patch-new.png" );
	Graphics::Image oldImage( 8, 8, 4, Color::Red );
	Graphics::Image newImage( 8, 8, 4, Color::Blue );
	ASSERT_TRUE( oldImage.saveToFile( oldImagePath, Graphics::Image::SaveType::PNG ) );
	ASSERT_TRUE( newImage.saveToFile( newImagePath, Graphics::Image::SaveType::PNG ) );

	std::string patchText = R"patch(diff --git a/assets/logo.png b/assets/logo.png
index 1234567..89abcde 100644
Binary files a/assets/logo.png and b/assets/logo.png differ
)patch";

	UIDiffView* diffView = UIDiffView::New();
	diffView->setPixelsSize( 400, 200 );
	diffView->loadFromPatch( patchText, newImagePath, oldImagePath );

	EXPECT_TRUE( diffView->isImageDiff() );
	EXPECT_EQ( UIDiffView::ViewMode::SideBySide, diffView->getViewMode() );
	EXPECT_TRUE( diffView->getLeftImageViewer()->isVisible() );
	EXPECT_TRUE( diffView->getRightImageViewer()->isVisible() );
	EXPECT_TRUE( diffView->getFileName().toUtf8() == "logo.png" );

	eeDelete( diffView );
	FileSystem::fileRemove( oldImagePath );
	FileSystem::fileRemove( newImagePath );
}
