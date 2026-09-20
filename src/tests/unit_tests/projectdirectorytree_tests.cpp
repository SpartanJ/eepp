#include "utest.h"

#include "../../tools/ecode/projectdirectorytree.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <eepp/system/filesystem.hpp>
#include <filesystem>
#include <future>

using namespace EE;
using namespace EE::System;
using namespace ecode;

namespace ecode {

// ProjectDirectoryTree only uses PluginManager when one is supplied. The unit-test target does not
// link the ecode application, so provide the null-manager path's unused symbols here.
UISceneNode* PluginManager::getUISceneNode() const {
	return nullptr;
}

void PluginManager::subscribeMessages(
	const std::string&, std::function<PluginRequestHandle( const PluginMessage& )> ) {}

void PluginManager::unsubscribeMessages( const std::string& ) {}

} // namespace ecode

namespace {

class ProjectDirectoryTreeTestDirectory {
  public:
	ProjectDirectoryTreeTestDirectory() {
		static std::atomic<Uint64> counter{ 0 };
		mPath = std::filesystem::temp_directory_path() /
				( "eepp-project-directory-tree-" +
				  std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) +
				  "-" + std::to_string( ++counter ) );
		std::filesystem::create_directories( mPath / ".git" );
	}

	~ProjectDirectoryTreeTestDirectory() { FileSystem::dirRemoveAll( mPath.string() ); }

	bool makeDirectory( const std::string& relativePath ) const {
		return std::filesystem::create_directories( mPath / relativePath ) ||
			   std::filesystem::is_directory( mPath / relativePath );
	}

	bool writeFile( const std::string& relativePath, std::string_view contents = {} ) const {
		const std::filesystem::path path( mPath / relativePath );
		std::filesystem::create_directories( path.parent_path() );
		return FileSystem::fileWrite( path.string(), contents );
	}

	std::string path( const std::string& relativePath = {} ) const {
		std::filesystem::path path( mPath );
		if ( !relativePath.empty() )
			path /= relativePath;
		path.make_preferred();
		return path.string();
	}

  private:
	std::filesystem::path mPath;
};

void scanAndWait( ProjectDirectoryTree& tree, const std::shared_ptr<ThreadPool>& pool ) {
	tree.scan( {} );
	std::promise<void> barrier;
	auto done = barrier.get_future();
	pool->run( [&barrier] { barrier.set_value(); } );
	done.wait();
}

void sendAdd( ProjectDirectoryTree& tree, const std::string& path ) {
	tree.onChange( ProjectDirectoryTree::Add, FileInfo( path ), {} );
}

bool hasDirectory( const ProjectDirectoryTree& tree, std::string directory ) {
	FileSystem::dirAddSlashAtEnd( directory );
	const auto directories = tree.getDirectories();
	return std::find( directories.begin(), directories.end(), directory ) != directories.end();
}

} // namespace

UTEST( ProjectDirectoryTree, RejectsIgnoredDirectoryCreatedAfterScan ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.writeFile( ".gitignore", "obj/\n" ) );
	ASSERT_TRUE( project.makeDirectory( "src" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.makeDirectory( "obj/linux/release" ) );
	ASSERT_TRUE( project.writeFile( "obj/linux/release/file.d" ) );
	sendAdd( tree, project.path( "obj" ) );
	sendAdd( tree, project.path( "obj/linux" ) );
	sendAdd( tree, project.path( "obj/linux/release" ) );
	sendAdd( tree, project.path( "obj/linux/release/file.d" ) );

	EXPECT_FALSE( hasDirectory( tree, project.path( "obj" ) ) );
	EXPECT_FALSE( hasDirectory( tree, project.path( "obj/linux" ) ) );
	EXPECT_FALSE( hasDirectory( tree, project.path( "obj/linux/release" ) ) );
	EXPECT_FALSE( tree.isFileInTree( project.path( "obj/linux/release/file.d" ) ) );
}

UTEST( ProjectDirectoryTree, RejectsNewDescendantsOfInitiallyIgnoredDirectory ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.writeFile( ".gitignore", "obj/\n" ) );
	ASSERT_TRUE( project.makeDirectory( "obj" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );
	ASSERT_FALSE( hasDirectory( tree, project.path( "obj" ) ) );

	ASSERT_TRUE( project.makeDirectory( "obj/linux/release" ) );
	ASSERT_TRUE( project.writeFile( "obj/linux/release/file.d" ) );
	sendAdd( tree, project.path( "obj/linux" ) );
	sendAdd( tree, project.path( "obj/linux/release" ) );
	sendAdd( tree, project.path( "obj/linux/release/file.d" ) );

	EXPECT_FALSE( hasDirectory( tree, project.path( "obj/linux" ) ) );
	EXPECT_FALSE( hasDirectory( tree, project.path( "obj/linux/release" ) ) );
	EXPECT_FALSE( tree.isFileInTree( project.path( "obj/linux/release/file.d" ) ) );
}

UTEST( ProjectDirectoryTree, RejectsDynamicallyCreatedPathSpecificIgnoredDirectory ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.writeFile( ".gitignore", "build/generated/\n" ) );
	ASSERT_TRUE( project.makeDirectory( "build" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.makeDirectory( "build/generated" ) );
	ASSERT_TRUE( project.writeFile( "build/generated/foo.cpp" ) );
	sendAdd( tree, project.path( "build/generated" ) );
	sendAdd( tree, project.path( "build/generated/foo.cpp" ) );

	EXPECT_FALSE( hasDirectory( tree, project.path( "build/generated" ) ) );
	EXPECT_FALSE( tree.isFileInTree( project.path( "build/generated/foo.cpp" ) ) );
}

UTEST( ProjectDirectoryTree, AdmitsLegitimateDynamicallyCreatedDirectory ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.makeDirectory( "src" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.makeDirectory( "src/newmodule" ) );
	ASSERT_TRUE( project.writeFile( "src/newmodule/foo.cpp" ) );
	sendAdd( tree, project.path( "src/newmodule" ) );
	sendAdd( tree, project.path( "src/newmodule/foo.cpp" ) );

	EXPECT_TRUE( hasDirectory( tree, project.path( "src/newmodule" ) ) );
	EXPECT_TRUE( tree.isFileInTree( project.path( "src/newmodule/foo.cpp" ) ) );
}

UTEST( ProjectDirectoryTree, RespectsNestedIgnoreFileForDynamicDirectory ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.makeDirectory( "src" ) );
	ASSERT_TRUE( project.writeFile( "src/.gitignore", "generated/\n" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.makeDirectory( "src/generated" ) );
	ASSERT_TRUE( project.writeFile( "src/generated/foo.cpp" ) );
	sendAdd( tree, project.path( "src/generated" ) );
	sendAdd( tree, project.path( "src/generated/foo.cpp" ) );

	EXPECT_FALSE( hasDirectory( tree, project.path( "src/generated" ) ) );
	EXPECT_FALSE( tree.isFileInTree( project.path( "src/generated/foo.cpp" ) ) );
}

UTEST( ProjectDirectoryTree, ScansNewDirectoryWithItsOwnIgnoreFile ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.makeDirectory( "src" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.writeFile( "src/newmodule/.gitignore", "generated/\n" ) );
	ASSERT_TRUE( project.writeFile( "src/newmodule/generated/foo.cpp" ) );
	sendAdd( tree, project.path( "src/newmodule" ) );

	EXPECT_TRUE( hasDirectory( tree, project.path( "src/newmodule" ) ) );
	EXPECT_FALSE( hasDirectory( tree, project.path( "src/newmodule/generated" ) ) );
	EXPECT_FALSE( tree.isFileInTree( project.path( "src/newmodule/generated/foo.cpp" ) ) );
}

UTEST( ProjectDirectoryTree, RespectsProjectDisallowedForDynamicFile ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.makeDirectory( "src" ) );
	ASSERT_TRUE( project.writeFile( ".ecode/.prjdisallowed", "src/private.cpp\n" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.writeFile( "src/private.cpp" ) );
	sendAdd( tree, project.path( "src/private.cpp" ) );

	EXPECT_FALSE( tree.isFileInTree( project.path( "src/private.cpp" ) ) );
}

UTEST( ProjectDirectoryTree, RespectsProjectAllowedForDynamicDirectory ) {
	ProjectDirectoryTreeTestDirectory project;
	ASSERT_TRUE( project.writeFile( ".gitignore", "vendor/\n" ) );
	ASSERT_TRUE( project.writeFile( ".ecode/.prjallowed", "vendor/\n" ) );
	auto pool = ThreadPool::createShared( 1 );
	ProjectDirectoryTree tree( project.path(), pool );
	scanAndWait( tree, pool );

	ASSERT_TRUE( project.makeDirectory( "vendor" ) );
	ASSERT_TRUE( project.writeFile( "vendor/library.cpp" ) );
	sendAdd( tree, project.path( "vendor" ) );
	sendAdd( tree, project.path( "vendor/library.cpp" ) );

	EXPECT_TRUE( hasDirectory( tree, project.path( "vendor" ) ) );
	EXPECT_TRUE( tree.isFileInTree( project.path( "vendor/library.cpp" ) ) );
}
