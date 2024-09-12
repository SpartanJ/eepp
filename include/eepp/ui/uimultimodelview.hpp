#ifndef EE_UI_UIMULTIMODELVIEW_HPP
#define EE_UI_UIMULTIMODELVIEW_HPP

#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uistackwidget.hpp>
#include <eepp/ui/uitableview.hpp>

namespace EE { namespace UI {

class EE_API UIMultiModelView : public UIStackWidget {
  public:
	enum ViewMode { List, Table };

	static UIMultiModelView* New();

	static UIMultiModelView* NewWithTag( const std::string& tag );

	const ViewMode& getViewMode() const;

	void setViewMode( const ViewMode& mode );

	UIAbstractView* getCurrentView() const;

	std::shared_ptr<Model> getModel() const;

	void setModel( const std::shared_ptr<Model>& model );

	std::function<void( const ModelIndex& )> getOnSelection() const;

	void setOnSelection( const std::function<void( const ModelIndex& )>& onSelection );

	std::function<void()> getOnSelectionChange() const;

	void setOnSelectionChange( const std::function<void()>& onSelectionChange );

	ModelSelection& getSelection() { return getCurrentView()->getSelection(); }

	const ModelSelection& getSelection() const { return getCurrentView()->getSelection(); }

	UIListView* getListView() const { return mList; }

	UITableView* getTableView() const { return mTable; }

	void setSelection( const ModelIndex& index, bool scrollToSelection = true );

	void setSingleClickNavigation( bool singleClickNavigation );

	void setMultiSelect( bool enable );

  protected:
	UIMultiModelView( const std::string& tag );

	std::shared_ptr<Model> mModel;
	ViewMode mMode{ List };
	UIListView* mList{ nullptr };
	UITableView* mTable{ nullptr };

	std::function<void()> mOnSelectionChange;
	std::function<void( const ModelIndex& )> mOnSelection;
};

}} // namespace EE::UI

#endif // EE_UI_UIMULTIMODELVIEW_HPP
