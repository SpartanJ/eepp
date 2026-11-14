#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/vfs/batchrename.hpp>
#include <limits>

using namespace vfstest;

namespace {

class RenameTestProvider : public MemoryProvider {
  public:
	enum class Behavior : Uint8 { Normal, FailSecond, CancelFirst, Race, ReadOnly, Insensitive };
	CancellationSource* cancellation{ nullptr };
	Uint32 renames{ 0 };
	Behavior behavior{ Behavior::Normal };

	ProviderCapabilities capabilities( const URI& uri ) const override {
		auto caps = MemoryProvider::capabilities( uri );
		if ( behavior == Behavior::ReadOnly )
			caps.set( VFSCapability::Rename, false );
		if ( behavior == Behavior::Insensitive )
			caps.set( VFSCapability::CaseSensitive, false );
		return caps;
	}

	VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options,
						  CancellationToken token ) override {
		++renames;
		if ( behavior == Behavior::FailSecond && renames == 2 )
			return readyError<void>( VFSErrorCode::IO, from, "Injected rename failure" );
		if ( behavior == Behavior::Race && renames == 1 )
			putFile( to, "other writer" );
		auto task = MemoryProvider::rename( from, to, options, token );
		if ( behavior == Behavior::CancelFirst && renames == 1 )
			cancellation->cancel();
		return task;
	}
};

} // namespace

UTEST( VFSBatchRename, PatternEnumeratesAndPreservesCompleteExtensions ) {
	BatchRenamePattern pattern;
	pattern.text = "Photo ####";
	pattern.startIndex = 12;
	BatchRenameItem item{ "old.tar.GZ", "tar.GZ" };
	EXPECT_STDSTREQ( "Photo 0012.tar.GZ", pattern.apply( item ).value() );
	EXPECT_STDSTREQ( "Photo 0013.tar.GZ", pattern.apply( item, 1 ).value() );
	pattern.position = BatchRenamePattern::Position::Before;
	pattern.text = "##_";
	EXPECT_STDSTREQ( "12_old.tar.GZ", pattern.apply( item ).value() );
	pattern.position = BatchRenamePattern::Position::After;
	pattern.text = "_##";
	EXPECT_STDSTREQ( "old_12.tar.GZ", pattern.apply( item ).value() );
	pattern.position = BatchRenamePattern::Position::Replace;
	pattern.text = "Photo #.tar.GZ";
	EXPECT_STDSTREQ( "Photo 12.tar.GZ", pattern.apply( item ).value() );
	pattern.text = "#";
	pattern.startIndex = 0;
	EXPECT_STDSTREQ( "0.tar.GZ", pattern.apply( item ).value() );
	pattern.startIndex = std::numeric_limits<Uint64>::max();
	EXPECT_STDSTREQ( "18446744073709551615.tar.GZ", pattern.apply( item ).value() );
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( pattern.apply( item, 1 ) ) );
}

UTEST( VFSBatchRename, PatternValidatesPlaceholdersAndUnnumberedNames ) {
	BatchRenamePattern pattern;
	BatchRenameItem item{ "old.txt", "txt" };
	for ( const char* text : { "", "Photo", "Photo # #" } ) {
		pattern.text = text;
		EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( pattern.apply( item ) ) );
	}
	pattern.allowUnnumbered = true;
	pattern.text = "Photo";
	EXPECT_STDSTREQ( "Photo.txt", pattern.apply( item ).value() );
	pattern.allowUnnumbered = false;
	pattern.position = BatchRenamePattern::Position::Before;
	EXPECT_STDSTREQ( "Photoold.txt", pattern.apply( item ).value() );
	pattern.position = BatchRenamePattern::Position::After;
	EXPECT_STDSTREQ( "oldPhoto.txt", pattern.apply( item ).value() );
	pattern.text = "#";
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( pattern.apply( { "old.txt", "tar.gz" } ) ) );
}

UTEST( VFSBatchRename, ReplaceAndAddOperateOnStemsAndRetainUnknownDotSuffixes ) {
	BatchRenamePattern pattern;
	pattern.mode = BatchRenamePattern::Mode::ReplaceText;
	pattern.text = "old";
	pattern.replacement = " new";
	EXPECT_STDSTREQ( "new_ new.tar.gz", pattern.apply( { "old_old.tar.gz", "tar.gz" } ).value() );
	EXPECT_STDSTREQ( "new.TXT", pattern.apply( { "old.TXT", "TXT" } ).value() );
	EXPECT_STDSTREQ( "OLD.txt", pattern.apply( { "OLD.txt", "txt" } ).value() );
	pattern.replacement.clear();
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( pattern.apply( { "old.txt", "txt" } ) ) );
	pattern.mode = BatchRenamePattern::Mode::AddText;
	pattern.position = BatchRenamePattern::Position::Before;
	pattern.text = "#_";
	EXPECT_STDSTREQ( "#_Vol.1 Ch.3.1", pattern.apply( { "Vol.1 Ch.3.1", "" } ).value() );
	EXPECT_STDSTREQ( "#_.profile", pattern.apply( { ".profile", "" } ).value() );
	pattern.position = BatchRenamePattern::Position::After;
	pattern.text = "_é";
	EXPECT_STDSTREQ( "archive_é.tar.gz", pattern.apply( { "archive.tar.gz", "tar.gz" } ).value() );
	EXPECT_STDSTREQ( "folder.with.dots_é", pattern.apply( { "folder.with.dots", "" } ).value() );
}

UTEST( VFSBatchRename, PlansBeforeExecutingAndReturnsInputOrder ) {
	VirtualFileSystem::Config config;
	config.orchestrationThreads = 1;
	VirtualFileSystem vfs( config );
	auto memory = std::make_shared<RenameTestProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///rename" );
	memory->putFile( root.withAppendedSegment( "file10.txt" ), "ten" );
	memory->putFile( root.withAppendedSegment( "file2.txt" ), "two" );
	BatchRenamePattern pattern;
	pattern.text = "New ##";
	auto result = waitFor(
		vfs.batchRename( root, { { "file2.txt", "txt" }, { "file10.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( result.hasValue() );
	EXPECT_FALSE( result.value().error.has_value() );
	EXPECT_EQ( 2u, result.value().completed );
	EXPECT_STDSTREQ( "file2.txt", result.value().items[0].name );
	EXPECT_STDSTREQ( "New 01.txt", result.value().names[0] );
	EXPECT_STDSTREQ( "two", readString( vfs, root.withAppendedSegment( "New 01.txt" ) ) );
	EXPECT_STDSTREQ( "ten", readString( vfs, root.withAppendedSegment( "New 02.txt" ) ) );
	EXPECT_EQ( 2u, memory->renames );
}

UTEST( VFSBatchRename, InvalidAndDuplicateTargetsNeverMutateTheBatch ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<RenameTestProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///rename" );
	memory->putFile( root.withAppendedSegment( "a.txt" ), "a" );
	memory->putFile( root.withAppendedSegment( "b.txt" ), "b" );
	BatchRenamePattern pattern;
	pattern.text = "same";
	pattern.allowUnnumbered = true;
	auto duplicate =
		waitFor( vfs.batchRename( root, { { "a.txt", "txt" }, { "b.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( duplicate.hasValue() );
	ASSERT_TRUE( duplicate.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::AlreadyExists, duplicate.value().error->code );
	EXPECT_EQ( 0u, duplicate.value().completed );
	pattern.text = "../escape#";
	auto invalid =
		waitFor( vfs.batchRename( root, { { "a.txt", "txt" }, { "b.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( invalid.hasValue() );
	ASSERT_TRUE( invalid.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::InvalidOperation, invalid.value().error->code );
	EXPECT_EQ( 0u, memory->renames );
	EXPECT_STDSTREQ( "a", readString( vfs, root.withAppendedSegment( "a.txt" ) ) );
	EXPECT_STDSTREQ( "b", readString( vfs, root.withAppendedSegment( "b.txt" ) ) );
}

UTEST( VFSBatchRename, ExistingLaterTargetPreventsEveryRename ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<RenameTestProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///rename" );
	for ( const char* name : { "a.txt", "b.txt", "new2.txt" } )
		memory->putFile( root.withAppendedSegment( name ), name );
	BatchRenamePattern pattern;
	pattern.text = "new#";
	auto result =
		waitFor( vfs.batchRename( root, { { "a.txt", "txt" }, { "b.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( result.hasValue() );
	ASSERT_TRUE( result.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::AlreadyExists, result.value().error->code );
	EXPECT_EQ( 0u, memory->renames );
	EXPECT_EQ( VFSErrorCode::NotFound,
			   codeOf( waitFor( vfs.stat( root.withAppendedSegment( "new1.txt" ) ) ) ) );
	EXPECT_STDSTREQ( "new2.txt", readString( vfs, root.withAppendedSegment( "new2.txt" ) ) );
}

UTEST( VFSBatchRename, CancellationAndProviderFailureRetainTheCompletedPrefix ) {
	for ( auto behavior : { RenameTestProvider::Behavior::CancelFirst,
							RenameTestProvider::Behavior::FailSecond } ) {
		CancellationSource cancellation;
		VirtualFileSystem vfs;
		auto memory = std::make_shared<RenameTestProvider>();
		memory->behavior = behavior;
		memory->cancellation = &cancellation;
		vfs.registerProvider( memory );
		const URI root( "mem:///rename" );
		memory->putFile( root.withAppendedSegment( "a.txt" ), "a" );
		memory->putFile( root.withAppendedSegment( "b.txt" ), "b" );
		BatchRenamePattern pattern;
		pattern.text = "new#";
		auto result = waitFor( vfs.batchRename( root, { { "a.txt", "txt" }, { "b.txt", "txt" } },
												pattern, cancellation.token() ) );
		ASSERT_TRUE( result.hasValue() );
		ASSERT_TRUE( result.value().error.has_value() );
		EXPECT_EQ( 1u, result.value().completed );
		EXPECT_EQ( behavior == RenameTestProvider::Behavior::CancelFirst ? VFSErrorCode::Cancelled
																		 : VFSErrorCode::IO,
				   result.value().error->code );
		EXPECT_STDSTREQ( "a", readString( vfs, root.withAppendedSegment( "new1.txt" ) ) );
		EXPECT_STDSTREQ( "b", readString( vfs, root.withAppendedSegment( "b.txt" ) ) );
		EXPECT_EQ( VFSErrorCode::NotFound,
				   codeOf( waitFor( vfs.stat( root.withAppendedSegment( "new2.txt" ) ) ) ) );
	}
}

UTEST( VFSBatchRename, ProviderEnforcesNoOverwriteAfterPreflight ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<RenameTestProvider>();
	memory->behavior = RenameTestProvider::Behavior::Race;
	vfs.registerProvider( memory );
	const URI root( "mem:///rename" );
	memory->putFile( root.withAppendedSegment( "a.txt" ), "a" );
	BatchRenamePattern pattern;
	pattern.text = "new#";
	auto result = waitFor( vfs.batchRename( root, { { "a.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( result.hasValue() );
	ASSERT_TRUE( result.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::AlreadyExists, result.value().error->code );
	EXPECT_EQ( 0u, result.value().completed );
	EXPECT_STDSTREQ( "other writer", readString( vfs, root.withAppendedSegment( "new1.txt" ) ) );
	EXPECT_STDSTREQ( "a", readString( vfs, root.withAppendedSegment( "a.txt" ) ) );
}

UTEST( VFSBatchRename, NoOpsUnsupportedAndInitiallyCancelledBatches ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<RenameTestProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///rename" );
	memory->putFile( root.withAppendedSegment( "a.txt" ), "a" );
	BatchRenamePattern pattern;
	pattern.mode = BatchRenamePattern::Mode::ReplaceText;
	pattern.text = "not present";
	auto unchanged = waitFor( vfs.batchRename( root, { { "a.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( unchanged.hasValue() );
	EXPECT_FALSE( unchanged.value().error.has_value() );
	EXPECT_EQ( 1u, unchanged.value().completed );
	EXPECT_EQ( 0u, memory->renames );
	memory->behavior = RenameTestProvider::Behavior::ReadOnly;
	auto unsupported = waitFor( vfs.batchRename( root, { { "a.txt", "txt" } }, pattern ) );
	ASSERT_TRUE( unsupported.hasValue() );
	ASSERT_TRUE( unsupported.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::Unsupported, unsupported.value().error->code );
	memory->behavior = RenameTestProvider::Behavior::Normal;
	CancellationSource cancelled;
	cancelled.cancel();
	auto stopped =
		waitFor( vfs.batchRename( root, { { "a.txt", "txt" } }, pattern, cancelled.token() ) );
	ASSERT_TRUE( stopped.hasValue() );
	ASSERT_TRUE( stopped.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::Cancelled, stopped.value().error->code );
	EXPECT_EQ( 0u, memory->renames );
}

UTEST( VFSBatchRename, CaseInsensitiveTargetsAreCheckedDuringPlanning ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<RenameTestProvider>();
	memory->behavior = RenameTestProvider::Behavior::Insensitive;
	vfs.registerProvider( memory );
	const URI root( "mem:///rename" );
	memory->putFile( root.withAppendedSegment( "a.txt" ), "a" );
	memory->putFile( root.withAppendedSegment( "b.TXT" ), "b" );
	BatchRenamePattern pattern;
	pattern.text = "same";
	pattern.allowUnnumbered = true;
	auto result =
		waitFor( vfs.batchRename( root, { { "a.txt", "txt" }, { "b.TXT", "TXT" } }, pattern ) );
	ASSERT_TRUE( result.hasValue() );
	ASSERT_TRUE( result.value().error.has_value() );
	EXPECT_EQ( VFSErrorCode::AlreadyExists, result.value().error->code );
	EXPECT_EQ( 0u, memory->renames );
}

UTEST( VFSBatchRename, LocalNamesKeepURIDelimitersAndDirectoryDots ) {
	TempDir directory;
	VirtualFileSystem vfs;
	const URI root = directory.uri();
	ASSERT_TRUE(
		writeString( vfs, root.withAppendedSegment( "a #%.txt" ), "contents" ).hasValue() );
	ASSERT_TRUE(
		waitFor( vfs.makeDir( root.withAppendedSegment( "folder.with.dots" ) ) ).hasValue() );
	BatchRenamePattern pattern;
	pattern.mode = BatchRenamePattern::Mode::AddText;
	pattern.position = BatchRenamePattern::Position::After;
	pattern.text = "_#%";
	auto result = waitFor(
		vfs.batchRename( root, { { "a #%.txt", "txt" }, { "folder.with.dots", "" } }, pattern ) );
	ASSERT_TRUE( result.hasValue() );
	EXPECT_FALSE( result.value().error.has_value() );
	EXPECT_EQ( 2u, result.value().completed );
	EXPECT_STDSTREQ( "contents", readString( vfs, root.withAppendedSegment( "a #%_#%.txt" ) ) );
	auto folder = waitFor( vfs.stat( root.withAppendedSegment( "folder.with.dots_#%" ) ) );
	ASSERT_TRUE( folder.hasValue() );
	EXPECT_TRUE( folder.value().isDirectory() );
}
