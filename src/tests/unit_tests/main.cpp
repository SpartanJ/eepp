#include "utest.h"

#include <cstdlib>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>

UTEST_STATE();

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	// Packaging checks need to exercise launchers whose cwd differs from the executable path.
	if ( const char* cwd = std::getenv( "EEPP_TEST_RESOURCE_CWD" ) )
		EE::System::FileSystem::changeWorkingDirectory( cwd );
	else
		EE::System::FileSystem::changeWorkingDirectory( EE::System::Sys::getProcessPath() );
	EE::UI::UIApplication::setSystemFontsEnabledByDefault( false );
	return utest_main( argc, argv );
}
