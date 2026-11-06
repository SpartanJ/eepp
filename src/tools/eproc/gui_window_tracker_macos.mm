#include "gui_window_tracker.hpp"

#import <AppKit/AppKit.h>

namespace eproc {

GuiWindowTracker::GuiWindowTracker() = default;
GuiWindowTracker::~GuiWindowTracker() = default;

void GuiWindowTracker::refresh() {
	mPids.clear();
	@autoreleasepool {
		for ( NSRunningApplication* app in [[NSWorkspace sharedWorkspace] runningApplications] ) {
			if ( !app.terminated &&
				 ( app.activationPolicy == NSApplicationActivationPolicyRegular ||
				   app.activationPolicy == NSApplicationActivationPolicyAccessory ) &&
				 app.processIdentifier > 0 )
				mPids.insert( app.processIdentifier );
		}
	}
}

DrawablePtr GuiWindowTracker::iconForPid( long ) {
	return {};
}

bool GuiWindowTracker::isAvailable() const {
	return true;
}

} // namespace eproc
