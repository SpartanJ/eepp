#ifndef EE_UICUIPOPUPMENU
#define EE_UICUIPOPUPMENU

#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

class EE_API UIPopUpMenu : public UIMenu {
	public:
		UIPopUpMenu( UIPopUpMenu::CreateParams Params );

		virtual ~UIPopUpMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		virtual bool Show();

		virtual bool Hide();
	protected:
		virtual void OnComplexControlFocusLoss();

		virtual Uint32 OnMessage( const UIMessage * Msg );

		#ifdef EE_PLATFORM_TOUCH
		Clock mTE;
		#endif
};

}}

#endif

