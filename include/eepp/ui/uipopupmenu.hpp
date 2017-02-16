#ifndef EE_UICUIPOPUPMENU
#define EE_UICUIPOPUPMENU

#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

class EE_API UIPopUpMenu : public UIMenu {
	public:
		UIPopUpMenu( UIPopUpMenu::CreateParams Params );

		virtual ~UIPopUpMenu();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual bool show();

		virtual bool hide();
	protected:
		virtual void onComplexControlFocusLoss();

		virtual Uint32 onMessage( const UIMessage * Msg );

		#ifdef EE_PLATFORM_TOUCH
		Clock mTE;
		#endif
};

}}

#endif

