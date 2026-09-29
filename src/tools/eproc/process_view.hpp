#ifndef EPROC_PROCESS_VIEW_HPP
#define EPROC_PROCESS_VIEW_HPP

#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace EE;
using namespace EE::UI;

namespace eproc {

inline bool focusFirstProcessRow( UIAbstractTableView* view ) {
	if ( !view || !view->getModel() || view->getModel()->rowCount() == 0 )
		return false;
	const ModelIndex first = view->getModel()->index(
		0, view->isType( UI_TYPE_TREEVIEW ) ? view->getModel()->treeColumn() : 0 );
	if ( !first.isValid() )
		return false;
	view->setFocus();
	view->setSelection( first, true, false );
	return true;
}

inline bool focusSearchFromFirstRow( UIAbstractTableView* view, UITextInput* searchInput,
									 const KeyEvent& event ) {
	if ( !searchInput || event.getKeyCode() != KEY_UP || event.getSanitizedMod() != KEYMOD_NONE ||
		 view->isEditing() || !view->getModel() || view->getSelection().size() != 1 )
		return false;
	const ModelIndex selected = view->getSelection().first();
	if ( !selected.isValid() || selected.row() != 0 || selected.parent().isValid() )
		return false;
	searchInput->setFocus();
	return true;
}

template <typename Base> class ProcessView : public Base {
  public:
	static ProcessView* New() { return eeNew( ProcessView, () ); }

	ProcessView() : Base() {}

	void setSearchInput( UITextInput* searchInput ) { mSearchInput = searchInput; }

  protected:
	Uint32 onKeyDown( const KeyEvent& event ) override {
		return focusSearchFromFirstRow( this, mSearchInput, event ) ? 1 : Base::onKeyDown( event );
	}

  private:
	UITextInput* mSearchInput{ nullptr };
};

using ProcessTableView = ProcessView<UITableView>;
using ProcessTreeView = ProcessView<UITreeView>;

} // namespace eproc

#endif
