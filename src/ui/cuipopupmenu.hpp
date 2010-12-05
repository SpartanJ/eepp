#ifndef EE_UICUIPOPUPMENU
#define EE_UICUIPOPUPMENU

#include "cuimenu.hpp"

namespace EE { namespace UI {

class cUIPopUpMenu : public cUIMenu {
	public:
		cUIPopUpMenu( cUIPopUpMenu::CreateParams Params );

		~cUIPopUpMenu();

		virtual void SetTheme( cUITheme * Theme );

		bool Show();

		bool Hide();
	protected:
		virtual void OnComplexControlFocusLoss();

		virtual Uint32 OnMessage( const cUIMessage * Msg );
};

}}

#endif

