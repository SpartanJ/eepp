#include "utest.hpp"
#include "vfs_test_utils.hpp"

#include <algorithm>
#include <array>
#include <eepp/system/applications.hpp>
#include <eepp/system/fileassociation.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI::Doc;

UTEST( FileAssociation, normalizesExtensions ) {
	EXPECT_STDSTREQ( "cpp", FileAssociation::normalizeExtension( ".CPP" ) );
	EXPECT_STDSTREQ( "d.ts", FileAssociation::normalizeExtension( "..D.TS" ) );
	EXPECT_STDSTREQ( "c++", FileAssociation::normalizeExtension( "c++" ) );
	EXPECT_TRUE( FileAssociation::normalizeExtension( "bad/ext" ).empty() );
	EXPECT_TRUE( FileAssociation::normalizeExtension( "." ).empty() );
}

UTEST( FileAssociation, rejectsMissingDefaultApplication ) {
	std::string error = "previous error";
	EXPECT_EQ( FileAssociation::DefaultResult::Failed,
			   FileAssociation::setDefaultApplication( {}, "txt", "text/plain", error ) );
	EXPECT_FALSE( error.empty() );
}

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS
UTEST( FileAssociation, rejectsInvalidDefaultExtensionWithoutOpeningSystemUI ) {
	const Application application{ "fixture", "Fixture", {}, {}, false };
	std::string error;
	for ( const auto* extension : { "", ".", "bad/ext" } ) {
		EXPECT_EQ(
			FileAssociation::DefaultResult::Failed,
			FileAssociation::setDefaultApplication( application, extension, "text/plain", error ) );
		EXPECT_FALSE( error.empty() );
	}
}
#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
UTEST( FileAssociation, changesInstalledDefaultWithoutRewritingDesktopEntry ) {
	vfstest::TempDir root;
	constexpr const char* variables[] = { "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CONFIG_DIRS",
										  "XDG_DATA_DIRS", "XDG_CURRENT_DESKTOP" };
	std::array<std::string, 5> previous;
	for ( size_t i = 0; i < previous.size(); ++i )
		previous[i] = Sys::getEnv( variables[i] );
	ScopedOp environment( nullptr, [&] {
		for ( size_t i = 0; i < previous.size(); ++i )
			Sys::setEnv( variables[i], previous[i].c_str() );
		Applications::invalidateCache();
	} );
	const auto config = root.path + "config";
	const auto data = root.path + "data";
	ASSERT_TRUE( Sys::setEnv( "XDG_CONFIG_HOME", config.c_str() ) );
	ASSERT_TRUE( Sys::setEnv( "XDG_DATA_HOME", data.c_str() ) );
	ASSERT_TRUE( Sys::setEnv( "XDG_CONFIG_DIRS", root.path.c_str() ) );
	ASSERT_TRUE( Sys::setEnv( "XDG_DATA_DIRS", root.path.c_str() ) );
	ASSERT_TRUE( Sys::setEnv( "XDG_CURRENT_DESKTOP", "KDE:Test" ) );
	ASSERT_TRUE( FileSystem::makeDir( config, true ) );
	ASSERT_TRUE( FileSystem::makeDir( data + "/applications", true ) );
	const std::string desktop =
		"[Desktop Entry]\nType=Application\nName=Chosen\nExec=chosen --new-window %U\n"
		"Icon=chosen-icon\nTerminal=true\nMimeType=text/x-eepp-test;\n";
	const auto desktopPath = data + "/applications/org.example.Chosen.desktop";
	ASSERT_TRUE( FileSystem::fileWrite( desktopPath, desktop ) );
	ASSERT_TRUE( FileSystem::fileWrite( data + "/applications/old.desktop",
										"[Desktop Entry]\nType=Application\nName=Old\nExec=old "
										"%F\nMimeType=text/x-eepp-test;\n" ) );
	const std::string preferences =
		"# Keep this comment\n[Default Applications]\ntext/x-eepp-test=old.desktop;\n"
		"image/png=images.desktop;\n[Added Associations]\ntext/x-eepp-test=old.desktop;\n"
		"[Removed Associations]\ntext/x-eepp-test=org.example.Chosen.desktop;blocked.desktop;\n";
	const std::array<std::string, 3> paths{ config + "/mimeapps.list",
											config + "/kde-mimeapps.list",
											data + "/applications/mimeapps.list" };
	for ( const auto& path : paths )
		ASSERT_TRUE( FileSystem::fileWrite( path, preferences ) );
	Applications::invalidateCache();
	const URI file( "sftp://example.test/document.eepp-test" );
	auto before = Applications::defaultFor( file, { "text/x-eepp-test" } );
	ASSERT_TRUE( before.has_value() );
	EXPECT_STDSTREQ( "old.desktop", before->id );
	const Application application{ "org.example.Chosen.desktop", "Chosen", {}, {}, false };
	std::string error = "previous error";
	ASSERT_EQ( FileAssociation::DefaultResult::Applied,
			   FileAssociation::setDefaultApplication( application, "eepp-test", "text/x-eepp-test",
													   error ) );
	EXPECT_TRUE( error.empty() );
	auto after = Applications::defaultFor( file, { "text/x-eepp-test" } );
	ASSERT_TRUE( after.has_value() );
	EXPECT_STDSTREQ( application.id, after->id );
	EXPECT_STDSTREQ( "chosen --new-window %U", after->exec );
	EXPECT_TRUE( after->terminal );
	std::string contents;
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, contents ) );
	EXPECT_STDSTREQ( desktop, contents );
	for ( const auto& path : paths ) {
		ASSERT_TRUE( FileSystem::fileGet( path, contents ) );
		EXPECT_TRUE( contents.find( "# Keep this comment" ) != std::string::npos );
		EXPECT_TRUE( contents.find( "image/png=images.desktop;" ) != std::string::npos );
		EXPECT_TRUE( contents.find( "text/x-eepp-test=org.example.Chosen.desktop;old.desktop;" ) !=
					 std::string::npos );
		EXPECT_TRUE( contents.find( "text/x-eepp-test=blocked.desktop;" ) != std::string::npos );
	}
	const auto firstWrite = contents;
	ASSERT_EQ( FileAssociation::DefaultResult::Applied,
			   FileAssociation::setDefaultApplication( application, "eepp-test", "text/x-eepp-test",
													   error ) );
	ASSERT_TRUE( FileSystem::fileGet( paths.back(), contents ) );
	EXPECT_STDSTREQ( firstWrite, contents );

	// Invalid inputs must leave the existing preferences intact.
	for ( const auto* mime : { "", "text", "text/", "text/plain\nimage/png=bad.desktop" } ) {
		EXPECT_EQ( FileAssociation::DefaultResult::Failed,
				   FileAssociation::setDefaultApplication( application, "txt", mime, error ) );
		EXPECT_FALSE( error.empty() );
	}
	EXPECT_EQ( FileAssociation::DefaultResult::Failed,
			   FileAssociation::setDefaultApplication(
				   { "missing.desktop", "Missing", {}, {}, false }, "txt", "text/plain", error ) );
	ASSERT_TRUE( FileSystem::fileGet( paths.back(), contents ) );
	EXPECT_STDSTREQ( firstWrite, contents );

	// A configuration path occupied by a regular file reports a useful write failure.
	const auto blocked = root.path + "blocked";
	ASSERT_TRUE( FileSystem::fileWrite( blocked, "occupied" ) );
	ASSERT_TRUE( Sys::setEnv( "XDG_CONFIG_HOME", blocked.c_str() ) );
	EXPECT_EQ( FileAssociation::DefaultResult::Failed,
			   FileAssociation::setDefaultApplication( application, "eepp-test", "text/x-eepp-test",
													   error ) );
	EXPECT_FALSE( error.empty() );
}
#endif

UTEST( SyntaxDefinitionManager, listsLiteralFileExtensionsWithoutLoadingDefinitions ) {
	auto* manager = SyntaxDefinitionManager::createSingleton();
	manager->addPreDefinition( { "Unit Test Language",
								 []() -> SyntaxDefinition& {
									 static SyntaxDefinition definition;
									 return definition;
								 },
								 { "%.unitext$", "^unitfile$" } } );
	const auto extensions = manager->getFileExtensions();

	EXPECT_TRUE( std::binary_search( extensions.begin(), extensions.end(), "cpp" ) );
	EXPECT_TRUE( std::binary_search( extensions.begin(), extensions.end(), "json" ) );
	EXPECT_TRUE( std::binary_search( extensions.begin(), extensions.end(), "unitext" ) );
	EXPECT_FALSE( std::binary_search( extensions.begin(), extensions.end(), "unitfile" ) );
}
