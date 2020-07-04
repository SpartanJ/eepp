#ifndef EE_UI_UIABSTRACTVIEW_HPP
#define EE_UI_UIABSTRACTVIEW_HPP

#include <eepp/ui/abstract/model.hpp>
#include <eepp/ui/abstract/modeleditingdelegate.hpp>
#include <eepp/ui/abstract/modelselection.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <memory>

namespace EE { namespace UI { namespace Abstract {

class EE_API UIAbstractView : public UIWidget {
  public:
	void set_model( std::shared_ptr<Model> );
	Model* model() { return m_model.get(); }
	const Model* model() const { return m_model.get(); }

	ModelSelection& selection() { return m_selection; }
	const ModelSelection& selection() const { return m_selection; }
	virtual void select_all() = 0;

	bool is_editable() const { return m_editable; }
	void set_editable( bool editable ) { m_editable = editable; }

	virtual void didUpdateModel( unsigned flags );
	virtual void didUpdateSelection();

	virtual Vector2i content_rect( const ModelIndex& ) const { return {}; }
	virtual ModelIndex index_at_event_position( const Vector2i& ) const = 0;

	void set_activates_on_selection( bool b ) { m_activates_on_selection = b; }
	bool activates_on_selection() const { return m_activates_on_selection; }

	std::function<void()> on_selection_change;
	std::function<void( const ModelIndex& )> on_activation;
	std::function<void( const ModelIndex& )> on_selection;
	std::function<void( const ModelIndex&, const DropEvent& )> on_drop;

	//std::function<OwnPtr<ModelEditingDelegate>( const ModelIndex& )> aid_create_editing_delegate;

	void notifySelectionChange();

  protected:
	UIAbstractView();

	virtual ~UIAbstractView();

	virtual void didScroll();
	void set_hovered_index( const ModelIndex& );
	void activate( const ModelIndex& );
	void activate_selected();

	bool m_editable{false};
	ModelIndex m_edit_index;
	UIWidget* m_edit_widget;
	Vector2i m_edit_widget_content_rect;

	Vector2i m_left_mousedown_position;
	bool m_might_drag{false};

	ModelIndex m_hovered_index;

  private:
	std::shared_ptr<Model> m_model;
	std::unique_ptr<ModelEditingDelegate> m_editing_delegate;
	ModelSelection m_selection;
	bool m_activates_on_selection{false};
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTVIEW_HPP
