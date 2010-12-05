#ifndef EE_UICUISEPARATOR
#define EE_UICUISEPARATOR

#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class cUISeparator : public cUIControlAnim {
	public:
		cUISeparator( cUIControlAnim::CreateParams Params );
		
		~cUISeparator();
		
		virtual void SetTheme( cUITheme * Theme );
	protected:
};

}}

#endif

