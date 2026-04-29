#include "utest.h"
#include <eepp/scene/node.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Doc;
using namespace EE::Scene;

static const std::string userCode = R"objcpp(#import "common.h"
#import <cmath>
#import <gdiplus.h>
#import <iostream>
#import <vector>
#import <windows.h>

@interface test () {
  struct DrawSineWave {
    struct AppState {
      int width = 800;
      int height = 600;
      Gdiplus::GdiplusStartupInput gdiplusStartupInput;
      ULONG_PTR gdiplusToken;
    };
    static void Draw(HDC hdc, int width, int height) {

      if (width <= 0 || height <= 0)
        return;

      using namespace Gdiplus;

      Graphics graphics(hdc);
      graphics.SetSmoothingMode(SmoothingModeAntiAlias);

      // Background
      SolidBrush bgBrush(Color(255, 255, 255, 255));
      graphics.FillRectangle(&bgBrush, 0, 0, width, height);

      // Axis
      Pen axisPen(Color(200, 200, 200), 1.0f);
      graphics.DrawLine(&axisPen, 0, height / 2, width, height / 2);

      // Sine wave parameters
      double amplitude = (height - 20) / 2.0;
      double midY = height / 2.0;
      double period = width;
      double twoPi = 6.283185307179586;

      // Build points
      std::vector<PointF> pts;
      double step = 0.25; // smaller step for smoother curve
      for (double x = 0; x < width; x += step) {
        double t = x / period;
        double y = midY - amplitude * std::sin(twoPi * t);
        pts.push_back(PointF(static_cast<REAL>(x), static_cast<REAL>(y)));
      }

      // Draw sine wave
      Pen sinePen(Color(0, 120, 215), 2.0f);
      if (!pts.empty()) {
        graphics.DrawLines(&sinePen, pts.data(), static_cast<INT>(pts.size()));
      }
    }
}
@end

OF_APPLICATION_DELEGATE(test)

@implementation test
- (void)applicationDidFinishLaunching:(OFNotification *)notification {
  HINSTANCE hInstance = GetModuleHandle(nullptr);
  DrawSineWave::Setup(hInstance, SW_SHOW);
  [OFApplication terminate];
}
@end
)objcpp";

#define VERIFY_CONSISTENCY( editor )                                                            \
	{                                                                                           \
		const DocumentView& view = editor->getDocumentView();                                   \
		TextDocument& doc = editor->getDocument();                                              \
		if ( !view.isOneToOne() ) {                                                             \
			EXPECT_EQ( (size_t)doc.linesCount(), view.getDocLineToVisibleIndex().size() );      \
			EXPECT_EQ( (size_t)doc.linesCount(), view.getVisibleLinesOffset().size() );         \
			size_t expectedVisibleCount = 0;                                                    \
			for ( Int64 i = 0; i < (Int64)doc.linesCount(); i++ ) {                             \
				if ( view.isLineVisible( i ) ) {                                                \
					EXPECT_NE( (Int64)VisibleIndex::invalid, (Int64)view.toVisibleIndex( i ) ); \
					Int64 startIdx = (Int64)view.toVisibleIndex( i );                           \
					Int64 endIdx = (Int64)view.toVisibleIndex( i, true );                       \
					expectedVisibleCount += ( endIdx - startIdx + 1 );                          \
				} else {                                                                        \
					EXPECT_EQ( (Int64)VisibleIndex::invalid, (Int64)view.toVisibleIndex( i ) ); \
				}                                                                               \
			}                                                                                   \
			EXPECT_EQ( expectedVisibleCount, view.getVisibleLinesCount() );                     \
		}                                                                                       \
	}

UTEST( UICodeEditor, DocumentViewStressTest ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Stress Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* editor = UICodeEditor::New();
	editor->setParent( (Node*)app.getUI() );
	editor->setPixelsSize( 800, 600 );
	editor->getDocument().setSyntaxDefinition(
		SyntaxDefinitionManager::instance()->getByLanguageName( "C++" ) );

	auto resetEditor = [&]() {
		editor->getDocument().selectAll();
		editor->getDocument().deleteSelection();
		editor->getDocument().textInput( userCode );
		editor->getDocument().getFoldRangeService().findRegionsNative();
		editor->unfoldAll();
	};

	// --- SCENARIO 1: Folding Only (No Wrap) ---
	editor->setLineWrapMode( LineWrapMode::NoWrap );
	resetEditor();

	// Fold everything
	editor->foldAll();
	VERIFY_CONSISTENCY( editor );

	// Delete while folded (should unfold affected)
	editor->getDocument().setSelection( { { 2, 0 }, { 10, 0 } } );
	editor->getDocument().deleteSelection();
	VERIFY_CONSISTENCY( editor );
	editor->getDocument().undo();
	VERIFY_CONSISTENCY( editor );

	// Insert text in the middle of a folded region
	editor->foldAll();
	editor->getDocument().setSelection( { { 5, 5 }, { 5, 5 } } );
	editor->getDocument().textInput( "STRESS_TEST" ); // Should unfold line 5
	VERIFY_CONSISTENCY( editor );

	// --- SCENARIO 2: Wrapping Only (No Folds) ---
	resetEditor();
	editor->setLineWrapMode( LineWrapMode::Letter );
	editor->setPixelsSize( 100, 600 ); // Force lots of wraps
	VERIFY_CONSISTENCY( editor );

	// Multi-line delete with wraps
	editor->getDocument().setSelection( { { 1, 5 }, { 4, 2 } } );
	editor->getDocument().deleteSelection();
	VERIFY_CONSISTENCY( editor );
	editor->getDocument().undo();
	VERIFY_CONSISTENCY( editor );

	// --- SCENARIO 3: Folding + Wrapping ---
	resetEditor();
	editor->setLineWrapMode( LineWrapMode::Letter );
	editor->setPixelsSize( 100, 600 );
	editor->foldAll();
	VERIFY_CONSISTENCY( editor );

	// Delete range straddling multiple folded regions with wraps
	// userCode has folds starting at lines: 1, 2, 3, 7
	editor->getDocument().setSelection( { { 0, 0 }, { 12, 0 } } );
	editor->getDocument().deleteSelection();
	VERIFY_CONSISTENCY( editor );
	editor->getDocument().undo();
	VERIFY_CONSISTENCY( editor );

	// Random heavy operations
	resetEditor();
	editor->setLineWrapMode( LineWrapMode::Word );
	editor->setPixelsSize( 200, 600 );

	for ( int i = 0; i < 5; i++ ) {
		editor->foldAll();
		editor->getDocument().setSelection( { { i * 2, 0 }, { i * 2 + 1, 5 } } );
		editor->getDocument().textInput( "RANDOM_INSERTION\nMORE_LINES\n" );
		VERIFY_CONSISTENCY( editor );
		editor->unfoldAll();
		VERIFY_CONSISTENCY( editor );
	}

	// Final check: Delete everything
	editor->getDocument().selectAll();
	editor->getDocument().deleteSelection();
	VERIFY_CONSISTENCY( editor );
	editor->getDocument().undo();
	VERIFY_CONSISTENCY( editor );
}

UTEST( UICodeEditor, FoldingCrashReproduction ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Reproduce Crash", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash() ) );

	auto* editor = UICodeEditor::New();
	editor->setParent( (Node*)app.getUI() );
	editor->setPixelsSize( 800, 600 );

	editor->getDocument().setSyntaxDefinition(
		SyntaxDefinitionManager::instance()->getByLanguageName( "C++" ) );
	editor->getDocument().textInput( userCode );

	// Wait for folding regions to be updated
	editor->getDocument().getFoldRangeService().findRegionsNative();

	// Try to reproduce the sequence that crashed:
	// 1. Fold regions
	editor->foldAll();

	// 2. Select everything and delete
	editor->getDocument().selectAll();
	editor->getDocument().deleteSelection();

	// 3. Undo
	editor->getDocument().undo();

	// 4. Unfold all
	editor->unfoldAll();

	// Another sequence: select range straddling folded region and delete
	editor->getDocument().textInput( userCode );
	editor->getDocument().getFoldRangeService().findRegionsNative();
	auto regions = editor->getDocument().getFoldRangeService().getFoldingRegions();

	if ( !regions.empty() ) {
		auto firstRegionLine = regions.begin()->first;
		auto firstRegionRange = regions.begin()->second;

		editor->fold( firstRegionLine );

		// Select from before the folded region to after
		TextRange sel( { firstRegionLine, 0 }, { firstRegionRange.end().line() + 1, 0 } );
		editor->getDocument().setSelection( sel );
		editor->getDocument().deleteSelection();
		editor->getDocument().undo();
	}
}

UTEST( UICodeEditor, ReproduceFoldingCrash ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - Reproduce Crash", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash() ) );

	auto* editor = UICodeEditor::New();
	editor->setParent( (Node*)app.getUI() );
	editor->setPixelsSize( 800, 600 );

	auto languages = SyntaxDefinitionManager::instance()->getLanguageNames();
	editor->getDocument().setSyntaxDefinition(
		SyntaxDefinitionManager::instance()->getByLanguageName( "C++" ) );
	editor->getDocument().textInput( userCode );

	// Wait for folding regions to be updated
	editor->getDocument().getFoldRangeService().findRegionsNative();

	auto regions = editor->getDocument().getFoldRangeService().getFoldingRegions();

	// Brute force: Try all combinations of folded regions
	size_t numRegions = regions.size();
	if ( numRegions > 8 )
		numRegions = 8; // Limit for speed

	for ( size_t i = 0; i < ( (size_t)1 << numRegions ); ++i ) {
		editor->getDocument().resetUndoRedo();
		editor->getDocument().resetSelection();
		editor->unfoldAll();

		size_t idx = 0;
		for ( auto const& [line, range] : regions ) {
			if ( ( i >> idx ) & 1 ) {
				editor->fold( line );
			}
			if ( ++idx >= numRegions )
				break;
		}

		// Try various selections and deletions
		idx = 0;
		for ( auto const& [line, range] : regions ) {
			// Selection from before the folded region to after
			TextRange sel( { line, 0 }, { range.end().line() + 1, 0 } );
			editor->getDocument().setSelection( sel );
			editor->getDocument().deleteSelection();
			editor->getDocument().undo();

			// Selection starting inside the folded region
			if ( range.end().line() > line ) {
				TextRange sel2( { line + 1, 0 }, { range.end().line() + 1, 0 } );
				editor->getDocument().setSelection( sel2 );
				editor->getDocument().deleteSelection();
				editor->getDocument().undo();
			}

			if ( ++idx >= numRegions )
				break;
		}

		// Also try foldAll then select everything and delete
		editor->foldAll();
		editor->getDocument().selectAll();
		editor->getDocument().deleteSelection();
		editor->getDocument().undo();
	}
}
