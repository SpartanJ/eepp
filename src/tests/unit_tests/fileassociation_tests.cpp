#include "utest.hpp"
#include "vfs_test_utils.hpp"

#include <algorithm>
#include <array>
#include <eepp/system/applications.hpp>
#include <eepp/system/fileassociation.hpp>
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

#if EE_PLATFORM != EE_PLATFORM_LINUX && EE_PLATFORM != EE_PLATFORM_BSD
UTEST( FileAssociation, unsupportedMimeRegistrationDoesNotChangeDefaults ) {
	FileAssociation association( { "fixture", "Fixture", {}, {} } );
	EXPECT_FALSE( FileAssociation::supportsMimeTypes() );
	EXPECT_FALSE(
		association.setRegisteredMimeTypes( { "inode/directory" }, { "inode/directory" } ) );
	EXPECT_FALSE( association.getLastError().empty() );
	EXPECT_TRUE( association.getRegisteredMimeTypes( { "inode/directory" } ).empty() );
	EXPECT_FALSE( association.getLastError().empty() );
}
#endif

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
namespace {

class XdgAssociationEnvironment {
  public:
	XdgAssociationEnvironment() {
		for ( size_t i = 0; i < previous.size(); ++i )
			previous[i] = Sys::getEnv( variables[i] );
	}

	~XdgAssociationEnvironment() {
		for ( size_t i = 0; i < previous.size(); ++i )
			Sys::setEnv( variables[i], previous[i].c_str() );
		Applications::invalidateCache();
	}

	bool initialize() {
		return Sys::setEnv( "XDG_CONFIG_HOME", config.c_str() ) &&
			   Sys::setEnv( "XDG_DATA_HOME", data.c_str() ) &&
			   Sys::setEnv( "XDG_CONFIG_DIRS", root.path.c_str() ) &&
			   Sys::setEnv( "XDG_DATA_DIRS", root.path.c_str() ) &&
			   Sys::setEnv( "XDG_CURRENT_DESKTOP", "KDE:Test" ) &&
			   FileSystem::makeDir( config, true ) &&
			   FileSystem::makeDir( data + "/applications", true );
	}

	vfstest::TempDir root;
	std::string config{ root.path + "config" };
	std::string data{ root.path + "data" };

  private:
	static constexpr const char* variables[] = { "XDG_CONFIG_HOME", "XDG_DATA_HOME",
												 "XDG_CONFIG_DIRS", "XDG_DATA_DIRS",
												 "XDG_CURRENT_DESKTOP" };
	std::array<std::string, 5> previous;
};

} // namespace

UTEST( FileAssociation, changesInstalledDefaultWithoutRewritingDesktopEntry ) {
	XdgAssociationEnvironment environment;
	ASSERT_TRUE( environment.initialize() );
	const auto& config = environment.config;
	const auto& data = environment.data;
	const auto& root = environment.root;
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
UTEST( FileAssociation, registersFoldersAndRestoresPreviousHandler ) {
	XdgAssociationEnvironment environment;
	ASSERT_TRUE( environment.initialize() );
	const auto& config = environment.config;
	const auto& data = environment.data;
	ASSERT_TRUE(
		FileSystem::fileWrite( data + "/applications/dolphin.desktop",
							   "[Desktop Entry]\nType=Application\nName=Dolphin\nExec=dolphin "
							   "%F\nMimeType=inode/directory;\n" ) );
	const std::array<std::string, 3> paths{ config + "/mimeapps.list",
											config + "/kde-mimeapps.list",
											data + "/applications/mimeapps.list" };
	for ( const auto& path : paths ) {
		ASSERT_TRUE( FileSystem::fileWrite(
			path,
			"# Preserve other handlers\n[Default Applications]\ninode/directory=dolphin.desktop;\n"
			"image/png=images.desktop;\n[Added "
			"Associations]\ninode/directory=dolphin.desktop;\n" ) );
	}
	FileAssociation association( { "fixture", "Fixture", Sys::getProcessFilePath(), {} } );
	ASSERT_TRUE( FileAssociation::supportsMimeTypes() );
	ASSERT_TRUE(
		association.setRegisteredMimeTypes( { "inode/directory" }, { "inode/directory" }, true ) );
	EXPECT_TRUE( association.isDesktopEntryInstalled() );
	EXPECT_EQ( 1u, association.getRegisteredMimeTypes( { "inode/directory" } ).size() );
	EXPECT_TRUE( association.getRegisteredExtensions( { "directory" } ).empty() );
	const auto desktopPath = data + "/applications/fixture.desktop";
	std::string desktop;
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, desktop ) );
	EXPECT_TRUE( desktop.find( "MimeType=inode/directory;\n" ) != std::string::npos );
	EXPECT_FALSE( FileSystem::fileExists( data + "/mime/packages/fixture.xml" ) );
	const URI folder = URI::fromFSPath( environment.root.path );
	auto handler = Applications::defaultFor( folder, { "inode/directory" } );
	ASSERT_TRUE( handler );
	EXPECT_STDSTREQ( "fixture.desktop", handler->id );
	// Hiding the launcher must retain the handler and its direct MIME registration.
	ASSERT_TRUE( association.setDesktopEntryInstalled( false ) );
	EXPECT_FALSE( association.isDesktopEntryInstalled() );
	EXPECT_EQ( 1u, association.getRegisteredMimeTypes( { "inode/directory" } ).size() );
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, desktop ) );
	EXPECT_TRUE( desktop.find( "NoDisplay=true" ) != std::string::npos );
	EXPECT_TRUE( desktop.find( "MimeType=inode/directory;\n" ) != std::string::npos );
	ASSERT_TRUE( association.setDesktopEntryInstalled( true ) );
	ASSERT_TRUE( association.setRegisteredMimeTypes( {}, { "inode/directory" }, false ) );
	EXPECT_TRUE( association.getRegisteredMimeTypes( { "inode/directory" } ).empty() );
	EXPECT_FALSE( FileSystem::fileExists( desktopPath ) );
	handler = Applications::defaultFor( folder, { "inode/directory" } );
	ASSERT_TRUE( handler );
	EXPECT_STDSTREQ( "dolphin.desktop", handler->id );
	for ( const auto& path : paths ) {
		std::string contents;
		ASSERT_TRUE( FileSystem::fileGet( path, contents ) );
		EXPECT_TRUE( contents.find( "# Preserve other handlers" ) != std::string::npos );
		EXPECT_TRUE( contents.find( "inode/directory=dolphin.desktop;" ) != std::string::npos );
		EXPECT_TRUE( contents.find( "image/png=images.desktop;" ) != std::string::npos );
		EXPECT_TRUE( contents.find( "fixture.desktop" ) == std::string::npos );
	}
}

UTEST( FileAssociation, mimeAndExtensionRegistrationsRemainIndependent ) {
	XdgAssociationEnvironment environment;
	ASSERT_TRUE( environment.initialize() );
	FileAssociation association( { "fixture", "Fixture", Sys::getProcessFilePath(), {} } );
	const std::vector<std::string> supported{ "inode/directory", "image/png" };
	ASSERT_TRUE( association.setRegisteredExtensions( { "eepp-test" }, { "eepp-test" }, true ) );
	ASSERT_TRUE( association.setRegisteredMimeTypes(
		{ "INODE/DIRECTORY", "inode/directory", "image/png", "text/plain" }, supported ) );
	auto registered = association.getRegisteredMimeTypes( { "INODE/DIRECTORY", "image/png" } );
	ASSERT_EQ( 2u, registered.size() );
	EXPECT_STDSTREQ( "image/png", registered[0] );
	EXPECT_STDSTREQ( "inode/directory", registered[1] );
	EXPECT_EQ( 1u, association.getRegisteredExtensions( { "eepp-test" } ).size() );
	// A type selected both directly and through an extension stays registered until both clear.
	ASSERT_TRUE( association.setRegisteredMimeTypes( { "application/x-fixture-eepp-test" },
													 { "application/x-fixture-eepp-test" } ) );
	ASSERT_TRUE( association.setRegisteredMimeTypes( {}, { "application/x-fixture-eepp-test" } ) );
	std::string desktop;
	const auto desktopPath = environment.data + "/applications/fixture.desktop";
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, desktop ) );
	EXPECT_TRUE( desktop.find( "MimeType=application/x-fixture-eepp-test;" ) != std::string::npos );
	ASSERT_TRUE( association.setRegisteredMimeTypes( { "inode/directory" }, supported ) );
	ASSERT_TRUE( association.setRegisteredExtensions( {}, { "eepp-test" }, false ) );
	EXPECT_EQ( 1u, association.getRegisteredMimeTypes( supported ).size() );
	EXPECT_TRUE( association.getRegisteredExtensions( { "eepp-test" } ).empty() );
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, desktop ) );
	EXPECT_TRUE( desktop.find( "MimeType=inode/directory;\n" ) != std::string::npos );
	EXPECT_FALSE( FileSystem::fileExists( environment.data + "/mime/packages/fixture.xml" ) );
	ASSERT_TRUE( association.setRegisteredMimeTypes( {}, supported ) );
	EXPECT_FALSE( FileSystem::fileExists( desktopPath ) );
}

UTEST( FileAssociation, invalidMimeTypesLeaveRegistrationUntouched ) {
	XdgAssociationEnvironment environment;
	ASSERT_TRUE( environment.initialize() );
	FileAssociation association( { "fixture", "Fixture", Sys::getProcessFilePath(), {} } );
	ASSERT_TRUE(
		association.setRegisteredMimeTypes( { "inode/directory" }, { "inode/directory" }, true ) );
	const auto desktopPath = environment.data + "/applications/fixture.desktop";
	const auto preferencesPath = environment.config + "/mimeapps.list";
	std::string desktop, preferences;
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, desktop ) );
	ASSERT_TRUE( FileSystem::fileGet( preferencesPath, preferences ) );
	for ( const auto* invalid :
		  { "", "inode", "/directory", "inode/", "inode/*", "image/png/extra",
			"inode/directory\nimage/png=bad.desktop", "inode/directory;image/png", "image/é" } ) {
		EXPECT_FALSE( association.setRegisteredMimeTypes( { "inode/directory", invalid },
														  { "inode/directory" }, true ) );
		EXPECT_FALSE( association.getLastError().empty() );
		EXPECT_FALSE( association.setRegisteredMimeTypes( { "inode/directory" },
														  { "inode/directory", invalid }, true ) );
		EXPECT_FALSE( association.getLastError().empty() );
		std::string actual;
		ASSERT_TRUE( FileSystem::fileGet( desktopPath, actual ) );
		EXPECT_STDSTREQ( desktop, actual );
		ASSERT_TRUE( FileSystem::fileGet( preferencesPath, actual ) );
		EXPECT_STDSTREQ( preferences, actual );
	}
	ASSERT_TRUE( association.setRegisteredMimeTypes( { "INODE/DIRECTORY", "inode/directory" },
													 { "inode/directory" }, true ) );
	std::string actual;
	ASSERT_TRUE( FileSystem::fileGet( desktopPath, actual ) );
	EXPECT_STDSTREQ( desktop, actual );
	ASSERT_TRUE( FileSystem::fileGet( preferencesPath, actual ) );
	EXPECT_STDSTREQ( preferences, actual );
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
