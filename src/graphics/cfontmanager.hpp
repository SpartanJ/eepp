#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include "base.hpp"
#include "cfont.hpp"

namespace EE { namespace Graphics {

class EE_API cFontManager : public tResourceManager<cFont>, public cSingleton<cFontManager> {
	friend class cSingleton<cFontManager>;
	public:
		cFontManager();

		virtual ~cFontManager();
	protected:
};

}}

#endif
