#ifndef EE_UICTHEMEMANAGER
#define EE_UICTHEMEMANAGER

#include "base.hpp"
#include "cuitheme.hpp"

namespace EE { namespace UI {

class EE_API cUIThemeManager : public tResourceManager<cUITheme>, public tSingleton<cUIThemeManager> {
	friend class tSingleton<cUIThemeManager>;
	public:
		cUIThemeManager();

		virtual ~cUIThemeManager();
};

}}

#endif
 
