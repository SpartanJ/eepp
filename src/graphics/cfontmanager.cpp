#include "cfontmanager.hpp"

namespace EE { namespace Graphics {

cFontManager::cFontManager() {
}

cFontManager::~cFontManager() {
}

cFont * cFontManager::Add( cFont * Font ) {
	eeASSERT( NULL != Font );
	return tResourceManager<cFont>::Add( Font );
}

}}
