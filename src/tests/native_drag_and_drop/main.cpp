#include "../../examples/native_drag_and_drop/native_drag_and_drop.hpp"
#include "testexitcode.hpp"
#include <array>
#include <chrono>
#include <eepp/system/sys.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace EE::System;

int runAutomatedNativeDragTests();

#if EE_PLATFORM == EE_PLATFORM_LINUX
/** Receiving drags without files (setDataDropHandler / setFileSaveDropHandler): X11 only. */
int runAutomatedNativeDropTests();

int runAutomatedWaylandDragTests( bool receiver, bool processExit = false );
#endif

namespace {
std::string utf8Path( const std::filesystem::path& path ) {
#if EE_PLATFORM != EE_PLATFORM_WIN
	return path.native(); // POSIX native paths already contain UTF-8 bytes; one owning copy.
#else
	const auto bytes = path.u8string();
	return { reinterpret_cast<const char*>( bytes.data() ), bytes.size() };
#endif
}

/** Own only the generated fixtures, never files supplied by the user or copied into this folder. */
struct SampleFiles {
	std::filesystem::path directory;
	std::array<std::filesystem::path, 2> files;

	~SampleFiles() {
		std::error_code error;
		for ( const auto& file : files ) {
			if ( !file.empty() )
				std::filesystem::remove( file, error );
		}
		if ( !directory.empty() )
			std::filesystem::remove( directory, error );
	}

	bool create( std::vector<std::string>& arguments ) {
		std::error_code error;
		auto candidate = std::filesystem::temp_directory_path( error );
		if ( error )
			return false;
		candidate /= "eepp-manual-dnd-" + std::to_string( Sys::getProcessID() ) + "-" +
					 std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
		if ( !std::filesystem::create_directory( candidate, error ) )
			return false;
		directory = std::move( candidate );
		files = { directory / std::filesystem::path( u8"sample #é-one.txt" ),
				  directory / "sample two.txt" };
		arguments.reserve( files.size() );
		for ( size_t i = 0; i < files.size(); ++i ) {
			std::ofstream output( files[i], std::ios::binary );
			output << "eepp manual drag-and-drop sample " << i + 1 << '\n';
			output.close();
			if ( !output )
				return false;
			arguments.emplace_back( utf8Path( files[i] ) );
		}
		return true;
	}
};
} // namespace

EE_MAIN_FUNC int main( int argc, char** argv ) {
	auto arguments = Sys::parseArguments( argc, argv );
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( arguments.size() == 1 && arguments[0] == "--automated-wayland-exit" )
		return runAutomatedWaylandDragTests( false, true );
	if ( arguments.size() == 1 &&
		 ( arguments[0] == "--automated-wayland" || arguments[0] == "--automated-wayland-target" ) )
		return runAutomatedWaylandDragTests( arguments[0] == "--automated-wayland-target" );
#endif
	if ( arguments.size() == 1 && arguments[0] == "--automated" ) {
		std::cout << "Automated integration checks: synthetic pointer and keyboard input.\n"
					 "Run on an isolated display.\n"
				  << std::flush;
		const int drag = runAutomatedNativeDragTests();
#if EE_PLATFORM == EE_PLATFORM_LINUX
		// Both are skipped together, on a display that is not X11.
		const int drop = runAutomatedNativeDropTests();
		if ( drag == EXIT_SUCCESS || drag == TestSkipped )
			return drop;
#endif
		return drag;
	}
	if ( arguments.size() == 1 && arguments[0] == "--help" ) {
		std::cout << "Usage: eepp-native-dnd-tests [--manual] [file paths...]\n"
					 "Default: manual source and receiving windows; no synthetic input.\n"
					 "Without paths, two temporary sample files are provided.\n"
					 "--automated: run integration checks on an isolated display.\n";
		return EXIT_SUCCESS;
	}
	if ( !arguments.empty() && arguments[0] == "--manual" )
		arguments.erase( arguments.begin() );
	for ( auto& argument : arguments ) {
		if ( argument.starts_with( "--" ) ) {
			std::cerr << "Unknown option or mixed modes: " << argument << ". Use --help.\n";
			return EXIT_FAILURE;
		}
		std::error_code error;
		const std::u8string_view bytes( reinterpret_cast<const char8_t*>( argument.data() ),
										argument.size() );
		const auto path = std::filesystem::absolute( std::filesystem::path( bytes ), error );
		if ( error ) {
			std::cerr << "Could not resolve file path: " << argument << '\n';
			return EXIT_FAILURE;
		}
		argument = utf8Path( path );
	}
	SampleFiles samples;
	if ( arguments.empty() && !samples.create( arguments ) ) {
		std::cerr << "Could not create temporary sample files. Supply your own file paths.\n";
		return EXIT_FAILURE;
	}
	return EE::Examples::runNativeDragDemo( std::move( arguments ), true );
}
