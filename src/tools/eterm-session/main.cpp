#include "../eterm/harnesssessionrecovery.hpp"

// eepp's config.hpp renames main to SDL_main under MSVC for SDL applications. This console
// helper links no SDL main shim and must keep its real entry point.
#ifdef main
#undef main
#endif

int main( int argc, char* argv[] ) {
	const auto result = eterm::HarnessSessionRecovery::command( argc, argv );
	return result.value_or( 2 );
}
