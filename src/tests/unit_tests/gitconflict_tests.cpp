#include "utest.h"

#include "../../tools/ecode/plugins/git/git.hpp"
#include <chrono>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <filesystem>

using namespace EE;
using namespace EE::System;
using namespace ecode;

namespace {

struct GitTempDirectory {
	GitTempDirectory() {
		path = std::filesystem::temp_directory_path() /
			   ( "eepp-git-conflict-" +
				 std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		std::filesystem::create_directories( path );
	}
	~GitTempDirectory() { FileSystem::dirRemoveAll( path.string() ); }
	std::filesystem::path path;
};

} // namespace

UTEST( GitConflict, ParsesNulDelimitedStageRecordsAndUnusualPaths ) {
	if ( Sys::which( "git" ).empty() )
		UTEST_SKIP( "Git is not installed" );

	const std::string oid( 40, 'a' );
	const std::string path = "space tab\tquote\" unicode-ñ newline\n.txt";
	std::string records = "100644 " + oid + " 1\t" + path;
	records += '\0';
	records += "100644 " + oid + " 2\t" + path;
	records += '\0';

	auto state = Git::parseUnmergedIndex( records );
	EXPECT_TRUE( state.error.empty() );
	ASSERT_EQ( 1u, state.files.size() );
	EXPECT_STREQ( path.c_str(), state.files.front().path.c_str() );
	EXPECT_TRUE( state.files.front().base.has_value() );
	EXPECT_TRUE( state.files.front().stage2.has_value() );
	EXPECT_FALSE( state.files.front().stage3.has_value() );
}

UTEST( GitConflict, RejectsMalformedUnmergedIndexRecords ) {
	if ( Sys::which( "git" ).empty() )
		UTEST_SKIP( "Git is not installed" );

	auto state = Git::parseUnmergedIndex( "not an index record\0" );
	EXPECT_FALSE( state.error.empty() );
	EXPECT_TRUE( state.files.empty() );
}

UTEST( GitConflict, DetectsRebaseDirectory ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );

	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	ASSERT_EQ( EXIT_SUCCESS,
			   git.git( std::vector<std::string>{ "init" }, temp.path.string(), output ) );
	ASSERT_TRUE( FileSystem::makeDir( ( temp.path / ".git/rebase-merge" ).string(), true ) );
	EXPECT_EQ( Git::GitOperation::Rebase, git.operation( temp.path.string() ) );
}

UTEST( GitConflict, KeepsMergeOperationAfterAllConflictsAreStaged ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );

	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		output.clear();
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "eepp tests" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "eepp-tests@example.invalid" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "rerere.enabled", "false" } ) );
	const std::string file = ( temp.path / "conflict.txt" ).string();
	ASSERT_TRUE( FileSystem::fileWrite( file, "base\n" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "add", "conflict.txt" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "-m", "base" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "-c", "incoming" } ) );
	ASSERT_TRUE( FileSystem::fileWrite( file, "incoming\n" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "-am", "incoming" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "main" } ) );
	ASSERT_TRUE( FileSystem::fileWrite( file, "current\n" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "-am", "current" } ) );
	EXPECT_NE( EXIT_SUCCESS, run( { "merge", "incoming" } ) );
	ASSERT_TRUE( FileSystem::fileWrite( file, "resolved\n" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "add", "conflict.txt" } ) );

	auto state = git.conflictState( temp.path.string(), false );
	EXPECT_TRUE( state.error.empty() );
	EXPECT_FALSE( state.hasConflicts() );
	EXPECT_EQ( Git::GitOperation::Merge, state.operation );

	ASSERT_EQ( EXIT_SUCCESS, run( { "merge", "--abort" } ) );
	EXPECT_NE( EXIT_SUCCESS, run( { "merge", "incoming" } ) );
	state = git.conflictState( temp.path.string(), false );
	EXPECT_TRUE( state.hasConflicts() );
	ASSERT_EQ( EXIT_SUCCESS, git.restoreHead( { "conflict.txt" }, temp.path.string() ).returnCode );
	state = git.conflictState( temp.path.string(), false );
	EXPECT_FALSE( state.hasConflicts() );
	std::string restored;
	ASSERT_TRUE( FileSystem::fileGet( file, restored ) );
	EXPECT_TRUE( restored == "current\n" );
}

UTEST( GitHistory, PaginatesFirstParentWithoutDuplicates ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );
	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "History Tester" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "history@example.invalid" } ) );
	for ( int i = 0; i < 5; ++i )
		ASSERT_EQ( EXIT_SUCCESS,
				   run( { "commit", "--allow-empty", "-m", "commit " + std::to_string( i ) } ) );
	Git::HistoryQuery query;
	query.limit = 2;
	auto first = git.history( query, temp.path.string() );
	ASSERT_TRUE( first.success() );
	ASSERT_EQ( 2u, first.commits.size() );
	ASSERT_TRUE( first.hasMore );
	query.continuation = first.commits.back().parents.front();
	auto second = git.history( query, temp.path.string() );
	ASSERT_TRUE( second.success() );
	ASSERT_EQ( 2u, second.commits.size() );
	EXPECT_FALSE( first.commits.back().hash == second.commits.front().hash );
	query.continuation = second.commits.back().parents.front();
	auto third = git.history( query, temp.path.string() );
	ASSERT_TRUE( third.success() );
	ASSERT_EQ( 1u, third.commits.size() );
	EXPECT_FALSE( third.hasMore );

	ASSERT_EQ( EXIT_SUCCESS, run( { "branch", "older", "HEAD~2" } ) );
	Git::HistoryQuery refQuery;
	refQuery.revision = "refs/heads/older";
	auto selectedRef = git.history( refQuery, temp.path.string() );
	ASSERT_TRUE( selectedRef.success() );
	ASSERT_EQ( 3u, selectedRef.commits.size() );
	EXPECT_STREQ( "commit 2", selectedRef.commits.front().subject.c_str() );
}

UTEST( GitHistory, PropagatedExclusionsHideAlreadyRepresentedMainline ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );
	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		output.clear();
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "History Tester" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "history@example.invalid" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "--allow-empty", "-m", "base" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "-c", "feature" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "--allow-empty", "-m", "feature one" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "--allow-empty", "-m", "main one" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "feature" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "merge", "--no-ff", "main", "-m", "merge main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "--allow-empty", "-m", "feature two" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "merge", "--no-ff", "feature", "-m", "merge feature" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "branch", "-D", "feature" } ) );

	Git::HistoryQuery rootQuery;
	auto root = git.history( rootQuery, temp.path.string() );
	ASSERT_TRUE( root.success() );
	ASSERT_TRUE( root.commits.front().isMerge() );
	Git::HistoryQuery featureQuery;
	featureQuery.revision = root.commits.front().parents[1];
	featureQuery.exclusions.emplace_back( root.commits.front().parents[0] );
	auto feature = git.history( featureQuery, temp.path.string() );
	ASSERT_TRUE( feature.success() );
	ASSERT_EQ( 3u, feature.commits.size() );
	ASSERT_TRUE( feature.commits[1].isMerge() );
	Git::HistoryQuery nestedQuery;
	nestedQuery.revision = feature.commits[1].parents[1];
	nestedQuery.exclusions = featureQuery.exclusions;
	nestedQuery.exclusions.emplace_back( feature.commits[1].parents[0] );
	auto nested = git.history( nestedQuery, temp.path.string() );
	ASSERT_TRUE( nested.success() );
	EXPECT_TRUE( nested.commits.empty() );
}

UTEST( GitHistory, HandlesEmptyRepositoryUnicodeAndHardLimit ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );
	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		output.clear();
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	Git::HistoryQuery query;
	auto empty = git.history( query, temp.path.string() );
	ASSERT_TRUE( empty.success() );
	EXPECT_TRUE( empty.commits.empty() );
	query.limit = 1001;
	EXPECT_TRUE( git.history( query, temp.path.string() ).fail() );

	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "Tést 🚀" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "unicode@example.invalid" } ) );
	const std::string subject = "unicode 🚀 | quote \" and tab\tend";
	const std::string message = subject + "\ncontinuation on a physical second line";
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "--allow-empty", "-m", message } ) );
	query.limit = 200;
	auto page = git.history( query, temp.path.string() );
	ASSERT_TRUE( page.success() );
	ASSERT_EQ( 1u, page.commits.size() );
	EXPECT_STREQ( subject.c_str(), page.commits.front().subject.c_str() );
	EXPECT_STREQ( message.c_str(), page.commits.front().message.c_str() );
	EXPECT_STREQ( "Tést 🚀", page.commits.front().authorName.c_str() );
}

UTEST( GitHistory, PaginatesMergedFirstParentLevelIndependently ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );
	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		output.clear();
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "History Tester" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "history@example.invalid" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "--allow-empty", "-m", "base" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "-c", "feature" } ) );
	for ( int i = 0; i < 5; ++i )
		ASSERT_EQ( EXIT_SUCCESS,
				   run( { "commit", "--allow-empty", "-m", "feature " + std::to_string( i ) } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "switch", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "merge", "--no-ff", "feature", "-m", "merge feature" } ) );

	Git::HistoryQuery rootQuery;
	auto root = git.history( rootQuery, temp.path.string() );
	ASSERT_TRUE( root.success() );
	ASSERT_TRUE( root.commits.front().isMerge() );
	Git::HistoryQuery childQuery;
	childQuery.revision = root.commits.front().parents[1];
	childQuery.exclusions.emplace_back( root.commits.front().parents[0] );
	childQuery.limit = 2;
	auto first = git.history( childQuery, temp.path.string() );
	ASSERT_TRUE( first.success() );
	ASSERT_EQ( 2u, first.commits.size() );
	ASSERT_TRUE( first.hasMore );
	childQuery.continuation = first.commits.back().parents.front();
	auto second = git.history( childQuery, temp.path.string() );
	ASSERT_TRUE( second.success() );
	ASSERT_EQ( 2u, second.commits.size() );
	ASSERT_TRUE( second.hasMore );
	childQuery.continuation = second.commits.back().parents.front();
	auto third = git.history( childQuery, temp.path.string() );
	ASSERT_TRUE( third.success() );
	ASSERT_EQ( 1u, third.commits.size() );
	EXPECT_FALSE( third.hasMore );
}

UTEST( GitHistory, ListsChangedFilesAndLoadsFirstParentDiff ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );
	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		output.clear();
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "History Tester" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "history@example.invalid" } ) );
	ASSERT_TRUE( FileSystem::fileWrite( ( temp.path / "old name.txt" ).string(),
										"same line one\nbefore\nsame line three\n" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "add", "old name.txt" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "-m", "base" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "mv", "old name.txt", "new name.txt" } ) );
	ASSERT_TRUE( FileSystem::fileWrite( ( temp.path / "new name.txt" ).string(),
										"same line one\nafter\nsame line three\n" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "add", "new name.txt" } ) );
	ASSERT_EQ( EXIT_SUCCESS,
			   run( { "commit", "-m", "rename and modify", "-m", "Detailed body line." } ) );
	ASSERT_EQ( EXIT_SUCCESS,
			   run( { "remote", "add", "origin", "git@github.com:SpartanJ/eepp.git" } ) );

	auto history = git.history( {}, temp.path.string() );
	ASSERT_TRUE( history.success() );
	ASSERT_FALSE( history.commits.empty() );
	auto files = git.commitFiles( history.commits.front(), temp.path.string() );
	ASSERT_TRUE( files.success() );
	ASSERT_EQ( 1u, files.files.size() );
	EXPECT_STREQ( "new name.txt", files.files.front().path.c_str() );
	EXPECT_STREQ( "old name.txt", files.files.front().oldPath.c_str() );
	EXPECT_EQ( 1, files.files.front().inserts );
	EXPECT_EQ( 1, files.files.front().deletes );
	EXPECT_FALSE( files.files.front().isBinary );
	EXPECT_NE( std::string::npos, files.message.find( "Detailed body line." ) );
	const std::string expectedCommitURL =
		"https://github.com/SpartanJ/eepp/commit/" + history.commits.front().hash;
	EXPECT_STREQ( expectedCommitURL.c_str(), files.commitURL.c_str() );
	auto diff = git.commitDiff( history.commits.front(), files.files.front(), temp.path.string() );
	ASSERT_TRUE( diff.success() );
	EXPECT_NE( std::string::npos, diff.result.find( "-before" ) );
	EXPECT_NE( std::string::npos, diff.result.find( "+after" ) );
}

UTEST( GitStatus, PreservesSuffixOfRenamedDirectoryNumstatPaths ) {
	const std::string gitPath = Sys::which( "git" );
	if ( gitPath.empty() )
		UTEST_SKIP( "Git is not installed" );
	GitTempDirectory temp;
	Git git( temp.path.string(), gitPath );
	std::string output;
	auto run = [&]( std::vector<std::string> args ) {
		output.clear();
		return git.git( args, temp.path.string(), output );
	};
	ASSERT_EQ( EXIT_SUCCESS, run( { "init", "-b", "main" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.name", "Status Tester" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "config", "user.email", "status@example.invalid" } ) );
	ASSERT_TRUE( FileSystem::makeDir( ( temp.path / "bin/assets/fontrendering" ).string(), true ) );
	ASSERT_TRUE( FileSystem::fileWrite(
		( temp.path / "bin/assets/fontrendering/image.webp" ).string(), "image contents" ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "add", "bin/assets/fontrendering/image.webp" } ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "commit", "-m", "base" } ) );
	ASSERT_TRUE( FileSystem::makeDir( ( temp.path / "bin/unit_tests" ).string(), true ) );
	ASSERT_EQ( EXIT_SUCCESS, run( { "mv", "bin/assets", "bin/unit_tests/assets" } ) );

	auto status = git.status( false, temp.path.string() );
	size_t staged = 0;
	size_t untracked = 0;
	for ( const auto& [_, files] : status.files ) {
		for ( const auto& file : files ) {
			if ( file.report.type == Git::GitStatusType::Staged ) {
				++staged;
				EXPECT_STREQ( "bin/unit_tests/assets/fontrendering/image.webp", file.file.c_str() );
			} else if ( file.report.type == Git::GitStatusType::Untracked ) {
				++untracked;
			}
		}
	}
	EXPECT_EQ( 1u, staged );
	EXPECT_EQ( 0u, untracked );
}
