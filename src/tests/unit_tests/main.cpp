#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

UTEST_STATE();

int main( int argc, const char* const argv[] ) {
	EE::System::FileSystem::changeWorkingDirectory( EE::System::Sys::getProcessPath() );
	return utest_main( argc, argv );
}
