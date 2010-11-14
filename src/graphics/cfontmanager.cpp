#include "cfontmanager.hpp"

namespace EE { namespace Graphics {

cFontManager::cFontManager() {
}

cFontManager::~cFontManager() {
}

cFont * cFontManager::Add( cFont * Font ) {
	eeASSERT( NULL != Font );
	//eePRINT( "Added font: %s\n", Font->Name().c_str() );

	return tResourceManager<cFont>::Add( Font );
}

}}
