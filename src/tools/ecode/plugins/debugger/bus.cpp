#include "bus.hpp"

namespace ecode {

Bus::State Bus::state() const {
	return mState;
}

void Bus::setState( State state ) {
	if ( state == mState )
		return;
	mState = state;
	onStateChanged( state );
}

void Bus::onStateChanged( State ) {
}

} // namespace ecode
