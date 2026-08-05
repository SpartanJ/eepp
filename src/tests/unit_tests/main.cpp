#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

UTEST_STATE();

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	EE::System::FileSystem::changeWorkingDirectory( EE::System::Sys::getProcessPath() );
	return utest_main( argc, argv );
}
