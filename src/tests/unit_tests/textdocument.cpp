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

UTEST( TextDocument, fileMightBeBinary ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	auto files = FileSystem::filesInfoGetInPath( "assets/textfiles" );
	for ( const auto& file : files ) {
		EXPECT_FALSE_MSG(
			TextDocument::fileMightBeBinary( file.getFilepath() ),
			String::format( "File %s should be detected as text file", file.getFilepath() )
				.c_str() );
	}
}

UTEST( TextDocument, newLineAutoIndent ) {
	TextDocument doc;
	doc.setAutoCloseBrackets( true );
	doc.setIndentType( TextDocument::IndentType::IndentTabs );
	doc.insert( 0, { 0, 0 }, "if ( true ) {" );
	doc.insert( 0, { 0, 13 }, "}" );
	doc.setSelection( { 0, 13 } ); // Between { and }
	doc.newLine();

	EXPECT_EQ( doc.linesCount(), 3UL );
	EXPECT_STRINGEQ( "if ( true ) {\n", doc.line( 0 ).getText() );
	EXPECT_STRINGEQ( "\t\n", doc.line( 1 ).getText() );
	EXPECT_STRINGEQ( "}\n", doc.line( 2 ).getText() );
	EXPECT_STDSTREQ( TextPosition( 1, 1 ).toString(), doc.getSelection().start().toString() );
}

UTEST( TextDocument, newLineMultiCursorAutoIndent ) {
	TextDocument doc;
	doc.setAutoCloseBrackets( true );
	doc.setIndentType( TextDocument::IndentType::IndentTabs );
	doc.insert( 0, { 0, 0 }, "{\n\t{\n\t\t(\n" );
	// Add closing brackets
	doc.insert( 0, { 0, 1 }, "}" );
	doc.insert( 0, { 1, 2 }, "}" );
	doc.insert( 0, { 2, 3 }, ")" );

	// Cursors between all pairs
	doc.resetSelection( TextRanges( std::vector<TextRange>{ TextRange( { 0, 1 }, { 0, 1 } ),
															TextRange( { 1, 2 }, { 1, 2 } ),
															TextRange( { 2, 3 }, { 2, 3 } ) } ) );

	doc.newLine();

	EXPECT_EQ( doc.linesCount(), 10UL );
	EXPECT_STRINGEQ( "{\n", doc.line( 0 ).getText() );
	EXPECT_STRINGEQ( "\t\n", doc.line( 1 ).getText() );
	EXPECT_STRINGEQ( "}\n", doc.line( 2 ).getText() );
	EXPECT_STRINGEQ( "\t{\n", doc.line( 3 ).getText() );
	EXPECT_STRINGEQ( "\t\t\n", doc.line( 4 ).getText() );
	EXPECT_STRINGEQ( "\t}\n", doc.line( 5 ).getText() );
	EXPECT_STRINGEQ( "\t\t(\n", doc.line( 6 ).getText() );
	EXPECT_STRINGEQ( "\t\t\t\n", doc.line( 7 ).getText() );
	EXPECT_STRINGEQ( "\t\t)\n", doc.line( 8 ).getText() );
	EXPECT_STRINGEQ( "\n", doc.line( 9 ).getText() );
}

UTEST( TextDocument, newLineNormal ) {
	TextDocument doc;
	doc.setIndentType( TextDocument::IndentType::IndentTabs );
	doc.insert( 0, { 0, 0 }, "\t\tif ( true )" );
	doc.setSelection( { 0, 13 } );
	doc.newLine();

	EXPECT_EQ( doc.linesCount(), 2UL );
	EXPECT_STRINGEQ( "\t\tif ( true )\n", doc.line( 0 ).getText() );
	EXPECT_STRINGEQ( "\t\t\n", doc.line( 1 ).getText() );
	EXPECT_STDSTREQ( TextPosition( 1, 2 ).toString(), doc.getSelection().start().toString() );
}

UTEST( TextDocument, autoCloseBrackets ) {
	TextDocument doc;
	doc.setAutoCloseBrackets( true );

	// Test word boundary
	doc.insert( 0, { 0, 0 }, "word" );
	doc.setSelection( { 0, 0 } ); // Before 'word'
	doc.textInput( "(" );		  // Next char 'w' is a word char, shouldn't auto close
	EXPECT_STRINGEQ( "(word\n", doc.line( 0 ).getText() );

	doc.reset();
	doc.setAutoCloseBrackets( true );
	doc.insert( 0, { 0, 0 }, " word" );
	doc.setSelection( { 0, 0 } ); // Before ' word'
	doc.textInput( "(" );		  // Next char ' ' is not a word char, should auto close
	EXPECT_STRINGEQ( "() word\n", doc.line( 0 ).getText() );

	doc.reset();
	doc.setAutoCloseBrackets( true );
	doc.insert( 0, { 0, 0 }, "() )" );
	doc.setSelection( { 0, 1 } ); // Inside first parens
	doc.textInput( "(" );		  // Unmatched right paren ahead, shouldn't auto close
	EXPECT_STRINGEQ( "(() )\n", doc.line( 0 ).getText() );

	doc.reset();
	doc.setAutoCloseBrackets( true );
	doc.insert( 0, { 0, 0 }, "()" );
	doc.setSelection( { 0, 1 } ); // Inside first parens
	doc.textInput( "(" );		  // Balanced right paren ahead, should auto close
	EXPECT_STRINGEQ( "(())\n", doc.line( 0 ).getText() );

	doc.reset();
	doc.setAutoCloseBrackets( true );
	doc.insert( 0, { 0, 0 }, "(\"\")" );
	doc.setSelection( { 0, 2 } ); // Inside quotes
	doc.textInput( "\"" );		  // Overwrites existing quote (stepping over)
	EXPECT_STRINGEQ( "(\"\")\n", doc.line( 0 ).getText() );

	doc.reset();
	doc.setAutoCloseBrackets( true );
	doc.insert( 0, { 0, 0 }, "()" );
	doc.setSelection( { 0, 1 } ); // Inside parens
	doc.textInput( "\"" );		  // Balanced quotes (0), should auto close
	EXPECT_STRINGEQ( "(\"\")\n", doc.line( 0 ).getText() );
}
