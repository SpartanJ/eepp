#ifndef EE_UICUIPOPUPMENU
#define EE_UICUIPOPUPMENU

#include <eepp/ui/cuimenu.hpp>

namespace EE { namespace UI {

class EE_API cUIPopUpMenu : public cUIMenu {
	public:
		cUIPopUpMenu( cUIPopUpMenu::CreateParams Params );

		virtual ~cUIPopUpMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		virtual bool Show();

		virtual bool Hide();
	protected:
		virtual void OnComplexControlFocusLoss();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		#ifdef EE_PLATFORM_TOUCH
		Clock mTE;
		#endif
};

}}

#endif

