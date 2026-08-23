#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>

UTEST_STATE();

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	EE::System::FileSystem::changeWorkingDirectory( EE::System::Sys::getProcessPath() );
	EE::UI::UIApplication::setSystemFontsEnabledByDefault( false );
	return utest_main( argc, argv );
}
