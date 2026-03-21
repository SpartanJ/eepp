#ifndef EE_UI_UILISTVIEW_HPP
#define EE_UI_UILISTVIEW_HPP

#include <eepp/ui/uitableview.hpp>

namespace EE { namespace UI {

class EE_API UIListView : public UITableView {
  public:
	static UIListView* New();

	static UIListView* NewWithTag( const std::string& tag );

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	void setTheme( UITheme* Theme );

  protected:
	UIListView( const std::string& tag = "listview" );
};

}} // namespace EE::UI

#endif // EE_UI_UILISTVIEW_HPP
