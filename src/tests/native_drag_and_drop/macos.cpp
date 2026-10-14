#include <eepp/window.hpp>
#include <iostream>

using namespace EE;
using namespace EE::Window;

// Posting the pointer events an AppKit drag session follows requires the Accessibility permission,
// which an automated run cannot grant. These checks cover everything that needs no real pointer.
int runAutomatedNativeDragTests() {
	auto* window = Engine::instance()->createWindow( { 320, 240, "eepp Cocoa drag source" } );
	if ( !window || !window->isOpen() ) {
		std::cout << "Could not create the source window\n";
		return 1;
	}
	auto drag = DragAndDrop::forWindow( *window );
	const auto capabilities = drag.getCapabilities();
	const bool capabilitiesPassed = capabilities.fileSource && capabilities.dragImage;
	std::cout << "Capabilities: fileSource=" << capabilities.fileSource
			  << " dragImage=" << capabilities.dragImage << " passed=" << capabilitiesPassed
			  << '\n';

	int completions = 0;
	const auto started = drag.startDrag( { { { "/tmp/eepp-cocoa-gesture.txt" } } },
										 [&completions]( DragResult ) { ++completions; } );
	Engine::instance()->updateInput();
	const bool gesturePassed =
		started.status == DragStartStatus::InvalidGesture && completions == 0 && !drag.isDragging();
	std::cout << "Start without a pressed button: status=" << static_cast<int>( started.status )
			  << " completions=" << completions << " passed=" << gesturePassed << '\n';

	std::cout << "Pointer-driven drops need real input; run without --automated to test them.\n";
	Engine::destroySingleton();
	return capabilitiesPassed && gesturePassed ? 0 : 1;
}
