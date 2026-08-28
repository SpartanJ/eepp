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
}
