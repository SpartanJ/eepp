#include "../../eepp/ui/icons/freedesktopiconlookup.hpp"
#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/system/filesystem.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Private;
using vfstest::TempDir;

namespace {

void write( const std::string& path, std::string_view text ) {
	FileSystem::makeDir( FileSystem::fileRemoveFileName( path ), true );
	FileSystem::fileWrite( path, text );
}

void touch( const std::string& path ) {
	write( path, "x" );
}

/** A user and a system icon directory with a small theme, its parent and hicolor:
 *
 * - "test" (Inherits=parent): fixed 16, threshold 22 and scalable 8-256 directories.
 * - "parent": fixed 16.
 * - hicolor: fixed 48 and the only "app-only" icon.
 */
struct Themes {
	TempDir root;
	std::string user;
	std::string system;

	Themes() {
		user = root.path + "user/";
		system = root.path + "system/";
		FileSystem::makeDir( user, true );
		write( system + "test/index.theme",
			   std::string_view(
				   "[Icon Theme]\nName=Test\nInherits=parent\n"
				   "Directories=16/places,22/places,scalable/places\n\n"
				   "[16/places]\nSize=16\nType=Fixed\n\n"
				   "[22/places]\nSize=22\nType=Threshold\nThreshold=2\n\n"
				   "[scalable/places]\nSize=64\nMinSize=8\nMaxSize=256\nType=Scalable\n" ) );
		touch( system + "test/16/places/folder.png" );
		touch( system + "test/22/places/folder.svg" );
		touch( system + "test/scalable/places/folder.svg" );
		touch( system + "test/16/places/user-home.png" );
		write( system + "parent/index.theme",
			   std::string_view( "[Icon Theme]\nName=Parent\n"
								 "Directories=16/mimetypes\n\n"
								 "[16/mimetypes]\nSize=16\nType=Fixed\n" ) );
		touch( system + "parent/16/mimetypes/text-x-generic.png" );
		touch( system + "parent/16/mimetypes/text-plain.png" );
		write( system + "hicolor/index.theme",
			   std::string_view( "[Icon Theme]\nName=Hicolor\n"
								 "Directories=48/apps\n\n"
								 "[48/apps]\nSize=48\nType=Fixed\n" ) );
		touch( system + "hicolor/48/apps/app-only.png" );
		touch( system + "hicolor/48/apps/text-x-python.png" );
		touch( system + "loose.png" );
	}

	std::vector<std::string> paths() const { return { user, system }; }
};

} // namespace

UTEST( FreedesktopIconLookup, sizesAndDirectoryTypes ) {
	Themes themes;
	FreedesktopIconLookup lookup( "test", themes.paths() );
	ASSERT_TRUE( lookup.isValid() );
	EXPECT_STDSTREQ( "test", lookup.getThemeName() );
	// Exact fixed size, a threshold match (22 +/- 2) and the scalable range.
	EXPECT_STDSTREQ( themes.system + "test/16/places/folder.png", lookup.find( { "folder" }, 16 ) );
	EXPECT_STDSTREQ( themes.system + "test/22/places/folder.svg", lookup.find( { "folder" }, 24 ) );
	EXPECT_STDSTREQ( themes.system + "test/scalable/places/folder.svg",
					 lookup.find( { "folder" }, 128 ) );
	// Outside every range: the closest directory (only 16 has user-home).
	EXPECT_STDSTREQ( themes.system + "test/16/places/user-home.png",
					 lookup.find( { "user-home" }, 64 ) );
}

UTEST( FreedesktopIconLookup, inheritanceAndNameOrder ) {
	Themes themes;
	FreedesktopIconLookup lookup( "test", themes.paths() );
	// Parents (Inherits), then hicolor.
	EXPECT_STDSTREQ( themes.system + "parent/16/mimetypes/text-plain.png",
					 lookup.find( { "text-plain" }, 16 ) );
	EXPECT_STDSTREQ( themes.system + "hicolor/48/apps/app-only.png",
					 lookup.find( { "app-only" }, 16 ) );
	// The theme chain is walked first: a generic icon of the user's theme family wins over a more
	// specific one that only hicolor has.
	EXPECT_STDSTREQ( themes.system + "parent/16/mimetypes/text-x-generic.png",
					 lookup.find( { "text-x-python", "text-x-generic" }, 16 ) );
	// Unthemed icons last, then nothing.
	EXPECT_STDSTREQ( themes.system + "loose.png", lookup.find( { "loose" }, 16 ) );
	EXPECT_TRUE( lookup.find( { "missing" }, 16 ).empty() );
}

UTEST( FreedesktopIconLookup, userIconsOverrideAndMissingThemes ) {
	Themes themes;
	// A user copy of a theme icon wins over the system one.
	touch( themes.user + "test/16/places/folder.png" );
	FreedesktopIconLookup lookup( "test", themes.paths() );
	EXPECT_STDSTREQ( themes.user + "test/16/places/folder.png", lookup.find( { "folder" }, 16 ) );

	// A theme that is not installed falls back to hicolor alone.
	FreedesktopIconLookup missing( "not-installed", themes.paths() );
	ASSERT_TRUE( missing.isValid() );
	EXPECT_STDSTREQ( "hicolor", missing.getThemeName() );
	EXPECT_TRUE( missing.find( { "folder" }, 16 ).empty() );
	EXPECT_STDSTREQ( themes.system + "hicolor/48/apps/app-only.png",
					 missing.find( { "app-only" }, 48 ) );

	// No themes at all.
	TempDir empty;
	FreedesktopIconLookup none( "test", { empty.path } );
	EXPECT_FALSE( none.isValid() );
	EXPECT_TRUE( none.find( { "folder" }, 16 ).empty() );
}
