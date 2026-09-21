#include "utest.h"

#include "../../tools/ecode/plugins/git/gitdiff.hpp"
#include <string>

using namespace ecode;

namespace {

std::string decorationSignature( const ComputedGitLineDiff& diff ) {
	std::string signature;
	signature.reserve( diff.lines.size() * 2 );
	for ( const auto& line : diff.lines ) {
		switch ( line.change ) {
			case GitLineChange::None:
				signature += 'n';
				break;
			case GitLineChange::Added:
				signature += 'a';
				break;
			case GitLineChange::Modified:
				signature += 'm';
				break;
		}
		if ( line.deletedBefore )
			signature += '^';
	}
	return signature;
}

bool matchesDiff( std::string_view baseline, std::string_view current,
				  std::string_view expectedSignature, bool expectedDeletedAtEOF = false ) {
	const auto diff = computeGitLineDiff( baseline, current );
	const auto actualSignature = decorationSignature( diff );
	return actualSignature == expectedSignature && diff.deletedAtEOF == expectedDeletedAtEOF;
}

} // namespace

UTEST( GitDiff, ClassifiesLineChangesAndDeletionBoundaries ) {
	EXPECT_TRUE( matchesDiff( "", "", "" ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "A\nB", "nn" ) );
	EXPECT_TRUE( matchesDiff( "A\nC", "A\nB\nC", "nan" ) );
	EXPECT_TRUE( matchesDiff( "A\nD", "A\nB\nC\nD", "naan" ) );
	EXPECT_TRUE( matchesDiff( "A\nB\nC", "A\nC", "nn^" ) );
	EXPECT_TRUE( matchesDiff( "A\nB\nC\nD", "A\nD", "nn^" ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "B", "n^" ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "A", "n", true ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "", "n", true ) );
	EXPECT_TRUE( matchesDiff( "A\nB\nC", "A\nX\nC", "nmn" ) );
	EXPECT_TRUE( matchesDiff( "A\nB\nC\nD", "A\nX\nY\nD", "nmmn" ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "A\nX\nY\nB", "naan" ) );
	EXPECT_TRUE( matchesDiff( "A\nB\nC\nD\nE", "A\nX\nE", "nm^n" ) );
	EXPECT_TRUE( matchesDiff( "A\nB\nC\nD\nE", "A\nX\nC\nY\nE", "nmnmn" ) );
	EXPECT_TRUE( matchesDiff( "", "A\nB", "aa" ) );
	EXPECT_TRUE( matchesDiff( "hello", "hello\n", "na" ) );
	EXPECT_TRUE( matchesDiff( "hello\n", "hello", "n", true ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "X\nB", "mn" ) );
	EXPECT_TRUE( matchesDiff( "A\nB", "A\nX", "nm" ) );
}

UTEST( GitDiff, NormalizesDocumentLineEndingsWithoutReallocation ) {
	std::string text = "A\r\nB\rC\n";
	const char* storage = text.data();
	normalizeGitDiffText( text );
	EXPECT_TRUE( text == "A\nB\nC\n" );
	EXPECT_TRUE( text.data() == storage );
}
