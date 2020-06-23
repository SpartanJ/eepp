#ifndef EE_UICUIPOPUPMENU
#define EE_UICUIPOPUPMENU

#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

class EE_API UIPopUpMenu : public UIMenu {
  public:
	static UIPopUpMenu* New();

	UIPopUpMenu();

	virtual ~UIPopUpMenu();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual bool show();

	virtual bool hide();

	bool isHiding() const;

  protected:
	Action* mHidingAction{nullptr};
};

}} // namespace EE::UI

#endif
