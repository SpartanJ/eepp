#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/textdocument.hpp>

using namespace EE::UI::Doc;
using namespace EE::System;

UTEST( TextDocument, multicursor ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	TextDocument doc;
	doc.loadFromFile( "assets/textformat/english.utf8.lf.nobom.txt" );
	EXPECT_EQ( doc.linesCount() > 0, true );

	// Same line delete
	for ( int op = 0; op < 2; op++ ) {
		doc.setSelection( { { 0, 4 }, { 0, 5 } } );

		EXPECT_STRINGEQ( "a", doc.getSelectedText() );

		// Select all "a" from first line
		doc.selectWord();
		doc.selectWord();
		doc.selectWord();

		switch ( op ) {
			case 0:
				doc.deleteToPreviousChar();
				break;
			case 1:
				doc.deleteToNextChar();
				break;
		}

		EXPECT_STRINGEQ( "It ws  bright cold dy in April, nd the clocks were striking thirteen.\n",
						 doc.line( 0 ).getText() );

		doc.resetSelection( TextRange{ { 0, 0 }, { 0, 0 } } );
		doc.undo();
	}

	// Multi-line delete
	for ( int op = 0; op < 4; op++ ) {
		TextRanges ranges(
			{ TextRange( { 3, 65 }, { 4, 11 } ), TextRange( { 17, 66 }, { 18, 67 } ) } );

		if ( op >= 2 )
			for ( auto& range : ranges )
				range.reverse();

		doc.resetSelection( ranges );

		switch ( op % 2 ) {
			case 0:
				doc.deleteToPreviousChar();
				break;
			case 1:
				doc.deleteToNextChar();
				break;
		}

		EXPECT_STRINGEQ( "though not quickly enough to prevent a swirl of gritty dust from him.\n",
						 doc.line( 3 ).getText() );
		EXPECT_STRINGEQ( "one of those pictures which are so contrived that the eyes follow ran.\n",
						 doc.line( 16 ).getText() );
		EXPECT_STDSTREQ( TextRange( { 3, 65 }, { 3, 65 } ).toString(),
						 doc.getSelectionIndex( 0 ).toString() );
		EXPECT_STDSTREQ( TextRange( { 16, 66 }, { 16, 66 } ).toString(),
						 doc.getSelectionIndex( 1 ).toString() );

		doc.undo();
	}

	doc.resetUndoRedo();
	doc.resetSelection( TextRange{ { 0, 0 }, { 0, 0 } } );
}
