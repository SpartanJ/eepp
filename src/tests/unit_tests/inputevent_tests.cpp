#include "utest.h"
#include <eepp/ui/uiapplication.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;

UTEST( InputEvent, clipboardChangedReachesCallbacksWithoutWindowID ) {
	UIApplication app( WindowSettings{ 320, 240, "Clipboard Event Test" } );
	auto* input = app.getWindow()->getInput();
	unsigned int received = 0;
	Uint32 windowID = 1;
	InputEvent::ClipboardOwner owner = InputEvent::ClipboardOwner::Self;
	const auto callbackID = input->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::ClipboardChanged ) {
			++received;
			windowID = event->WinID;
			owner = event->clipboard.owner;
		}
	} );

	InputEvent event{};
	event.Type = InputEvent::ClipboardChanged;
	event.clipboard.owner = InputEvent::ClipboardOwner::Unknown;
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 1u );
	EXPECT_EQ( windowID, 0u );
	EXPECT_EQ( owner, InputEvent::ClipboardOwner::Unknown );

	event.clipboard.owner = InputEvent::ClipboardOwner::Self;
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 2u );
	EXPECT_EQ( owner, InputEvent::ClipboardOwner::Self );

	event.clipboard.owner = InputEvent::ClipboardOwner::External;
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 3u );
	EXPECT_EQ( owner, InputEvent::ClipboardOwner::External );

	input->popCallback( callbackID );
	EXPECT_TRUE( input->pushEvent( event ) );
	EXPECT_EQ( received, 3u );
}
