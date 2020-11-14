#ifndef EE_UI_UISTACKWIDGET_HPP
#define EE_UI_UISTACKWIDGET_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIStackWidget : public UIWidget {
  public:
	static UIStackWidget* New();

	static UIStackWidget* NewWithTag( const std::string& tag = "stackwidget" );

	void setActiveWidget( UIWidget* widget );

	UIWidget* getActiveWidget() const;

  protected:
	UIWidget* mActiveWidget{ nullptr };

	UIStackWidget( const std::string& tag = "stackwidget" );

	virtual void onSizeChange();

	virtual void onChildCountChange( Node* child, const bool& removed );
};

}} // namespace EE::UI

#endif // EE_UI_UISTACKWIDGET_HPP
