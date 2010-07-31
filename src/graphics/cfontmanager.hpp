#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include "base.hpp"
#include "cfont.hpp"
#include "../system/tresourcemanager.hpp"

namespace EE { namespace Graphics {

class cFontManager : public tResourceManager<cFont>, public cSingleton<cFontManager> {
	friend class cSingleton<cFontManager>;
	public:
		cFontManager();

		virtual ~cFontManager();
	protected:
};

}}

#endif
