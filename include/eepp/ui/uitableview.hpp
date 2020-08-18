#ifndef EE_UI_UITABLEVIEW_HPP
#define EE_UI_UITABLEVIEW_HPP

#include <eepp/ui/abstract/uiabstracttableview.hpp>

using namespace EE::UI::Abstract;

namespace EE { namespace UI {

class EE_API UITableView : public UIAbstractTableView {
  public:
	static UITableView* New();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	virtual void drawChilds();

	virtual Node* overFind( const Vector2f& point );

	Float getMaxColumnContentWidth( const size_t& colIndex );

  protected:
	Sizef mContentSize;

	UITableView();

	UITableView( const std::string& tag );

	virtual void createOrUpdateColumns();

	void updateContentSize();

	void onColumnSizeChange( const size_t& );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual ModelIndex findRowWithText( const std::string& text );
};

}} // namespace EE::UI

#endif // EE_UI_UITABLEVIEW_HPP
