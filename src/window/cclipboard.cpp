#include "cclipboard.hpp"

namespace EE { namespace Window {

cClipboard::cClipboard( cWindow * window ) :
	mWindow( window )
{}

cClipboard::~cClipboard() {}

cWindow * cClipboard::GetWindow() const {
	return mWindow;
}

}}
