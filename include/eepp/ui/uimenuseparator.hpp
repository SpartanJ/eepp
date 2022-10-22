#ifndef EE_UICUISEPARATOR
#define EE_UICUISEPARATOR

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIMenuSeparator : public UIWidget {
  public:
	static UIMenuSeparator* New();

	UIMenuSeparator();

	virtual ~UIMenuSeparator();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

  protected:
};

}} // namespace EE::UI

#endif
